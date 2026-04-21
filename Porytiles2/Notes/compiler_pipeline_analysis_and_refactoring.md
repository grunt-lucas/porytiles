# TilesetCompiler: Logical Flow & Refactoring Analysis

## Context

`Porytiles2/lib/domain/services/tileset_compiler.cpp` (currently ~2373 lines)
contains a `CompilerTask` class that compiles Porytiles-format tileset assets
into Porymap-format output. The class was previously named
`PrimaryTilesetCompiler`, but it was unified with the secondary compiler: a
single `TilesetCompiler::compile(tileset, is_secondary, paired_primary, ...)`
entry point now handles both roles by threading `is_secondary_` and
`paired_primary_` through the same pipeline.

The logic branches on several mode axes:

- `tiles_edit_mode` — optimize / patch / locked
- `pals_edit_mode` — optimize / patch / locked
- `global_frame_linking` (+ per-anim overrides) — automatic / manual
- `is_secondary` (with an optional `paired_primary` pointer) — primary vs secondary
- `tiles_pal_mode` — regular / true_color
- `cross_tileset_anim_linking` — on / off (secondary-only)
- Packing strategy + per-strategy params (best_fusion / backtracking / overload_and_remove)
- Per-animation resolution strategies (pal, keyframe, multi-pal subtile, linking)

This document traces the complete pipeline flow broken out by mode, covers the
two new cross-cutting concerns (diagnostic emission and cross-tileset sharing),
and proposes refactoring strategies.

---

## Part 1: Complete Pipeline Flow

Pipeline entry point: `CompilerTask::run()` at line 259. It unwraps config
values, resolves the paired primary's extrinsic transparency if applicable
(lines 314-326), and executes the six steps in order (lines 329-339).

### Step 1: `pipeline_step_process_porytiles_input()` (line 342) — no mode branching
- Metatileize Porytiles bottom/middle/top layer images
- Decompose metatiles into a flat pixel tile vector
- Generate canonical versions of each tile

### Step 2: `pipeline_step_process_porymap_input()` (line 364) — no mode branching
- Triple-layerize the existing Porymap component
- Decompile tilemap entries + tiles.png + pals into a metatile vector
- Decompose and canonicalize, just like Step 1

### Step 3: `pipeline_step_validate_input()` (line 399) — secondary-aware

| Check                                                  | primary |        secondary         |
|--------------------------------------------------------|:-------:|:------------------------:|
| Validate metatile count                                |   Yes   |   Yes (with secondary-aware limit)   |
| Validate paired primary's Porymap pals (import-time)   |   No    |   **Yes** (lines 421-430) |
| Validate this tileset's Porymap pals                   | Yes (pals != optimize) | Yes (pals != optimize, from `pal_start`) |
| Reject Porytiles overrides in primary pal slots        |   No    |   **Yes** (lines 445-454) |
| Validate Porytiles pals                                |   Yes   |  Yes (from `pal_start`)  |
| Validate palette hints / alpha / layer mode            |   Yes   |           Yes            |
| Validate tile / global color count                     |   Yes   |   Yes (secondary-aware)  |
| Validate precision loss, anim frames                   |   Yes   |           Yes            |

Two import-time checks were added specifically so secondary compilation surfaces
primary-induced errors with a clear source instead of panicking mid-packing:

1. **Primary Porymap palette validation** (lines 414-431): Primary palettes are
   preloaded into the packer as locked slots, so any violation of the ET
   invariant in non-slot-0 positions would crash the packer. Running
   `validate_porymap_pal` against the primary's palettes first turns that
   crash into a diagnostic scoped to the *primary's* name.
2. **Primary-slot override rejection** (lines 443-454): A secondary tileset may
   not define a Porytiles override palette in any primary slot. This returns a
   `FormattableError` (not a panic) with the offending slot's filename.

### Step 4: `pipeline_step_setup_working_data()` (line 517) — heavy branching

#### 4a: Secondary panic gates (lines 519-525)

Before any working data is constructed, secondary compilation is rejected for
any non-optimize mode:

```
if (is_secondary() && tiles_edit_mode_ != optimize) panic(...);
if (is_secondary() && pals_edit_mode_ != optimize) panic(...);
```

These are internal panics today. Suggestion 8 below proposes elevating them to
user-visible validated errors emitted in Step 3.

#### 4b: Palette creation (pals_edit_mode axis)

