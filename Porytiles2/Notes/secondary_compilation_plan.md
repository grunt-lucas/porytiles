# Plan: Secondary Tileset Compilation

## Context

Porytiles currently only compiles **primary** tilesets. Secondary tilesets in pokeemerald occupy the upper VRAM region (tiles 512-1023, palettes 6-12) and layer on top of a paired primary. Their metatiles can reference both primary tiles (0-511) and their own tiles (512+). Secondary animations use `TILE_OFFSET_4BPP(NUM_TILES_IN_PRIMARY + X)` for VRAM targeting.

The goal is to generalize the existing primary-only compilation pipeline to handle secondary tilesets. The user's design intent is to reuse existing classes (compiler, workspace, packer) with minimal new abstractions, by:
1. Pre-loading primary tiles into the workspace so secondary tile matching naturally produces correct global indices
2. Passing primary palettes as prefilled/locked to the packer
3. Trimming the primary portion on export

**Constraint**: Secondary compilation requires a Porytiles-managed primary (so we can reliably read its compiled output).

---

## ~~Phase 1: Rename Compiler and Add Paired Primary Parameter~~

**Goal**: Rename `PrimaryTilesetCompiler` to `TilesetCompiler`, add optional `paired_primary` parameter. No behavior change — all existing tests pass. No new types needed.

### ~~1a. Rename `PrimaryTilesetCompiler` → `TilesetCompiler`~~

- Rename header: `primary_tileset_compiler.hpp` → `tileset_compiler.hpp`
- Rename impl: `primary_tileset_compiler.cpp` → `tileset_compiler.cpp`
- Rename class: `PrimaryTilesetCompiler` → `TilesetCompiler`
- Add paired primary parameter to `compile()`:
  ```c++
  [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>>
  compile(const Tileset &tileset, const Tileset *paired_primary = nullptr) const;
  ```
- `paired_primary == nullptr` → primary compilation (existing behavior)
- `paired_primary != nullptr` → secondary compilation
- Update all existing call sites (use cases, commands) to use the new name. Behavior unchanged since `paired_primary` defaults to nullptr.

Inside `CompilerTask`, store `const Tileset *paired_primary_` and derive everything from existing abstractions:
- `is_secondary()` → `paired_primary_ != nullptr`
- Primary tiles → `paired_primary_->porymap_component().tiles_png()`
- Primary compiled palettes → `paired_primary_->porymap_component().pals()`
- Primary pal overrides → `paired_primary_->porytiles_component().pals()`
- `num_tiles_in_primary`, `num_pals_in_primary` → from `DomainConfig` (already available)

### Files to modify
- `Porytiles2/include/porytiles2/domain/services/primary_tileset_compiler.hpp` → rename + modify
- `Porytiles2/lib/domain/services/primary_tileset_compiler.cpp` → rename + modify
- `Porytiles2/include/porytiles2/app/use_cases/create_primary_tileset.hpp` — update type reference
- `Porytiles2/lib/app/use_cases/create_primary_tileset.cpp` — update type reference
- `Porytiles2/include/porytiles2/app/use_cases/compile_primary_tileset.hpp` — update type reference
- `Porytiles2/lib/app/use_cases/compile_primary_tileset.cpp` — update type reference
- `Porytiles2/tools/driver/command_compile_tileset.hpp` — update type reference
- `Porytiles2/tools/driver/command_create_tileset.hpp` — update type reference
- Any other files that `#include` the old header (grep for it)
- CMakeLists.txt source lists

---

## Phase 2: Compiler Secondary Logic — Workspace & Palettes

**Goal**: Teach `CompilerTask` to handle secondary compilation via the paired primary tileset pointer.

### Core idea: "Global index" workspace

For secondary compilation, create a workspace of capacity `num_tiles_total` (1024) with primary tiles pre-loaded at positions 0..`num_tiles_in_primary-1`. The cursor starts at `num_tiles_in_primary`. All tile matching produces **global** indices naturally, so `TilemapEntry.tile_index()` values are correct for both primary and secondary tile references in metatiles.

On export, trim the primary prefix — only positions `num_tiles_in_primary`..N become the secondary's `tiles.png`.

### 2a. `TilesPngWorkspace` changes

New constructor or factory method for secondary initialization:

```c++
/// Initialize workspace pre-loaded with primary tiles. Cursor starts at primary_tile_count.
/// Capacity = total tiles (primary + secondary).
static TilesPngWorkspace for_secondary(
    const Image<IndexPixel> &primary_tiles_png,
    std::size_t primary_tile_count,
    std::size_t total_capacity);
```

