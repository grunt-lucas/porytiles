# PrimaryTilesetCompiler: Logical Flow & Refactoring Analysis

## Context

`primary_tileset_compiler.cpp` contains a `CompilerTask` class (~1600 lines) that compiles
Porytiles-format tileset assets into Porymap-format output. The logic branches extensively
on two mode axes: `tiles_edit_mode` (optimize/patch/locked) and `pals_edit_mode`
(optimize/patch/locked). This document traces the complete pipeline flow broken out by mode,
then proposes refactoring strategies.

---

## Part 1: Complete Pipeline Flow

### Step 1: `pipeline_step_process_porytiles_input()` — No mode branching
- Metatileize Porytiles bottom/middle/top layer images
- Decompose metatiles into flat pixel tile vector
- Generate canonical versions of each tile

### Step 2: `pipeline_step_process_porymap_input()` — No mode branching
- Triple-layerize existing Porymap component
- Decompile tilemap entries + tiles.png + pals into metatile vector
- Decompose into flat pixel tile vector + canonical versions

### Step 3: `pipeline_step_validate_input()` — Minor branching

| Check                       | pals:optimize | pals:locked | pals:patch |
|-----------------------------|:-------------:|:-----------:|:----------:|
| Validate metatile count     |      Yes      |     Yes     |    Yes     |
| Validate Porymap pals       |    **No**     |   **Yes**   |  **Yes**   |
| Validate Porytiles pals     |      Yes      |     Yes     |    Yes     |
| Validate palette hints      |      Yes      |     Yes     |    Yes     |
| Validate alpha channels     |      Yes      |     Yes     |    Yes     |
| Validate layer mode         |      Yes      |     Yes     |    Yes     |
| Validate tile color count   |      Yes      |     Yes     |    Yes     |
| Validate global color count |      Yes      |     Yes     |    Yes     |
| Validate precision loss     |      Yes      |     Yes     |    Yes     |
| Validate anim frames        |      Yes      |     Yes     |    Yes     |

### Step 4: `pipeline_step_setup_working_data()` — Heavy branching

#### 4a: Palette Creation (pals_edit_mode axis)

|                  pals:optimize                  |              pals:locked               |    pals:patch     |
|:-----------------------------------------------:|:--------------------------------------:|:-----------------:|
| Build ColorIndexMap from tiles+anims+pals+hints | Copy all 16 pals from existing Porymap | **panic("TODO")** |
|      Run OverloadAndRemoveStrategy packing      |            (no computation)            |                   |
|   Resolve out-of-band pals (wildcards→black)    |                                        |                   |
| Copy unpacked secondary/junk pals from original |                                        |                   |

#### 4b: TilesPngWorkspace Creation (tiles_edit_mode axis)

|          tiles:optimize           |            tiles:patch            |           tiles:locked           |
|:---------------------------------:|:---------------------------------:|:--------------------------------:|
|          Empty workspace          |  Loaded from existing tiles.png   |  Loaded from existing tiles.png  |
| Capacity = `num_tiles_in_primary` | Capacity = `num_tiles_in_primary` | Capacity = exact `size_in_tiles` |

#### 4c: Animation Registration (tiles_edit_mode axis)

All modes share: build keyframe data (palette-match composite tiles, convert to IndexPixel).

| Phase         |                  tiles:optimize                  |                                                 tiles:patch                                                 |                        tiles:locked                         |
|---------------|:------------------------------------------------:|:-----------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------:|
| **Pre-loop**  |           `reserve_anim_slots(total)`            |                                                      —                                                      |                              —                              |
| **Placement** | Sequential `place_anim_tile` at reserved offsets | Try `find_existing_contiguous_tiles_by_color` → else `find_contiguous_transparent_slots` + `place_tiles_at` | Must `find_existing_contiguous_tiles_by_color` or **error** |
| **Post-loop** |        Register all with AnimTileMatcher         |                                      Register all with AnimTileMatcher                                      |              Register all with AnimTileMatcher              |

### Step 5: `pipeline_step_match_tiles_pals()` — Heavy branching

**Pre-check:** pals:patch → panic("TODO")

For each Porytiles tile, in order:

```
FOR each porytiles tile [i]:
│
├─ [tiles != optimize] → TRY REUSE PORYMAP TILE
│   Check exact match, then canonical match at same index
│   If found → emit reused TilemapEntry, CONTINUE
│
├─ TRANSPARENT CHECK (all modes)
│   If transparent → emit TilemapEntry{0,0,false,false}, CONTINUE
│
└─ ASSIGN VIA PALETTE MATCH (pipeline_helper_assign_tile_via_pal_match)
    │
    ├─ Match tile against new_porymap_pals_ → find covering palette
    │   No covering pal:
    │     [pals:optimize] → panic (should have failed at packing)
    │     [pals:locked]   → emit "no matching pal" error
    │
    ├─ Convert to IndexPixel tile using matched palette
    │
    ├─ ANIM MATCHER GATING
    │   [tiles:optimize]    → always check anim matcher
    │   [tiles:locked/patch] → only check if original tile_index is in animation range
    │
    ├─ Check anim matcher → if match, use animation tile_index + pal_index
    │
    ├─ TILE LOOKUP IN WORKSPACE
    │   [tiles:optimize]    → first_occurrence_of (exact index match, O(1))
    │   [tiles:locked/patch] → first_occurrence_of_by_color (color-equivalence, O(n))
    │   If found → compute flip bits, return success
    │
    └─ TILE NOT FOUND
        [tiles:locked]        → error "tile not found"
        [tiles:optimize/patch] → check capacity, insert if room, else "tile limit" error
```

### Step 6: `pipeline_step_assemble_output()` — Minor branching

