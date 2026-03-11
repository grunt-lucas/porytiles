# Analysis: Packing Diagnostics and Upfront Feasibility Warnings

## Context

Users frequently complain that Porytiles "times out without finding a solution." The current error messages are completely generic:
```
"Backtracking strategy failed to find a valid palette assignment after all preset configurations."
```
No information about WHY it failed, which tiles are problematic, or what the user can do about it.

This analysis covers two improvements grounded in the theory from Grange et al. ("Algorithms for the Bin Packing Problem with Overlapping Items", 2017):

1. **Better post-failure error messages** — actionable diagnostics when all matrix configs are exhausted
2. **Upfront feasibility warnings** — quickly computable metrics that flag hard/impossible inputs before packing

---

## Part 1: Pre-Packing Feasibility Analysis

Fast checks that run BEFORE attempting any packing strategy. All O(n) or O(n²) with small n — microseconds for typical Porytiles inputs (50-300 tiles).

### Tier A: Provably Impossible Checks (emit as errors, skip packing entirely)

**1. Color Budget Violation: |A| > n × C**

If the total unique colors across all tiles exceeds the total available palette slots, no solution exists regardless of algorithm.

```
error[palette-budget-exceeded]: Tileset 'gTileset_PetalburgCity' uses 97 unique colors,
  but only 6 palettes are available (6 × 15 = 90 color slots).
  No valid palette assignment exists. Reduce your color count by at least 7.
```

- Cost: O(tiles × avg_colors_per_tile) — one pass, union all ColorSets, count
- Paper basis: Follows directly from the ILP constraint ∑x_k ≤ z_k × C (Eq. 3)

**2. Tile Too Large: |t| > C (paper Rule 3)**

Any tile with more than 15 unique colors (after transparency) literally cannot fit in a GBA palette.

```
error[tile-exceeds-capacity]: Tile in metatile 23 (bottom-left) uses 17 unique colors,
  but each palette can hold at most 15. This tile is impossible to assign.
```

- Cost: O(tiles) — trivial
- Paper basis: Rule 3: "Each tile has less than C symbols"

**3. Incompatible Loner Tiles: Rule 6 violation**

Paper's Rule 6: every tile must be compatible with at least one other (|t ∪ t'| ≤ C for some t'). If a tile is incompatible with ALL others, it must consume a palette alone. If the number of such "loner" tiles exceeds n, it's impossible.

More generally: if the incompatibility graph has a clique of size > n, it's provably impossible (those tiles all need separate palettes).

```
error[incompatible-clique]: Found 7 tiles that are mutually incompatible
  (no two can share a palette), but only 6 palettes are available:
    Tile in metatile 5 (top-left): 14 colors
    Tile in metatile 12 (bottom-right): 13 colors
    ...
  At least one of these tiles must be modified to share colors with another.
```

- Cost: O(tiles²) for pairwise compatibility check. Finding a clique of size > n in the incompatibility graph can be done with simple greedy/BFS for our small graph sizes.

### Tier B: Difficulty Warnings (emit as warnings, proceed with packing)

**4. Average Multiplicity Warning (paper Section 4.3, Conjecture 2)**

Average multiplicity = Card(T)/|A| correlates with difficulty at r=0.784 — the single best predictor from the paper. Already implemented as `compute_average_multiplicity()` in `packing_metrics.hpp` but **not wired into the pipeline**.

Thresholds (need tuning on real Porytiles tilesets):
- avg_mult < 3: Very low color sharing — extremely difficult for heuristics
- avg_mult 3-8: Moderate sharing — may require extensive search
- avg_mult > 8: Good sharing — should pack efficiently

```
warning[low-color-sharing]: Your tileset has low color sharing (average multiplicity: 2.3).
  Each color appears in only ~2 tiles on average, making palette optimization difficult.
  Consider reusing existing colors across more tiles instead of introducing new ones.
```

- Cost: Already implemented — O(tiles × colors), microseconds
- Paper basis: Section 4.3, Conjecture 2

**5. Color Budget Tightness**

Even when |A| ≤ n × C, a tight ratio means very little slack for the packer.

```
warning[tight-color-budget]: Your tileset uses 88 of 90 available color slots (98% utilization).
  Packing requires near-perfect color sharing across palettes, which may take longer or fail.
```

