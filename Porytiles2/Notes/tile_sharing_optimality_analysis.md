# Optimal Tile Sharing: Implementation Guide

## Purpose and Scope

This document is the implementation guide for the two unimplemented tile sharing modes:
`TileSharingPacking::optimal` and `TileSharingAlignment::optimal`. Both currently panic at runtime
(`palette_packer.cpp:281-286`). For background on GBA tile sharing, shape groups, and the existing
greedy Indirection pipeline, see [tile_sharing_indirect_system.md](tile_sharing_indirect_system.md).


## Implementation Status

| Packing   | Alignment | Behavior                                | Status      |
|-----------|-----------|-----------------------------------------|-------------|
| `off`     | `off`     | No sharing                              | Implemented |
| `off`     | `greedy`  | Post-pack alignment only                | Implemented |
| `biased`  | `off`     | Biased packing, no alignment            | Implemented |
| `biased`  | `greedy`  | Biased packing + greedy alignment       | Implemented |
| `optimal` | `optimal` | Hard-constraint packing + CSP alignment | **TODO**    |
| `optimal` | `greedy`  | Hard-constraint packing + greedy align  | **TODO**    |
| `biased`  | `optimal` | Soft packing + CSP alignment            | **TODO**    |
| `off`     | `optimal` | No packing awareness + CSP alignment    | **TODO**    |


## Optimal Packing

### Goal

Reject any palette assignment that co-places shape group siblings. The search must find a packing
where every shape group has all members in distinct palettes, or report infeasibility.

### Algorithm

Extend the existing backtracking packer with a hard rejection check. At each candidate placement,
query `ShapeGroupMetadata` for whether the candidate tile's shape group already has a member in the
target palette. If so, prune that branch immediately.

This is the same bin-packing search (NP-hard), but with additional pruning constraints. The added
constraints can either speed up the search (more pruning = smaller search tree) or make feasible
solutions harder to find (more rejections = fewer valid packings).

### Integration Points

**1. `PackingInput` needs a mode signal** (`packing_strategy.hpp:21-83`)

Currently, `PackingInput` carries `std::optional<ShapeGroupMetadata>` but no enum distinguishing
`biased` from `optimal`. The strategies interpret metadata presence as "penalize siblings":
- BestFusion/O&R: adds a cost penalty
- Backtracking: sorts sibling-containing palettes lower in the candidate list

For optimal mode, `BacktrackingStrategy` must **hard-reject** (prune the branch) instead of merely
sorting lower. Two approaches:

- **Option A**: Add a `TileSharingPacking` field to `PackingInput` so strategies can switch behavior.
- **Option B**: Wrap the metadata in a discriminated type (e.g., `SharingConstraint{mode, metadata}`)
  that encodes the enforcement level.

Option A is simpler and consistent with how `PackingParams` already carries the enum.

**2. Strategy-specific notes**

Only `BacktrackingStrategy` can guarantee hard constraints — it explores the full search tree with
pruning. `BestFusionStrategy` and `OverloadAndRemoveStrategy` are greedy and cannot guarantee that
all siblings end up in different palettes. Therefore:

- When `TileSharingPacking::optimal` is selected, the system should **force `BacktrackingStrategy`**
  regardless of the user's `packing_strategy_type` config setting.
- Emit a diagnostic remark if the user's configured strategy was overridden (e.g., "Packing strategy
  overridden to Backtracking: optimal tile sharing requires exhaustive search.").
- This override should happen in `palette_packer.cpp` at the point where the panic currently lives
  (line 281), before the `strategy_->pack()` call at line 305.

**3. `palette_packer.cpp` orchestration changes** (line 281+)

Replace the panic with:

1. Force `BacktrackingStrategy` (see above)
2. Build shape group metadata (reuse the existing `biased` path at lines 293-303, but unconditionally)
3. Set `PackingInput::shape_group_metadata_` with the mode signal
4. Call `strategy_->pack()` — the strategy interprets the mode signal as hard-reject