|               pals:optimize                |         pals:locked          |    pals:patch     |
|:------------------------------------------:|:----------------------------:|:-----------------:|
| Call `pipeline_helper_run_pal_packing()`   | Copy all 16 pals from Porymap | **panic (line 535)** |

`pipeline_helper_run_pal_packing()` (line 1045) is now significantly richer:

- Computes `color_count_limit` differently for secondary (secondary-slot budget
  only, `(num_pals_total - num_pals_in_primary) * 15`) vs primary (`num_pals_in_primary * 15`).
- Selects a packing strategy via `make_packing_strategy(packing_strategy, packing_strategy_params, diag_)`.
  Current strategies: `BestFusionStrategy`, `BacktrackingStrategy`,
  `OverloadAndRemoveStrategy`. Each receives the diagnostic sink so it can
  emit remarks about preset / parameter selection.
- Builds `available_pals` bitset: for secondary with paired primary, enables
  primary slots (so the packer can assign secondary tiles to primary palettes
  without modifying them); always enables secondary slots.
- Builds `prefilled_pals`: for secondary with paired primary, locks primary
  palettes from the compiled paired primary (lines 1091-1105); for primary,
  carries over Porytiles override palettes.
- For secondary with paired primary, reconstructs primary RGBA tiles from the
  primary's Porymap tiles.png and normalizes them to triple-layer so the
  cross-tileset shape-group analysis has canonical inputs (lines 1115-1167).
- After packing, `tile_to_pal_` is populated and `new_porymap_pals_` is
  filled, resolving any out-of-band Porytiles overrides (wildcards -> black
  or extrinsic transparency).

`pipeline_helper_build_color_index_map()` (line 1226) was extracted as a named
helper during this refactor; it assembles the color-index map from Porytiles
tiles, anims, pals, and hints. For secondary compilations it also folds in
the primary palette colors after the budget check.

#### 4c: TilesPngWorkspace creation (tiles_edit_mode x is_secondary axes)

|    tiles:optimize (primary)    | tiles:optimize (secondary, paired) | tiles:optimize (secondary, standalone) |
|:------------------------------:|:----------------------------------:|:--------------------------------------:|
| Empty workspace, capacity = `num_tiles_in_primary` | `TilesPngWorkspace::for_secondary(primary_tiles_png, num_in_primary, num_total)` — primary tiles pre-loaded | `TilesPngWorkspace::for_standalone_secondary(num_in_primary, num_total)` — transparent space reserved for primary range |

Non-optimize (primary only — secondary cannot reach this due to panic gate):

|          tiles:patch           |          tiles:locked           |
|:------------------------------:|:-------------------------------:|
| Loaded from existing tiles.png, capacity = `num_tiles_in_primary` | Loaded from existing tiles.png, capacity = exact `size_in_tiles` |

#### 4d: Animation registration (tiles_edit_mode x FrameLinking x is_secondary)

Done by `pipeline_helper_register_animations()` (line 1380), which calls
`pipeline_helper_build_keyframe_data()` (line 1280) per animation to assemble
palette-matched composite tiles converted to IndexPixel form.

Effective FrameLinking per animation: checked against `per_anim_overrides_`
first, then falls back to `global_frame_linking_`. The same resolution is
duplicated in `pipeline_helper_apply_manual_overrides()` — a DRY violation
noted below (Suggestion 5).

**Pre-loop (optimize only):** `reserve_anim_slots(total)`.

**Per-animation placement (FrameLinking x tiles_edit_mode):**

FrameLinking::automatic (all edit modes use normal search/placement):
- optimize: Sequential `place_anim_tile` at reserved offsets
- patch: Try `find_existing_contiguous_tiles_by_color`, else
  `find_contiguous_transparent_slots` + `place_tiles_at`
- locked: Must `find_existing_contiguous_tiles_by_color` or error

FrameLinking::manual:
- optimize: Same as automatic (tiles.png still needs keyframes for DMA)
- patch/locked: Read `tile_offset` from anim.json (no tile searching/placement)

**Secondary cross-tileset path (when `cross_tileset_anim_linking_` is true):**
The `AnimTileMatcher` is extended with an `is_cross_tileset` flag so a
secondary can register *primary* animations against its own tile space. This
lets a secondary reuse primary animation tiles rather than claiming additional
slots. See Part 2.6 below for the full story.