- Metric: |A| / (n × C) — warn if > 0.85 or so
- Cost: O(tiles × colors) to compute |A|

**6. High-Pressure Tiles**

Tiles with |t| ≥ C-2 (13+ colors) leave very few spare slots for sharing.

```
warning[high-pressure-tiles]: 4 tiles use 13 or more colors (out of 15 max per palette):
  Tile in metatile 23 (bottom-left): 15 colors — completely fills a palette alone.
  Tile in metatile 41 (top-right): 14 colors — leaves only 1 slot for sharing.
  Tiles with near-maximum colors severely constrain palette assignment.
```

- Cost: O(tiles), trivial

**7. Incompatible Pair Density**

High ratio of incompatible tile pairs means the problem is highly constrained.

```
warning[high-incompatibility]: 34 of 120 tile pairs (28%) cannot share a palette.
  The most constrained pairs:
    Tiles in metatile 5 and metatile 19: combined 28 colors (max 15 per palette).
    ...
```

- Cost: O(tiles²), ~20K bitset operations for 200 tiles — microseconds

### Tier C: Additional Indicators (lower priority, but theoretically interesting)

**8. Color Fragmentation Score**

Count colors with multiplicity = 1 (appear in only one tile). These "unique colors" consume palette slots but provide zero sharing benefit.

- Metric: count of colors α where μ(α) = 1, divided by |A|
- High fragmentation (> 50% unique colors) = significant palette pressure

**9. Connected Components of the Compatibility Graph**

If tiles form disconnected groups (no colors shared between groups), each group is an independent sub-problem. If any single component requires more palettes than available, it's infeasible.

- Cost: O(tiles² + tiles) — build compatibility graph, run BFS/DFS
- Practical value: identifies natural palette groupings

**10. Tile Size Variance**

High variance (some 3-color tiles, some 14-color tiles) creates harder packing than uniform sizes. The paper's instance generator uses normally-distributed tile sizes.

---

## Part 2: Post-Failure Diagnostic Enhancement

When all strategy configurations are exhausted, provide rich, actionable diagnostics.

### Search Progress Tracking

**Current state**: DFS/BFS return only `AssignResult::{success, no_solution, cutoff_reached}` — zero metadata on failure.

**Proposed**: Add a `SearchProgress` struct returned alongside the result:

```c++
struct SearchProgress {
    std::size_t max_tile_depth;     // deepest tile index successfully assigned before failure
    std::size_t nodes_explored;     // total nodes explored
    AssignResult result;            // why it stopped
};
```

Each matrix iteration returns this. Aggregate across all configs:
- **"Closest to success"**: which config assigned the most tiles before failing
- **"Bottleneck tile"**: the tile index where search most often stalls — this is the tile the user should focus on

Implementation: minimal overhead — update `max_tile_depth` on each successful recursive call in `assign_depth_first` (one comparison per node).

### Tile Difficulty Ranking

After failure, rank all tiles by "difficulty contribution":

```
score(t) = |t| × incompatible_count(t)
```

Tiles that are both LARGE and INCOMPATIBLE with many others are the prime candidates for user intervention.

### Concrete Suggestions Engine

Based on the analysis, generate specific suggestions:

**If color budget is tight:**
```
note: Suggestions to resolve:
  Reduce unique colors from 88 to ~80 for comfortable packing.
  These colors appear in only 1 tile and could be merged with similar colors:
    Color (234, 21, 97) in metatile 15 — similar to (230, 22, 95) used in 4 other tiles.
```

**If specific tiles are bottlenecks:**
```
note: Most problematic tiles:
  Tile #42 (metatile 7, bottom-middle): 14 colors, incompatible with 5 other large tiles.
  Tile #17 (metatile 3, top-left): 13 colors, shares only 2 colors with tile #42.
  Making tiles #42 and #17 share 3 more colors would let them fit in the same palette.
```

**If multiplicity is low:**
```
note: Your tileset has very low color sharing (average multiplicity: 2.1).
  This approaches a standard Bin Packing instance, which is much harder to optimize.
  Consider reusing existing palette colors instead of introducing unique colors per tile.
```

### Complete Post-Failure Message Format