Internally:
- Allocate `total_capacity` tiles
- Load primary tiles from image into positions 0..`primary_tile_count-1`
- Register them in `canonical_forms_` for deduplication
- Set cursor to `primary_tile_count` (secondary tile 0 is transparent, matching the vanilla convention)
- Actually, tile 0 is already the global transparent tile. Secondary metatiles referencing transparent should use tile 0. So position `primary_tile_count` doesn't need to be a second transparent tile — it's just the first available secondary slot.

New export mode for secondary:

```c++
/// Export only the secondary portion (tiles from primary_tile_count onward)
[[nodiscard]] Image<IndexPixel> export_secondary_image(
    std::size_t primary_tile_count,
    ExportFlipMode flip_mode,
    ExportTrimMode trim_mode) const;
```

### 2b. Animation slot reservation for secondary

**Decision (confirmed)**: Reserve a transparent tile at position `num_tiles_in_primary` for vanilla compatibility. Vanilla secondary tiles.png tile 0 is always transparent. No metatiles reference it explicitly, but we match vanilla convention.

Parameterize `reserve_anim_slots`:
```c++
void reserve_anim_slots(std::size_t count, std::size_t start_offset);
```
- Primary: `reserve_anim_slots(total, 1)` — anims at positions 1..total, cursor at total+1
- Secondary: `reserve_anim_slots(total, num_tiles_in_primary + 1)` — position `num_tiles_in_primary` is reserved transparent, anims start at `num_tiles_in_primary + 1`

The `for_secondary()` factory must initialize position `num_tiles_in_primary` as a transparent tile and set cursor to `num_tiles_in_primary + 1` (or `num_tiles_in_primary` if no anims, then the first inserted tile after transparent).

### 2c. `pipeline_step_setup_working_data()` changes

In the **optimize** path:

```c++
if (paired_primary_ != nullptr) {
    // Workspace: full capacity with primary tiles pre-loaded
    tiles_workspace_ = TilesPngWorkspace::for_secondary(
        paired_primary_->porymap_component().tiles_png(),
        num_tiles_in_primary_.value(),
        num_tiles_total_.value());

    // Palette packing: secondary palette slots only
    color_count_limit = (num_pals_total - num_pals_in_primary) * (pal::max_size - 1);
    available_pals = bits num_pals_in_primary..num_pals_total-1;

    // Prefill primary palettes as fully locked
    for (i = 0; i < num_pals_in_primary; ++i) {
        packing_params.prefilled_pals_[i] = paired_primary_->porymap_component().pal_at(i);
    }
    // Also carry over any Porytiles pal overrides from the secondary itself for slots >= num_pals_in_primary
} else {
    // existing primary logic (unchanged)
}
```

**Locked/patch modes**: Deferred — implement optimize mode first, handle locked/patch secondary as a follow-up. The optimize path is the primary use case and will validate the full architecture.

### 2d. `pipeline_step_match_tiles_pals()` — no changes needed

Since the workspace uses global indices, `pipeline_helper_assign_tile_via_pal_match` naturally produces global tile indices in TilemapEntry. A secondary metatile that references a primary tile will find it in the workspace at position < `num_tiles_in_primary` — correct.

### 2e. Tile capacity and error messages

Add a computed field to `CompilerTask`:
```c++
std::size_t tile_capacity_ = paired_primary_ != nullptr
    ? num_tiles_total_ - num_tiles_in_primary_
    : num_tiles_in_primary_;
```
Use in `tile_limit_reached` error messages.

### 2f. Export changes in `pipeline_step_assemble_output()`

Currently: `tiles_workspace_->export_image(flip_mode, trim_mode)`

For secondary:
```c++
if (paired_primary_ != nullptr) {
    output_tiles_png = tiles_workspace_->export_secondary_image(
        num_tiles_in_primary_.value(), flip_mode, trim_mode);
} else {
    output_tiles_png = tiles_workspace_->export_image(flip_mode, trim_mode);
}
```

### 2g. Palette output for secondary

Currently copies all 16 palettes to output. For secondary, palettes 0..`num_pals_in_primary-1` should be copied from the primary (or left as-is from the existing Porymap component). Palettes `num_pals_in_primary`..`num_pals_total-1` come from packing. Palettes 13-15 are junk/reserved (same as primary).

The existing logic in `pipeline_helper_run_pal_packing` already handles this if `available_pals` is set correctly and prefilled pals are populated. The palette output loop just copies whatever ended up in `new_porymap_pals_[]`.

### Files to modify
- `Porytiles2/include/porytiles2/domain/models/tiles_png_workspace.hpp` — new factory + export method
- `Porytiles2/lib/domain/models/tiles_png_workspace.cpp` — implement above
- `Porytiles2/lib/domain/services/tileset_compiler.cpp` — secondary branching in setup/assemble phases

---

## Phase 3: Animation System Changes

### 3a. `AnimCodeGenerator` — secondary offset format