**Post-loop:** Register all animations with `AnimTileMatcher` (all modes,
both FrameLinking values).

### Step 5: `pipeline_step_match_tiles_pals()` (line 590) — heavy branching

Pre-check: pals:patch -> panic (line 594, same reason as Step 4).

For each Porytiles tile, in order:

```
for each porytiles tile [i]:
    [tiles != optimize] try pipeline_helper_try_reuse_porymap_tile(i)
        Check exact match, then canonical match at same index.
        If found -> emit reused TilemapEntry, continue.

    [all modes] if transparent -> emit TilemapEntry{0,0,false,false}, continue.

    Call pipeline_helper_assign_tile_via_pal_match(tile, i):
        Match tile against new_porymap_pals_ -> find covering palette.
            No covering pal:
              [pals:optimize] -> panic (should have failed at packing)
              [pals:locked]   -> no_covering_pal result; emit_no_matching_pal_error

        Convert to IndexPixel tile using the matched palette.

        Anim matcher gating:
          [tiles:optimize]       -> always check anim matcher
          [tiles:locked/patch]   -> only check if original tile_index is
                                    in an animation range

        If anim matcher matches -> use animation tile_index + pal_index.

        Tile lookup in workspace:
          [tiles:optimize]       -> first_occurrence_of (exact, O(1))
          [tiles:locked/patch]   -> first_occurrence_of_by_color (O(n))
          If found -> compute flip bits, return success.

        Tile not found:
          [tiles:locked]         -> tile_not_found; emit_no_matching_tile_error
          [tiles:optimize/patch] -> check capacity; insert if room,
                                    else tile_limit_reached;
                                    emit_tile_limit_error with secondary-aware limit
```

For secondary, the user-visible tile-limit shown in the diagnostic is
`num_tiles_total - num_tiles_in_primary` rather than `num_tiles_in_primary`
(lines 653-656).

After the loop, on primary compilations the compiler calls
`pipeline_helper_validate_primary_anim_subtile_coverage()` (line 677) as a
defense-in-depth pass. Rationale: catch unreferenced non-transparent animation
subtiles at *primary* compile time, so a paired secondary does not later fail
with a confusing "primary references missing subtile" error. Secondary
compilation relies on the same check being run during primary compilation.

### Step 6: `pipeline_step_assemble_output()` (line 683)

The compiler produces a `Tileset` holding a new `PorymapTilesetComponent` and
a cloned `PorytilesTilesetComponent` with updated round-trip metadata.

**Round-trip metadata writeback** (lines 703-712): For each animation in the
cloned Porytiles component, if the matcher computed a tile_offset for that
animation, it is written back into the anim params so it persists to
anim.json. For secondary compilations, the stored offset is local
(`offset - num_tiles_in_primary`) rather than the global offset used during matching.

Compilation then runs:
- `pipeline_helper_compile_animations()` (line 1782) — finalize animations
- `pipeline_helper_apply_manual_overrides()` (line 1880) — apply manual
  FrameLinking overrides to metatiles_bin before dual-layerization. Emits
  warnings `multi-palette-animation`, `automatic-mode-overrides-ignored`, and
  `manual-no-overrides`. Emits five `primary-references-*` errors when a
  secondary references primary metatiles that don't exist or aren't reachable.
- Dual-layer conversion (when `num_tiles_per_metatile == dual`)
- Copy metatile attributes from original, inferring LayerType for dual-layer

**tiles.png export** (lines 754-774):

|                   | tiles:optimize (primary)          | tiles:optimize (secondary) | tiles:locked/patch             |
|-------------------|-----------------------------------|----------------------------|--------------------------------|
| Export call       | `export_image`                    | `export_secondary_image(num_tiles_in_primary, ...)` | `export_image`                |
| Trim mode         | `trim_trailing_transparent`       | `trim_trailing_transparent` | `include_trailing_transparent` |

`export_secondary_image` skips over the pre-loaded primary range so the
output PNG only contains this tileset's own tiles.

**Palette copy to output** (lines 776-804):

- Primary: copy all 16 new_porymap_pals_ into the output component.
- Secondary (paired): copy primary's Porymap palettes into slots
  `[0, num_pals_in_primary)`, new_porymap_pals_ into the secondary range, and
  the original secondary's junk/reserved pals into `[num_pals_total, num_pals)`.
