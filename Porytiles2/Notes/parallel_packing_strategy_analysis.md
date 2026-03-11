# Analysis: Parallelizing Packing Strategy Matrix Execution

## Context

Porytiles' palette packing solves the Pagination problem (Bin Packing with Overlapping Items, NP-complete). The `BacktrackingStrategy` tries 48 parameter configurations sequentially, and `OverloadAndRemoveStrategy` tries 17 — both using a "first success wins" sequential fallback. The question: can we leverage multi-core processors via a threadpool to try these configurations simultaneously?

## Answer: Yes — the architecture is already parallelism-friendly

**There are no hard blocking bottlenecks.** The iterations are embarrassingly parallel. Here's the detailed breakdown:

---

### Why it works — State isolation analysis

| Resource              | Thread-safe? | Details                                                                          |
|-----------------------|--------------|----------------------------------------------------------------------------------|
| `PackingInput`        | **Yes**      | Passed as `const &`, never mutated                                               |
| `SearchContext`       | **Yes**      | Built fresh per `pack()` call from immutable input                               |
| DFS `palette_colors`  | **Yes**      | `auto colors = ctx.initial_palette_colors` — local copy per iteration (line 532) |
| BFS `solution` vector | **Yes**      | Local per iteration (line 541)                                                   |
| O&R PRNG state        | **Yes**      | Each config has its own seed, creates own `mt19937`                              |
| Global/static state   | **Yes**      | None exists — confirmed by full codebase search                                  |
| `UserDiagnostics*`    | **No**       | Only shared resource — see below                                                 |

### The one shared resource: UserDiagnostics

`diag_->remark()` is called only **after** a successful result (lines 534-535, 544-545). This is easily addressed:
- **Option A**: Defer diagnostic emission — store winning params, emit after joining all threads
- **Option B**: Add a mutex around diagnostic calls
- **Option C**: Give each thread a thread-local `BufferedUserDiagnostics`, merge after

Option A is cleanest since diagnostics only fire once (on success).

---

### Where parallelism helps and doesn't

| Tileset difficulty                | Sequential behavior                                        | Parallel benefit                                  |
|-----------------------------------|------------------------------------------------------------|---------------------------------------------------|
| **Easy** (config 1 succeeds)      | Returns immediately                                        | ~0% improvement, negligible overhead              |
| **Medium** (config 5-15 succeeds) | Burns through 4-14 failed configs                          | Moderate wall-clock improvement                   |
| **Hard** (config 20+ succeeds)    | Burns through 19+ failed configs, each hitting node_cutoff | **4-8x wall-clock improvement** on 8-core machine |

Each failed BacktrackingStrategy config explores up to its node_cutoff (1M-8M nodes). For hard tilesets, sequential scanning through many failed configs is the dominant cost. Parallelism directly attacks this.

---

### Design sketch: Threadpool with cooperative cancellation

**Key insight**: The existing `node_cutoff` check in DFS (line 174: `if (explored_nodes > params.node_cutoff)`) is a natural injection point for cooperative cancellation. Adding an `std::atomic<bool>` check alongside it costs one atomic load per node — negligible.

```c++
// Pseudocode for parallel preset matrix execution
std::atomic<bool> found{false};
ThreadPool pool(std::thread::hardware_concurrency());
std::vector<std::future<std::optional<PackingOutput>>> futures;

for (const auto &params : matrix) {
    futures.push_back(pool.submit([&input, &found, params]()
        -> std::optional<PackingOutput> {
        if (found.load(std::memory_order_relaxed)) return std::nullopt;
        auto result = run_single_config(input, params, &found);
        if (result.has_value()) {
            found.store(true, std::memory_order_relaxed);
        }
        return result;
    }));
}

// Collect first success
for (auto &f : futures) {
    auto result = f.get();
    if (result.has_value()) return *result;  // or collect best
}
```

### Semantic upgrade opportunity

Parallelism naturally enables a shift from "first valid solution" to **"best solution"** — run all configs, compare results by quality metric (fewest palettes, best color utilization, lowest "loss" as defined in the pagination paper). This is something sequential first-success can never do efficiently. It would be a genuine **quality** improvement, not just speed.

---

### Concerns and mitigations

| Concern                            | Severity | Mitigation                                                                                                                              |
|------------------------------------|----------|-----------------------------------------------------------------------------------------------------------------------------------------|
| Easy tilesets gain nothing         | Low      | Threadpool overhead is minimal; fast configs complete before pool even ramps up                                                         |
| BFS memory × thread count          | Low      | GBA palettes are small (~13 palettes × 256-bit bitsets); even 48 concurrent BFS runs use negligible memory                              |
| Matrix ordering loses meaning      | Medium   | With parallelism, the cheap→expensive ordering is irrelevant. Cancellation flag handles early termination instead                       |
| Build complexity (threadpool impl) | Medium   | C++23 has no standard threadpool, but `std::jthread` + `std::async` or a simple custom pool works. Could also use a header-only library |
| Cross-platform thread behavior     | Low      | `std::thread`/`std::jthread` are standard C++; works on both GCC and Clang                                                              |

### Higher-level parallelism opportunity

Beyond parallelizing within a single strategy's matrix, you could also run **multiple strategies simultaneously** (e.g., BacktrackingStrategy AND OverloadAndRemoveStrategy in parallel), picking the best result from either. The `PackingStrategy` interface already supports this cleanly.

---

### Critical files

- `Porytiles2/lib/domain/packing/services/backtracking_strategy.cpp` — lines 525-550 (matrix loop)
- `Porytiles2/lib/domain/packing/services/overload_and_remove_strategy.cpp` — lines 172-187 (matrix loop)
- `Porytiles2/include/porytiles2/domain/packing/services/packing_strategy.hpp` — PackingStrategy interface
- `Porytiles2/include/porytiles2/xcut/diagnostics/user_diagnostics.hpp` — shared diagnostic resource

### Summary

The packing strategies are **already designed for parallelism** (stateless, const interface, independent iterations, no global state). A threadpool approach is the right fit. The main design decisions are: (1) cooperative cancellation via atomic flag injected into the DFS/BFS inner loop, (2) deferred diagnostic emission, and (3) whether to upgrade to "best solution" semantics. No architectural changes needed — this is an implementation-level enhancement inside the existing `pack()` methods.