**File**: `Porytiles2/lib/infra/services/anim_code_generator.cpp`

`generate_queue_function()` currently emits `TILE_OFFSET_4BPP({})` with `params.tile_offset()`.

Change: accept `bool is_primary` parameter (already available from the `generate()` call — thread it down):

```c++
if (is_primary) {
    format("TILE_OFFSET_4BPP({})", params.tile_offset());
} else {
    format("TILE_OFFSET_4BPP(NUM_TILES_IN_PRIMARY + {})", params.tile_offset());
}
```

`AnimParams.tile_offset()` stores the **local** offset within the tileset's tile space.

For primary: local offset = global workspace index (e.g., tile at workspace position 3 → tile_offset=3).
For secondary: local offset = global workspace index - num_tiles_in_primary (e.g., tile at workspace position 515 → tile_offset=3).

This conversion happens when the compiler stores the offset into AnimParams after registration.

### 3b. `AnimCodeParser` — parse `NUM_TILES_IN_PRIMARY + X` pattern

**File**: `Porytiles2/lib/infra/services/anim_code_parser.cpp`

`extract_tile_offset()` currently only matches `TILE_OFFSET_4BPP ( <integer> )`.

Add second pattern:
```c++
// Pattern: TILE_OFFSET_4BPP ( NUM_TILES_IN_PRIMARY + <integer> )
if (tokens[i+2].is(identifier) && tokens[i+2].text() == "NUM_TILES_IN_PRIMARY" &&
    tokens[i+3].is(plus) &&
    tokens[i+4].is(integer_literal) &&
    tokens[i+5].is(right_paren)) {
    return tokens[i+4].int_value();  // Return local offset
}
```

Check that the token types `plus` / `TokenType::plus` exist in the lexer. If not, add it. (The C code lexer likely already handles `+` as a token.)

### 3c. Compiler animation registration — global-to-local offset conversion

In `pipeline_helper_register_animations()`, after placing keyframe tiles in the workspace and getting the global workspace index (`tile_offset`), convert to local before storing:

```c++
std::size_t local_tile_offset = paired_primary_ != nullptr
    ? tile_offset - num_tiles_in_primary_.value()
    : tile_offset;
anim_tile_matcher_.register_animation(anim_name, anim, tile_offset, ...);  // global for matching
// Later, when setting AnimParams:
params.tile_offset(local_tile_offset);  // local for code gen
```

Wait — `register_animation` takes `tile_offset` and uses it to compute global tile indices for the matcher lookup table. So `register_animation` should receive the **global** offset. Then `AnimParams.tile_offset()` (used by code gen) should be set to the **local** offset.

Need to check: does `register_animation` write to `AnimParams.tile_offset()`? If so, we need two values. If not, they're independent. Looking at the compiler code, `register_animation` stores the offset for matching, and separately the compiler sets `params.tile_offset()` during `pipeline_helper_compile_animations()`.

In `pipeline_helper_compile_animations()` (around line 1200+), the tile_offset is retrieved from the matcher and written to `AnimParams`:
```c++
const auto tile_offset = anim_tile_matcher_.tile_offset(anim_name);
// ...
params.tile_offset(tile_offset);
```

So if the matcher stores global offsets, we convert to local here:
```c++
std::size_t local_offset = paired_primary_ != nullptr
    ? tile_offset - num_tiles_in_primary_.value()
    : tile_offset;
params.tile_offset(local_offset);
```

### Files to modify
- `Porytiles2/include/porytiles2/infra/services/anim_code_generator.hpp` — thread `is_primary` to queue fn
- `Porytiles2/lib/infra/services/anim_code_generator.cpp` — conditional offset format + is_primary threading
- `Porytiles2/lib/infra/services/anim_code_parser.cpp` — add secondary offset pattern
- `Porytiles2/lib/domain/services/tileset_compiler.cpp` — global-to-local offset in compile_animations

---

## Phase 4: Use Cases and CLI

### 4a. New use case: `CompileSecondaryTileset`

**New files**:
- `Porytiles2/include/porytiles2/app/use_cases/compile_secondary_tileset.hpp`
- `Porytiles2/lib/app/use_cases/compile_secondary_tileset.cpp`

Orchestration:
1. Validate secondary tileset exists and is Porytiles-managed
2. Find paired primary via `LayoutMetadataProvider`: iterate layouts, find first where `secondary_tileset == this tileset`, get its `primary_tileset`. **Warn** if other layouts pair this secondary with a different primary.
3. Validate the paired primary is Porytiles-managed (error if not — "Secondary compilation requires a Porytiles-managed primary.")
4. Load the compiled primary tileset via `TilesetRepo`
5. Load secondary tileset
6. Call `compiler_->compile(secondary_tileset, &primary_tileset)`
8. Save compiled secondary via `TilesetRepo`
9. Wire animation code (using `is_primary=false`)