- Secondary (standalone): zeroed palettes for primary slots; rest as above.

**True-color mode** (lines 806-809): If `tiles_pal_mode_ == true_color`, the
compiler calls `pipeline_helper_apply_true_color_to_tiles_png()` (line 2032).
This helper rewrites tiles.png pixels so each pixel's color index encodes both
its palette slot and its intra-palette color index
(`(pal_idx << 4) | color_idx`), then rebuilds an 8-bit PNG palette that
concatenates all in-scope palettes. During this pass it can emit three
remarks: `true-color-anim-only-tile`, `true-color-multi-palette-tile`,
`true-color-unreferenced-tile`.

### Extracted diagnostic helpers

Three previously-inline error paths are now named helpers:

- `pipeline_helper_emit_no_matching_tile_error` (line 2258) — emits
  `no-matching-tile` error plus notes for matched palette and generated
  index tile.
- `pipeline_helper_emit_no_matching_pal_error` (line 2289) — emits
  `no-matching-palette` error plus a long note with the top-N closest
  match candidates.
- `pipeline_helper_emit_tile_limit_error` (line 2332) — emits `tile-limit`
  error.

Each helper owns its tag name and note composition; Step 5 just dispatches to
them on `TileAssignmentResult::Status` values.

---

## Part 2: Mode Combination Matrix

### Primary compilations (is_secondary = false)