| Sub-step                 |           tiles:optimize            |       tiles:locked/patch       |
|--------------------------|:-----------------------------------:|:------------------------------:|
| Compile animations       |                Same                 |              Same              |
| Apply manual overrides   |      Same (FrameLinking-based)      |   Same (FrameLinking-based)    |
| Dual-layer conversion    | Same (num_tiles_per_metatile-based) |              Same              |
| Copy metatile attributes |                Same                 |              Same              |
| **Export tiles.png**     |     `trim_trailing_transparent`     | `include_trailing_transparent` |
| Copy palettes            |                Same                 |              Same              |
| True-color encoding      |     Same (tiles_pal_mode-based)     |              Same              |

---

## Part 2: Mode Combination Matrix

### Valid/Working Combinations

|                    |                 pals:optimize                  |                    pals:locked                     | pals:patch |
|--------------------|:----------------------------------------------:|:--------------------------------------------------:|:----------:|
| **tiles:optimize** |    **Full recompile** (Porytiles1 behavior)    |     New tiles from scratch, keep existing pals     |    TODO    |
| **tiles:patch**    |   Fresh pals, insert new tiles into existing   | Keep existing pals, insert new tiles into existing |    TODO    |
| **tiles:locked**   | **BUGGY** (pals change but tiles can't update) |            **Verify mode** (no changes)            |    TODO    |

### The pals:optimize + tiles:locked Bug
The TODO in `run()` explains: if palettes are re-optimized but tiles are locked, the compiler
emits identical metatile entries (since Porytiles/Porymap metatiles match) but with new palette
assignments. The tilemap entries then reference palette slots that don't match the locked tile
data, corrupting the tileset. **This combo should be banned.**

---

## Part 3: Refactoring Suggestions

### Suggestion 1: Ban the Invalid Combo (Quick Win)
Add validation at the top of `run()` to reject `pals:optimize + tiles:locked` with a clear
error message. Reduces the working matrix and removes a known corruption vector.

### Suggestion 2: Strategy Pattern for Tile Placement (Recommended)

Extract tile-mode branching into a **TilePlacementStrategy** interface:

```
TilePlacementStrategy (interface)
├── create_workspace(tileset, capacity) → TilesPngWorkspace
├── place_anim_keyframes(workspace, keyframe_data) → offset
├── try_reuse_tile(tile_index, ...) → optional<TilemapEntry>
├── lookup_tile(canonical_tile, workspace, palette) → optional<size_t>
├── handle_tile_not_found(canonical_tile, workspace) → TileAssignmentResult
└── export_image(workspace) → Image

OptimizeTilePlacement
├── Empty workspace, exact lookup, insert freely, reserve anim slots
PatchTilePlacement
├── Loaded workspace, color-equiv lookup, insert into free space
LockedTilePlacement
└── Loaded workspace, color-equiv lookup, error on miss
```

This eliminates ~60% of the if/else branching. The pipeline skeleton stays in CompilerTask
but delegates to the strategy at each branch point.

### Suggestion 3: Strategy Pattern for Palette Setup (Complementary)

Extract palette-mode branching into a **PaletteSetupStrategy** interface:

```
PaletteSetupStrategy (interface)
├── create_palettes(tileset, ...) → array<Palette, 16>
└── should_validate_porymap_pals() → bool

OptimizePaletteSetup  → full packing algorithm
LockedPaletteSetup    → copy existing palettes
PatchPaletteSetup     → (future) incremental editing
```

Less impactful than Suggestion 2 since palette logic is already fairly isolated in
`pipeline_helper_run_pal_packing`, but completes the pattern.

### Suggestion 4: Two Compilers — FreshCompiler vs IncrementalCompiler

This is the user's original idea (OptimizingCompiler vs PatchCompiler). Reframed:
- **FreshCompiler**: handles `tiles:optimize + pals:optimize` — the clean-slate case
- **IncrementalCompiler**: handles all locked/patch combinations — works with existing assets

**Pros:**
- Each compiler is conceptually simpler and self-contained
- Easier to reason about invariants within each compiler
- No need for strategy interfaces

**Cons:**
- ~80% of the pipeline is shared (steps 1-3, animation compilation, dual-layer conversion,
  attribute copying, true-color encoding). Significant duplication risk.
- Three-way tile mode branching within IncrementalCompiler still exists (locked vs patch)
- Harder to add new modes later

**Mitigation:** Extract shared logic into free helper functions called by both compilers.
This is viable but results in a "helper explosion" — many free functions that are only called
from two places.

### Suggestion 5: Explicit Data Flow Between Steps (Longer-Term)

Replace the ~20+ shared member variables with explicit return types between pipeline steps:

```
ProcessedPorytiles  ← step 1
ProcessedPorymap    ← step 2
(validation)        ← step 3
WorkingData         ← step 4 (palettes, workspace, matcher)
MatchedTilemap      ← step 5
OutputTileset       ← step 6
```

Each step becomes a quasi-pure function. Benefits: testability, readability, no implicit
coupling. This is orthogonal to the strategy pattern — they compose well together.

---

## Part 4: Recommended Approach

**Priority order:**
1. **Ban pals:optimize + tiles:locked** — trivial, eliminates a corruption bug
2. **TilePlacementStrategy** (Suggestion 2) — biggest reduction in branching complexity
3. **PaletteSetupStrategy** (Suggestion 3) — completes the pattern, enables clean pals:patch impl
4. **Explicit data flow** (Suggestion 5) — longer-term cleanup for testability

I'd recommend **against** the two-compiler split because the pipeline skeleton is genuinely
shared. The strategy pattern gives the same clarity ("what does optimize mode do at this step?")
without duplicating orchestration logic. The strategies also make it straightforward to implement
pals:patch later — just add a new strategy implementation.