**Primary pairing strategy (confirmed)**: Use first found pairing from layouts.json. Warn if inconsistent pairings exist across layouts. Multi-primary support and explicit CLI override (`--partner-primary`) are future work.

Dependencies: same as `CompilePrimaryTileset` plus `LayoutMetadataProvider`.

### 4b. New use case: `CreateSecondaryTileset`

**New files**:
- `Porytiles2/include/porytiles2/app/use_cases/create_secondary_tileset.hpp`
- `Porytiles2/lib/app/use_cases/create_secondary_tileset.cpp`

Similar to `CreatePrimaryTileset` but adds the paired-primary lookup and passes `&primary_tileset` to compiler. Also needs a `SecondaryTilesetCreator` (or generalize `PrimaryTilesetCreator`) to generate starter assets appropriate for secondary tilesets.

### 4c. CLI command dispatch

**File**: `Porytiles2/tools/driver/command_compile_tileset.hpp`

After resolving the tileset name, query `TilesetMetadataProvider::is_secondary(tileset_name)`:
- If secondary → construct and run `CompileSecondaryTileset`
- If primary → existing `CompilePrimaryTileset`

Same pattern for `command_create_tileset.hpp`.

Both commands need `ProjectLayoutMetadataProvider` setup added to their dependency wiring.

### Files to create
- `Porytiles2/include/porytiles2/app/use_cases/compile_secondary_tileset.hpp`
- `Porytiles2/lib/app/use_cases/compile_secondary_tileset.cpp`
- `Porytiles2/include/porytiles2/app/use_cases/create_secondary_tileset.hpp`
- `Porytiles2/lib/app/use_cases/create_secondary_tileset.cpp`

### Files to modify
- `Porytiles2/tools/driver/command_compile_tileset.hpp` — dispatch by is_secondary
- `Porytiles2/tools/driver/command_create_tileset.hpp` — dispatch by is_secondary
- `Porytiles2/CMakeLists.txt` — add new source files

---

## Phase 5: Validation

New validations for secondary compilation (in `pipeline_step_validate_input` or use case layer):

1. **Primary must be Porytiles-managed** — checked in use case before compilation
2. **Metatile count fits secondary range** — count <= `num_metatiles_total - num_metatiles_in_primary`
3. **Color count within secondary palette budget** — `(num_pals_total - num_pals_in_primary) * 15` unique colors max
4. **Global color count** includes primary palette colors for the ColorIndexMap since secondary tiles can reference primary palettes (needed for tile matching)

### Files to modify
- `Porytiles2/lib/domain/algorithms/tileset_compile_validators.cpp` — add secondary-aware validators
- `Porytiles2/lib/domain/services/tileset_compiler.cpp` — call secondary validators when paired_primary_ != nullptr

---

## Implementation Order

1. **Phase 1** (rename + paired_primary parameter) — zero behavior change, safe refactor
2. **Phase 2** (workspace + palette changes) — core secondary compilation logic
3. **Phase 3** (animation system) — code gen/parse + offset conversion
4. **Phase 4** (use cases + CLI) — wire it all together
5. **Phase 5** (validation) — can be interleaved with Phase 2-4

---

## Verification Plan

1. **Build**: `cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1` — must compile on both GCC and Clang
2. **Existing tests**: `./porytiles-build-debug/Porytiles2/tests/Porytiles2AllTests > /tmp/test.log 2>&1` — all must pass (no regressions from rename)
3. **New unit tests**: AnimCodeGenerator secondary output, AnimCodeParser secondary parsing, TilesPngWorkspace::for_secondary, palette packing with prefilled primary palettes
4. **Integration test**: Compile a secondary tileset from `pokeemerald-expansion` testbed (e.g., `gTileset_Petalburg` against `gTileset_General`), verify output artifacts match or are equivalent to vanilla
5. **Round-trip**: Decompile → recompile a secondary tileset, verify idempotency
6. **Error cases**: attempt secondary compilation against non-Porytiles-managed primary → expect clear error

---

## Resolved Design Decisions

1. **Transparent tile at secondary base**: **Reserve as transparent** at position `num_tiles_in_primary` for vanilla compatibility.
2. **Locked/patch mode for secondary**: **Deferred** — implement optimize mode first, handle locked/patch as follow-up.
3. **Primary pairing strategy**: **Use first found** pairing from layouts.json. Warn on inconsistent pairings. Multi-primary support and `--partner-primary` CLI flag are future work.

## Future Work (Out of Scope)

- Multiple partner primary support (see `topic_staging_area.md`)
- `--partner-primary` CLI flag for explicit override
- Locked/patch mode for secondary compilation
- Primary Palette Fixing (out-of-band pals, see `topic_staging_area.md`)