|                    |              pals:optimize              |                 pals:locked                 | pals:patch |
|--------------------|:---------------------------------------:|:-------------------------------------------:|:----------:|
| **tiles:optimize** |  **Full recompile** (Porytiles1 parity) |   New tiles from scratch, keep existing pals   |    TODO    |
| **tiles:patch**    |  Fresh pals, insert new tiles into existing  | Keep existing pals, insert new tiles into existing |    TODO    |
| **tiles:locked**   | **BUGGY** (pals change but tiles can't) |        **Verify mode** (no changes)         |    TODO    |

### Secondary compilations (is_secondary = true)

Only `optimize + optimize` is reachable today. Everything else panics at the
Step 4 gate (lines 521, 524):

|                    | pals:optimize | pals:locked | pals:patch |
|--------------------|:-------------:|:-----------:|:----------:|
| **tiles:optimize** |  Supported    | panic (524) |   panic    |
| **tiles:patch**    |  panic (521)  | panic (521) |   panic    |
| **tiles:locked**   |  panic (521)  | panic (521) |   panic    |

### Known-bad / invalid combinations

- **pals:optimize + tiles:locked** (primary): if palettes are re-optimized but
  tiles are locked, the compiler emits identical metatile entries but with new
  palette assignments, producing tilemap entries that reference palette slots
  not matching the locked tile data. The `run()` prologue (line 262) has a
  long TODO explaining why this combo should simply be banned. Tiles
  fundamentally depend on palettes.
- **is_secondary + anything except optimize/optimize**: hard panic. These
  should become user-visible `FormattableError`s emitted in Step 3. See
  Suggestion 8.
- **pals:patch**: unimplemented; panics at both Step 4 (line 535) and Step 5
  (line 594).

---

## Part 2.5: Diagnostic Emission Infrastructure

The compiler writes to a `UserDiagnostics` sink (`diag_` member) with four
severities: `error` + `error_note`, `warning` + `warning_note`, `remark`,
and (implicitly) `info`. Each call passes a **tag** string and a vector of
formatted lines. Tags are the stable machine-readable identifier for
filtering, suppression, and documentation.

Current tag inventory (observed in `tileset_compiler.cpp`):

| Severity | Tag                                   | Emitted where                                     | Meaning |
|----------|---------------------------------------|---------------------------------------------------|---------|
| remark   | `cross-tileset-anim-match`            | line 925  | Secondary tile matched a primary anim subtile |
| remark   | `cross-tileset-anim-skip-no-keyframe` | line 1595 | Primary anim had no usable keyframe for cross-tileset matching |
| remark   | `cross-tileset-anim-rgba-fallback`    | line 1685 | Secondary anim subtile resolved via RGBA match against a primary pal |
| remark   | `true-color-anim-only-tile`           | line 2120 | Tile referenced only by an animation, not by a metatile |
| remark   | `true-color-multi-palette-tile`       | line 2172 | Tile used with multiple palettes in true-color output |
| remark   | `true-color-unreferenced-tile`        | line 2211 | Tile not referenced by any metatile or animation |
| warning  | `cross-tileset-anim-fallthrough`      | line 991  | Tile expected to match primary anim was matched locally instead |
| warning  | `cross-tileset-anim-fallthrough-disabled` | line 1004 | Similar fallthrough case when cross-tileset linking disabled |
| warning  | `primary-anim-unreferenced-subtile`   | line 1774 | Primary animation subtile never referenced by any primary metatile |
| warning  | `multi-palette-animation`             | line 1835 | Single animation spans more than one palette |
| warning  | `automatic-mode-overrides-ignored`    | line 1906 | Automatic mode present; per-anim overrides silently ignored |
| warning  | `manual-no-overrides`                 | line 1919 | Manual mode but no per-anim overrides provided |
| error    | `primary-references-on-primary`       | line 1966 | Primary metatile tried to reference primary metatiles |
| error    | `primary-references-no-paired-primary`| line 1976 | Secondary references primary metatiles but no primary paired |
| error    | `primary-references-anim-not-found`   | line 1990 | Primary-reference override points at missing animation |
| error    | `primary-references-frame-subtile-oob`| line 2006 | Primary-reference override has out-of-range frame/subtile |
| error    | `primary-references-metatile-oob`     | line 2021 | Primary-reference override points at out-of-range metatile |
| error    | `no-matching-tile`                    | line 2274 | Locked-tile mode: no workspace tile matched this Porytiles tile |
| error    | `no-matching-palette`                 | line 2302 | Locked-pal mode: no palette covers this tile's colors |
| error    | `tile-limit`                          | line 2345 | Would exceed tile capacity (primary or secondary limit) |

Notes/observations:

- Diagnostics are a genuine second output channel of the compiler. Together
  with the Porytiles round-trip metadata they make the compiler's output
  shape "Porymap artifacts + Porytiles metadata + diagnostic stream".
- Tag discovery is currently ad-hoc: consumers must grep the code. A
  refactoring (Suggestion 6) could own tag constants in one place.
- Several call sites build a `remark_lines` / `warning_lines` vector inline,
  then format lines with `format_.format(...)`, then pass the vector to
  `diag_.X(tag, lines)`. This pattern accounts for most of the ~200 LOC of
  diagnostic plumbing scattered through the class.

---

## Part 2.6: Cross-Tileset Tile & Animation Sharing

Config axis: `cross_tileset_anim_linking_` (boolean, secondary-only). When
enabled, a secondary tileset may reuse primary animation tiles rather than
claiming new slots.

Mechanics:

1. **Workspace prefill** (Step 4c, line 562): for paired secondary +
   tiles:optimize, `TilesPngWorkspace::for_secondary` pre-loads the primary's
   tiles.png into the primary-slot range. Secondary tiles are assigned at
   global indices `>= num_tiles_in_primary`.
2. **Packer prefill** (Step 4b, lines 1091-1105): primary palettes are fully
   locked as `prefilled_pals` so the packer cannot add colors to them but
   can still assign secondary tiles to them when the tile's colors are a
   subset of a locked primary palette.
3. **Color-index map extension** (`pipeline_helper_build_color_index_map`):
   after budget validation, primary palette colors are folded in so packing
   accounts for them.
4. **Cross-tileset shape-group analysis** (lines 1115-1167): primary tiles are
   reconstructed from the primary's compiled component (indexed tiles plus
   triple-layerized entries) into canonical RGBA form, deduplicated by
   `(tile_index, pal_index)`, and handed to the packer as `primary_tiles_`
   for shape-group awareness.
5. **Cross-tileset animation matching** (`pipeline_helper_register_animations`):
   `AnimTileMatcher` carries an `is_cross_tileset` flag per match. Primary
   animations registered for a secondary compile are tagged as cross-tileset,
   which makes their matches emit the `cross-tileset-anim-*` remark family
   instead of normal matches.

Three remarks and two warnings track this code path:

- `cross-tileset-anim-match` (remark) — a secondary tile did match a primary
  animation subtile; the remark cites the matched anim and subtile index.
- `cross-tileset-anim-skip-no-keyframe` (remark) — primary animation has no
  usable keyframe (manual linking, likely), so cross-tileset matching against
  it is skipped.
- `cross-tileset-anim-rgba-fallback` (remark) — a secondary animation subtile
  was not referenced by any primary metatile but its colors matched a primary
  palette, so the compiler falls back to RGBA-based resolution.