**4. `backtracking_strategy.cpp` changes**

In the DFS/BFS candidate evaluation loop, after checking palette capacity, add:

```
if (mode == optimal && palette_contains_sibling(tile_id, candidate_pal, metadata)):
    skip candidate  // hard prune, do not explore this branch
```

The existing `biased` path that sorts siblings lower can remain as a separate code path.

### Feasibility and Failure

Strict separation may be infeasible. Example: a shape group with 7 members whose color sets can only
fit in 5 palettes. With only 6 primary palettes available, at least two siblings must share a palette.

**Diagnostics for infeasibility:**

- After exhausting the backtracking search tree, report which shape groups have more members than
  available palettes (the trivially infeasible case).
- For non-trivial infeasibility (color budget interactions), report the last partial solution found
  and which groups could not be separated.
- Suggest falling back to `biased` packing, which will do its best-effort.

### Packing and Palette Count Interaction

Sharing saves tile VRAM but may cost palette VRAM. If strict separation requires 14 palettes instead
of 12, but saves 30 tile slots, the tradeoff may or may not be worthwhile. This is tileset-specific
and should be surfaced to the user via diagnostics rather than decided automatically.


## Optimal Alignment

### Goal

Find a slot assignment for all palette colors such that every sharing constraint is simultaneously
satisfied, or prove that no such assignment exists. This replaces the greedy Indirection pipeline's
best-effort approach and its inherent failure modes (first-writer-wins, cross-group interference,
greedy reference selection).

### Algorithm: CSP Reduction

The key insight: optimal alignment **reduces to graph coloring**.

#### Step 1: Build equivalence classes (union-find)

Sharing constraints say "color A in palette X must occupy the same slot as color B in palette Y."
These constraints are transitive: if A↔B and B↔C, then A↔C. Use **union-find** to merge all
transitively-connected (palette, color) pairs into equivalence classes. Each class must be assigned
exactly one slot value.

```
// Pseudocode
UnionFind<(PaletteIndex, Color)> uf;

for each ShapeGroup g:
    pick reference member ref
    for each other member m in g (different palette than ref):
        for each ShapeMask mask in g.canonical_shape:
            color_ref = ref.colors[mask]
            color_m = m.colors[mask]
            uf.merge((ref.palette, color_ref), (m.palette, color_m))
```

Unlike the greedy pipeline, reference member selection here does **not** affect correctness — only
which pairs get merged first. All transitive constraints are captured regardless of reference choice.

#### Step 2: Add prefilled constraints

For each prefilled (palette, color, slot) triple, fix that equivalence class's slot assignment.
If two colors in the same equivalence class have different prefilled slots, the problem is
immediately unsatisfiable — report the conflict and stop.

#### Step 3: Build conflict graph

- **Nodes**: equivalence classes from Step 1
- **Edges**: two classes that coexist in at least one palette (they cannot share a slot, because two
  different colors cannot occupy the same slot within one palette)
- **Pre-assigned colors**: classes with prefilled slot constraints

```
// Pseudocode
ConflictGraph G;

for each palette P:
    classes_in_P = { uf.find((P, color)) for color in P.colors }
    for each pair (class_a, class_b) in classes_in_P where class_a != class_b:
        G.add_edge(class_a, class_b)
```

#### Step 4: Find a 15-coloring

Find a valid coloring of the conflict graph with colors in {1, 2, ..., 15} (slot indices), respecting
pre-assigned colors from Step 2. This is the **precoloring extension** problem.

```
// Pseudocode
result = backtracking_color(G, available_colors={1..15}, precoloring=prefilled_assignments)
if result == UNSAT:
    report which constraints conflict
    fall back to greedy or report error
```

### Integration Points

**1. Replaces the greedy Indirection pipeline**

The optimal alignment solver replaces the `build_indirect_links()` + second `build_all_output_palettes()`
call sequence in `palette_packer.cpp`. Specifically, when `TileSharingAlignment::optimal`:

- Skip `build_indirect_links()` entirely (no `IndirectLink` generation)
- Skip the second `build_all_output_palettes()` call with links
- Instead, run the CSP solver (Steps 1-4 above) against the base palettes + shape groups
- The solver outputs a direct `(palette_index, color) -> slot` assignment map
- Use this map to construct final palettes directly (a new code path, simpler than the 5-phase builder)

**2. Input requirements**

The solver needs:
- Shape groups (from `analyze_shape_groups()`)
- Tile-to-palette assignments (from `PackingOutput::tile_to_pal_`)
- Packed palette color sets (from `PackingOutput::pals_`)
- Prefilled palettes (from `PackingParams::prefilled_pals_`)

All of these are already available at the point in `palette_packer.cpp` where the alignment panic
currently lives (line 284).

**3. Output format**

The solver produces a `std::map<std::pair<std::size_t, Rgba32>, std::size_t>` mapping
`(palette_index, color) -> slot_index`. This replaces the `ColorPosition` state machine
(`Undetermined -> Indirect -> Absolute`) used by the greedy pipeline. The final palette construction
is a simple loop:

```
for each (palette_index, color), slot in assignment:
    output_palette[palette_index].set(slot, color)
```

**4. Where to put the new code**

- **Union-find**: `Porytiles2/include/porytiles2/utilities/union_find.hpp` (new file, generic utility)
- **CSP solver**: `Porytiles2/include/porytiles2/domain/packing/algorithms/optimal_alignment_solver.hpp`
  and corresponding `.cpp` in `Porytiles2/lib/domain/packing/algorithms/`
- **Integration**: new branch in `palette_packer.cpp` alongside the existing greedy path

### Complexity

The precoloring extension problem is NP-complete in general. However, for GBA tilesets the problem
instance is tiny:

- **Nodes**: at most ~200 equivalence classes (6-13 palettes x 15 colors, minus merges)
- **Max clique size**: <= 15 (all classes within a single palette form a clique)
- **Available colors**: 15 (slot indices)

Backtracking with arc consistency (AC-3) solves this in microseconds for these sizes.

### Is 15-colorability Guaranteed Given Valid Packing?

**No.** Equivalence class merging can create cross-palette edges that raise node degree above 14.
A graph with max clique 15 but chromatic number > 15 is theoretically possible (odd holes can push
chromatic number above clique number — the Strong Perfect Graph Theorem characterizes exactly when
this happens).

**In practice**: for real GBA tilesets with 6-13 palettes and 8-12 colors each, 15-colorability
should virtually always hold. The conflict graph is highly structured (union of small cliques with
sparse cross-edges), and 15 colors is generous relative to the clique sizes encountered in practice.

### Diagnostics for Alignment Failure (UNSAT)

When the 15-coloring fails:

1. **Identify the conflicting core**: extract the minimal subgraph that is not 15-colorable (the
   failing constraint set). Report which shape groups contributed edges to this subgraph.
2. **Suggest deprioritization**: recommend which shape groups to exclude from sharing to make the
   remaining constraints satisfiable. Prioritize excluding groups with the fewest members (least
   sharing benefit).
3. **Prefilled conflict detection**: if Step 2 detects contradictory prefilled slots within an
   equivalence class, report the specific prefilled palettes and slots involved — the user may be
   able to rearrange their prefilled palette layout.
4. **Fall back gracefully**: offer to run `greedy` alignment as a fallback, which will achieve
   partial sharing even when full sharing is impossible.


## Joint Optimization (Coupling Between Dimensions)

The two dimensions are **configurationally independent** (any packing strategy can be paired with any
alignment strategy) but **causally dependent** at runtime: the packing output determines the conflict
graph that alignment must solve.

### A Valid Packing Can Produce Unsolvable Alignment

Even with all shape group siblings in different palettes, cross-group interactions can create
alignment conflicts:

```
Example:
- Shape group 1 links (pal 0, red) <-> (pal 1, blue)     -> same equivalence class, must share a slot
- Shape group 2 links (pal 0, red) <-> (pal 2, green)    -> merges into same class: {red, blue, green}
- Shape group 3 links (pal 1, yellow) <-> (pal 2, green)  -> merges yellow into the class too
- Now {red, blue, green, yellow} all need the same slot
- But pal 0 has both red and another color from a different class that is also forced to that slot
  via similar transitive chains -> conflict
```

### Joint Backtracking Approach

For a true guarantee that all sharing is exploited, solve packing and alignment jointly:

1. Try a packing (via `BacktrackingStrategy` with hard sibling rejection)
2. Build the conflict graph from the resulting palette assignments
3. Attempt 15-coloring
4. If coloring fails, backtrack to a different packing
5. If no packing produces a solvable alignment, report the maximum-sharing subset

This is expensive in theory (NP-hard search with NP-complete subproblem at each node), but feasible
in practice for GBA sizes. The packing search space is small (6-13 palettes, <=15 colors each), and
the coloring subproblem is microsecond-scale.

### Practical Recommendation

For most tilesets, the two-phase approach (solve packing first, then alignment) will find a valid
solution. Joint backtracking should be reserved for cases where two-phase fails, as a diagnostic
fallback rather than the default mode. The implementation order should be:

1. Implement optimal packing (hard rejection in backtracking) — standalone value
2. Implement optimal alignment (CSP solver) — standalone value
3. Wire joint backtracking as a fallback when `(optimal, optimal)` two-phase fails


## Critical Files

Files relevant to implementing optimal packing and alignment:

| File                                                                                | Role                                                                                   |
|-------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------|
| `Porytiles2/lib/domain/packing/services/palette_packer.cpp`                         | Orchestration — panic sites at lines 281-286, integration point for both optimal modes |
| `Porytiles2/include/porytiles2/domain/packing/services/packing_strategy.hpp`        | `PackingInput` struct — needs mode signal for optimal vs biased                        |
| `Porytiles2/include/porytiles2/domain/packing/services/backtracking_strategy.hpp`   | `BacktrackingStrategy` — add hard-reject pruning for optimal packing                   |
| `Porytiles2/include/porytiles2/domain/packing/models/shape_group_metadata.hpp`      | `ShapeGroupMetadata` — tile-to-group mapping data struct                               |
| `Porytiles2/include/porytiles2/domain/packing/algorithms/sharing_metrics.hpp`       | `palette_contains_sibling()` free function — used by hard-reject pruning               |
| `Porytiles2/include/porytiles2/domain/packing/algorithms/palette_builder.hpp`       | `build_all_output_palettes()` — bypassed by optimal alignment solver                   |
| `Porytiles2/include/porytiles2/domain/packing/algorithms/indirect_link_builder.hpp` | `build_indirect_links()` — bypassed by optimal alignment solver                        |
| `Porytiles2/include/porytiles2/domain/models/shape_group.hpp`                       | `ShapeGroup`, `ShapeGroupMember` — input to both solvers                               |
| `Porytiles2/include/porytiles2/domain/config/tile_sharing_packing.hpp`              | `TileSharingPacking` enum (auto-generated)                                             |
| `Porytiles2/include/porytiles2/domain/config/tile_sharing_alignment.hpp`            | `TileSharingAlignment` enum (auto-generated)                                           |
| `Porytiles2/lib/domain/services/primary_tileset_compiler.cpp`                       | Config unwrap — strategy override logic for optimal packing goes here                  |

### New Files to Create

| File                                                                                   | Role                                                    |
|----------------------------------------------------------------------------------------|---------------------------------------------------------|
| `Porytiles2/include/porytiles2/utilities/union_find.hpp`                               | Generic union-find (disjoint-set) data structure        |
| `Porytiles2/include/porytiles2/domain/packing/algorithms/optimal_alignment_solver.hpp` | CSP solver: union-find -> conflict graph -> 15-coloring |
| `Porytiles2/lib/domain/packing/algorithms/optimal_alignment_solver.cpp`                | CSP solver implementation                               |