```
error: Failed to pack palettes for tileset 'gTileset_PetalburgCity'.

note: Tried 48 backtracking configurations, none found a valid assignment.
  Closest to success: DFS with 4M node cutoff assigned 42 of 47 tiles before exhausting search.
  Bottleneck: tile #43 (metatile 8, top-right) — where search repeatedly stalled.

note: Tileset analysis:
  87 unique colors, 6 palettes available (90 slots) — 97% utilization needed.
  Average color sharing: 4.1 (each color appears in ~4 tiles — moderate).
  3 tiles use 14+ colors, severely constraining assignments.
  8 tile pairs are completely incompatible.

note: Most problematic tiles:
  Tile #43 (metatile 8, top-right): 14 colors, incompatible with 5 other tiles.
  Tile #17 (metatile 3, top-left): 13 colors, shares only 2 colors with tile #43.

note: Suggestions:
  Reduce unique colors from 87 to ~80 for comfortable packing.
  Focus on tile #43 — making it share 3+ more colors with tile #17 would help most.
  Consider using palette hints to guide specific color groupings.
```

---

## Part 3: Existing Infrastructure to Leverage

| Function/Type                                                             | Location               | Status                                  |
|---------------------------------------------------------------------------|------------------------|-----------------------------------------|
| `compute_average_multiplicity()`                                          | `packing_metrics.hpp`  | Implemented, NOT wired into pipeline    |
| `build_global_multiplicity_map()`                                         | `packing_metrics.hpp`  | Implemented, NOT wired into pipeline    |
| `color_set_count()`, `union_size()`, `intersection_size()`, `is_subset()` | `color_set.hpp`        | Fully implemented, efficient bitset ops |
| `for_each_color()`                                                        | `color_set.hpp`        | Efficient Brian Kernighan bit scanning  |
| `UserDiagnostics::warning()`, `::remark()`, `::note()`                    | `user_diagnostics.hpp` | Full diagnostic severity system         |
| `PackableTile::color_set()`, `::color_count()`, `::id()`                  | `packable_tile.hpp`    | Tile-level accessors                    |
| `FormattableError` with `FormatParam` + `Style::bold`                     | error system           | Rich error formatting                   |

### Key files to modify

- `Porytiles2/lib/domain/packing/services/palette_packer.cpp` — add pre-packing analysis before calling strategy
- `Porytiles2/lib/domain/packing/services/backtracking_strategy.cpp` — add SearchProgress tracking, enhance error return
- `Porytiles2/lib/domain/packing/services/overload_and_remove_strategy.cpp` — same as above
- `Porytiles2/lib/domain/packing/algorithms/packing_metrics.hpp` — add new metric functions (compatibility density, tile difficulty score, etc.)
- `Porytiles2/lib/domain/services/primary_tileset_compiler.cpp` — wire pre-packing warnings into pipeline

### New code to write

- `PackingFeasibilityAnalyzer` (or free functions) in domain/packing/algorithms — runs all Tier A/B checks
- `SearchProgress` struct — returned by DFS/BFS alongside result
- `PackingFailureDiagnostics` — post-failure analysis and suggestion generation
- Incompatibility graph construction + clique detection (simple greedy for small graphs)

---

## Summary

**Question 1 (better error messages)**: Yes — rich post-failure diagnostics are feasible. The key additions are (a) `SearchProgress` tracking in DFS/BFS to identify bottleneck tiles and closest-to-success configs, (b) tile-level difficulty analysis using incompatibility counts and color pressure, and (c) a suggestions engine that translates analysis into concrete user actions.

**Question 2 (upfront warnings)**: Yes — multiple cheaply computable metrics beyond multiplicity:
- **Color budget check**: |A| vs n×C (provably impossible if exceeded)
- **Average multiplicity**: Already implemented, r=0.784 correlation, just needs wiring
- **Incompatibility analysis**: pairwise compatibility density + clique detection
- **High-pressure tile detection**: tiles with |t| ≥ C-2
- **Color fragmentation**: proportion of unique-to-one-tile colors

All of these run in microseconds for typical Porytiles input sizes. The most impactful additions would be (in priority order):
1. Color budget impossible-check (trivial to implement, catches obvious cases)
2. Average multiplicity warning (already implemented, just wire it in)
3. High-pressure tile warning (trivial, very actionable for users)
4. Post-failure bottleneck tile identification (requires SearchProgress, most value for debugging)
5. Incompatibility clique detection (most theoretically powerful, moderate implementation effort)