- `cross-tileset-anim-fallthrough`, `cross-tileset-anim-fallthrough-disabled`
  (warnings) — a tile the compiler expected to resolve via cross-tileset
  matching was instead matched locally (or would have been matched but
  linking is disabled).

---

## Part 3: Refactoring Suggestions

### Suggestion 1: Ban the Invalid Combo (Quick Win)

Add validation at the top of `run()` to reject `pals:optimize + tiles:locked`
with a clear error message. Reduces the working matrix and removes a known
corruption vector. Still the lowest-risk, highest-value change.

### Suggestion 2: Strategy Pattern for Tile Placement (Recommended)

Extract tile-mode branching into a **TilePlacementStrategy** interface:

```
TilePlacementStrategy (interface)
    create_workspace(tileset, capacity, is_secondary, paired_primary) -> TilesPngWorkspace
    reserve_anim_slots(workspace, total_tiles) -> void                     [optimize only]
    place_anim_keyframes(workspace, keyframe_data) -> offset
    try_reuse_tile(tile_index, ...) -> optional<TilemapEntry>
    lookup_tile(canonical_tile, workspace, palette) -> optional<size_t>
    handle_tile_not_found(canonical_tile, workspace) -> TileAssignmentResult
    export_image(workspace) -> Image

OptimizeTilePlacement   — empty workspace, exact lookup, insert freely, reserve anim slots
PatchTilePlacement      — loaded workspace, color-equiv lookup, insert into free space
LockedTilePlacement     — loaded workspace, color-equiv lookup, error on miss
```

This eliminates ~60% of the if/else branching. The pipeline skeleton stays in
`CompilerTask` but delegates at each branch point.

**New since this doc was first written:**

- `create_workspace` now has three primary-vs-secondary variants
  (`TilesPngWorkspace` default ctor, `for_secondary`, `for_standalone_secondary`).
  A strategy lifts this decision out of Step 4c.
- `export_image` has a primary/secondary variant (`export_image` vs
  `export_secondary_image(num_tiles_in_primary, ...)`). The strategy owns this.
- `handle_tile_not_found` now needs to compute a secondary-aware user-visible
  tile limit; today this lives inline at lines 653-656.

**FrameLinking interaction:** The `FrameLinking x tiles_edit_mode` branching
in animation registration (Step 4d) makes this even more compelling. The
pipeline orchestrator would handle FrameLinking as a pre-filter:
- manual + non-optimize: Read tile_offset from anim.json directly, bypass strategy
- all other cases: Delegate to `strategy.place_anim_keyframes()`

This keeps FrameLinking routing in the pipeline skeleton (where it belongs —
it's about *whether* to interact with tiles.png) and tile placement mechanics
in the strategy (where they belong — *how* to interact with tiles.png).
Without strategies, the current code nests FrameLinking checks inside
tiles_edit_mode checks, creating a 2x3 branch matrix in a single function.

**DRY opportunity:** Effective FrameLinking resolution (per_anim_overrides
lookup + global fallback) is currently duplicated in both
`pipeline_helper_register_animations` and `pipeline_helper_apply_manual_overrides`.
With the strategy pattern or explicit data flow (Suggestion 5),
`effective_linking` could be computed once per animation and threaded through
as part of the working data.

### Suggestion 3: Strategy Pattern for Palette Setup (Complementary)

Extract palette-mode branching into a **PaletteSetupStrategy** interface:

```
PaletteSetupStrategy (interface)
    create_palettes(tileset, paired_primary, is_secondary, ...) -> array<Palette, 16>
    color_count_limit(is_secondary) -> size_t
    build_available_pals_bitset(is_secondary, paired_primary) -> bitset
    build_prefilled_pals(is_secondary, paired_primary) -> array<optional<Palette>, 16>
    should_validate_porymap_pals() -> bool

OptimizePaletteSetup   — full packing algorithm
LockedPaletteSetup     — copy existing palettes
PatchPaletteSetup      — (future) incremental editing
```

**New since this doc was first written:** the optimize variant now carries
three new concerns that benefit from being pulled into a dedicated object:

- Secondary-aware color budget (lines 1051-1053)
- Primary-pal prefill (lines 1091-1105)
- Cross-tileset shape-group tile reconstruction (lines 1115-1167)

These are all "how the packer knows about the paired primary". A strategy
object owns them instead of 100+ lines of branching inside
`pipeline_helper_run_pal_packing`.

### Suggestion 4: Primary vs Secondary Compiler Split (Revised)

The original doc proposed a Fresh vs Incremental split. That framing is less
compelling now; the more natural axis the code has grown is
**Primary vs Secondary**. The two differ materially in:

- Step 3: different validations run (paired primary pal check, secondary
  override rejection)
- Step 4: different workspace construction, different packer prefill, panic
  gate for non-optimize modes
- Step 5: different tile-limit in diagnostics; defense-in-depth primary-anim
  coverage check primary-only
- Step 6: different export call, different palette-copy layout, different
  round-trip offset arithmetic

Pros of a split:
- Each compiler is conceptually simpler; secondary-specific state
  (`paired_primary_`, `paired_primary_extrinsic_transparency_`,
  `anim_pal_indices_`, primary-pal-prefill plumbing) stops leaking into the
  primary code path.
- Easier to reason about invariants within each compiler.
- No need for an `is_secondary()` check inside every helper.

Cons:
- Significant duplication risk: the six-step skeleton and most helpers are
  still shared.
- Tile/pal edit-mode branching still exists inside each.
- Two compilers + strategy pattern (Suggestion 2) yields a 2-dim factoring
  cost.

A reasonable middle ground is to keep one `CompilerTask` but introduce a
**CompilationRole** object that owns the primary-vs-secondary variations
(workspace factory, export mode, palette copy layout, pal budget). This
composes cleanly with the TilePlacementStrategy and PaletteSetupStrategy
proposals.

### Suggestion 5: Explicit Data Flow Between Steps (Longer-Term)

Replace the now ~25 shared member variables with explicit return types between
pipeline steps:

```
ProcessedPorytiles  <- step 1
ProcessedPorymap    <- step 2
(validation)        <- step 3
WorkingData         <- step 4 (palettes, workspace, matcher, resolved
                               effective_linkings, pal index map, prefilled pals,
                               paired_primary_et)
MatchedTilemap      <- step 5
OutputTileset       <- step 6 (Porymap artifacts + updated Porytiles metadata
                               + diagnostic totals)
```

Each step becomes a quasi-pure function. Benefits: testability, readability,
no implicit coupling. This is orthogonal to the strategy pattern — they
compose well together.

**New motivation since this doc was first written:**

- The class now has ~25 shared member variables, including eight new
  `ConfigValue<...>` fields (`tiles_pal_mode_`, `per_anim_overrides_`,
  `cross_tileset_anim_linking_`, `paired_primary_extrinsic_transparency_`,
  `global_frame_linking_`, plus the existing set) and
  `anim_pal_indices_` (map from anim name to pal indices).
- `anim_tile_matcher_` still bridges Step 4 and Step 6 implicitly to carry
  tile_offset writeback.
- The compiler has grown **three** output channels, not two: Porymap
  artifacts, Porytiles round-trip metadata, and the diagnostic stream.
  `OutputTileset` should name all three.
- Per-animation effective FrameLinking is still computed identically in two
  helpers. Threading it through `WorkingData` eliminates that DRY violation.

### Suggestion 6 (NEW): Extract a `CompilerDiagnosticsReporter`

The three `pipeline_helper_emit_*_error` helpers are a partial step in this
direction: they own tag names and note composition for their respective
errors. Generalize: a dedicated `CompilerDiagnosticsReporter` collaborator
that:

- Owns constants for all ~20 diagnostic tags (central registry, one source
  of truth).
- Exposes a typed method per diagnostic (e.g.
  `report_cross_tileset_anim_match(anim_name, subtile, ...)`) instead of
  requiring callers to build `remark_lines` vectors by hand.
- Internally delegates to `UserDiagnostics` while handling the common shape
  (header line + optional tile viz + notes).

Benefits:
- Removes ~200 LOC of inline `format_.format(...)` + vector building from
  `CompilerTask`.
- Makes the tag inventory discoverable (just read the reporter's public API).
- Natural place to add metrics (diagnostic counts), rate-limiting, or
  structured output (JSON) in the future.

Composes cleanly with Suggestions 2/3: strategies inject the reporter instead
of receiving a raw `UserDiagnostics &`.

### Suggestion 7 (NEW): Policy Types for Resolution Strategies

`PerAnimOverride` already carries four enum-valued policy axes:

- `AnimPalResolutionStrategy`
- `AnimKeyFrameResolutionStrategy` (error / warning / mangle)
- `AnimMultiPalSubtileResolutionStrategy`
- `FrameLinking` (automatic / manual / hybrid-TODO)

Each enum variant has distinct behavior that is currently implemented via
switch statements inside `pipeline_helper_register_animations` and
`pipeline_helper_apply_manual_overrides`. This is a textbook Policy pattern:
one small class per variant with a uniform `apply(...)` method. Benefits:

- Adding a new variant stops being "grep for every switch and add a case".
- Each policy's contract is explicit in its type, not implicit in how the
  switch is structured.
- Testing each policy in isolation becomes easy.

The three-tier pal resolution cascade (per-tile -> per-anim -> global) becomes
a natural `CompositeResolutionPolicy` that chains single-tier policies.

### Suggestion 8 (NEW): Promote Panics to Validated Errors

Several mode combinations currently panic from deep inside the pipeline:

- `is_secondary + tiles_edit_mode != optimize` (line 521)
- `is_secondary + pals_edit_mode != optimize` (line 524)
- `pals_edit_mode == patch` (lines 535 and 594)
- `pals:optimize + tiles:locked` corruption (effectively; see Suggestion 1)

All of these are deterministically known at Step 3. Each should become a
`FormattableError` emitted during `pipeline_step_validate_input` (or even
earlier, at config load), following the error style in STYLE.md:

```c++
return FormattableError{
    "Secondary compilation of tileset '{}' does not support "
    "tiles ArtifactEditMode::{} (only 'optimize' is supported).",
    FormatParam{tileset_.name(), Style::bold},
    FormatParam{to_string(tiles_edit_mode_.value()), Style::bold}};
```

Benefits:
- Users see a proper diagnostic, not a crash.
- Validation becomes the single place that describes which combos are
  supported (matches the matrix in Part 2).
- Removes panics from the hot compilation path, improving reliability.

---

## Part 4: Recommended Approach

**Priority order:**

1. **Ban invalid combos (umbrella task):** Suggestion 1 + Suggestion 8.
   Trivial edits. Covers `pals:optimize + tiles:locked`, `is_secondary +
   non-optimize`, and `pals:patch`. Eliminates one known corruption vector
   and three panic sites in one pass. Also establishes Step 3 as the single
   source of truth for "what mode combos does this compiler support".
2. **TilePlacementStrategy (Suggestion 2):** Biggest reduction in branching
   complexity. Now also subsumes the primary-vs-secondary workspace and
   export divergence.
3. **PaletteSetupStrategy (Suggestion 3):** Completes the pattern; absorbs
   the new cross-tileset packer prefill / color-budget subtraction logic;
   enables a clean future `pals:patch` implementation.
4. **CompilerDiagnosticsReporter (Suggestion 6):** Independent of the
   strategy work; removes ~200 LOC from `CompilerTask`. Good candidate to
   parallelize with (2) and (3).
5. **Explicit data flow (Suggestion 5):** Longer-term cleanup for
   testability. Now more valuable because the compiler has genuinely grown
   three output channels (Porymap artifacts + diagnostics + Porytiles
   round-trip metadata) and per-anim effective FrameLinking is computed
   twice.
6. **Resolution-Strategy Policy types (Suggestion 7):** Follow-on cleanup
   after data flow is explicit; each policy becomes easy to inject.

**Primary-vs-secondary split (Suggestion 4):** I'd still recommend *against*
a full two-compiler split because the six-step skeleton remains genuinely
shared. Instead, introduce a `CompilationRole` object inside `CompilerTask`
that owns primary-vs-secondary differences (workspace factory, export mode,
palette copy layout, tile-limit arithmetic). This captures the clarity
benefit of the split without duplicating orchestration. It composes with
Suggestions 2 and 3: the strategies receive the `CompilationRole` as context
rather than re-testing `is_secondary()` themselves.

**Three-axis branching (tiles_edit_mode x pals_edit_mode x FrameLinking),
plus is_secondary, plus tiles_pal_mode, plus cross_tileset_anim_linking, plus
packing strategy**: the single-class approach has reached its complexity
ceiling. Strategies + explicit data flow + banning unreachable combos is the
surgical path out. The doc's original guidance still stands, strengthened by
the new axes.
