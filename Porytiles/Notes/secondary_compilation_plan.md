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
- `Porytiles/include/porytiles/domain/services/primary_tileset_compiler.hpp` → rename + modify
- `Porytiles/lib/domain/services/primary_tileset_compiler.cpp` → rename + modify
- `Porytiles/include/porytiles/app/use_cases/create_primary_tileset.hpp` — update type reference
- `Porytiles/lib/app/use_cases/create_primary_tileset.cpp` — update type reference
- `Porytiles/include/porytiles/app/use_cases/compile_primary_tileset.hpp` — update type reference
- `Porytiles/lib/app/use_cases/compile_primary_tileset.cpp` — update type reference
- `Porytiles/tools/driver/command_compile_tileset.hpp` — update type reference
- `Porytiles/tools/driver/command_create_tileset.hpp` — update type reference
- Any other files that `#include` the old header (grep for it)
- CMakeLists.txt source lists

---

## ~~Phase 2: Compiler Secondary Logic — Workspace & Palettes~~

**Goal**: Teach `CompilerTask` to handle secondary compilation via the paired primary tileset pointer.

### Core idea: "Global index" workspace

For secondary compilation, create a workspace of capacity `num_tiles_total` (1024) with primary tiles pre-loaded at positions 0..`num_tiles_in_primary-1`. The cursor starts at `num_tiles_in_primary`. All tile matching produces **global** indices naturally, so `TilemapEntry.tile_index()` values are correct for both primary and secondary tile references in metatiles.

On export, trim the primary prefix — only positions `num_tiles_in_primary`..N become the secondary's `tiles.png`.

### ~~2a. `TilesPngWorkspace` changes~~

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

### ~~2b. Animation slot reservation for secondary~~

**Decision (confirmed)**: Reserve a transparent tile at position `num_tiles_in_primary` for vanilla compatibility. Vanilla secondary tiles.png tile 0 is always transparent. No metatiles reference it explicitly, but we match vanilla convention.

Parameterize `reserve_anim_slots`:
```c++
void reserve_anim_slots(std::size_t count, std::size_t start_offset);
```
- Primary: `reserve_anim_slots(total, 1)` — anims at positions 1..total, cursor at total+1
- Secondary: `reserve_anim_slots(total, num_tiles_in_primary + 1)` — position `num_tiles_in_primary` is reserved transparent, anims start at `num_tiles_in_primary + 1`

The `for_secondary()` factory must initialize position `num_tiles_in_primary` as a transparent tile and set cursor to `num_tiles_in_primary + 1` (or `num_tiles_in_primary` if no anims, then the first inserted tile after transparent).

### ~~2c. `pipeline_step_setup_working_data()` changes~~

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

### ~~2d. `pipeline_step_match_tiles_pals()` — no changes needed~~

Since the workspace uses global indices, `pipeline_helper_assign_tile_via_pal_match` naturally produces global tile indices in TilemapEntry. A secondary metatile that references a primary tile will find it in the workspace at position < `num_tiles_in_primary` — correct.

### ~~2e. Tile capacity and error messages~~

Add a computed field to `CompilerTask`:
```c++
std::size_t tile_capacity_ = paired_primary_ != nullptr
    ? num_tiles_total_ - num_tiles_in_primary_
    : num_tiles_in_primary_;
```
Use in `tile_limit_reached` error messages.

### ~~2f. Export changes in `pipeline_step_assemble_output()`~~

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

### ~~2g. Palette output for secondary~~

Currently copies all 16 palettes to output. For secondary, palettes 0..`num_pals_in_primary-1` should be copied from the primary (or left as-is from the existing Porymap component). Palettes `num_pals_in_primary`..`num_pals_total-1` come from packing. Palettes 13-15 are junk/reserved (same as primary).

The existing logic in `pipeline_helper_run_pal_packing` already handles this if `available_pals` is set correctly and prefilled pals are populated. The palette output loop just copies whatever ended up in `new_porymap_pals_[]`.

### Files to modify
- `Porytiles/include/porytiles/domain/models/tiles_png_workspace.hpp` — new factory + export method
- `Porytiles/lib/domain/models/tiles_png_workspace.cpp` — implement above
- `Porytiles/lib/domain/services/tileset_compiler.cpp` — secondary branching in setup/assemble phases

---

## ~~Phase 3: Animation System Changes~~

### ~~3a. `AnimCodeGenerator` — secondary offset format~~

**File**: `Porytiles/lib/infra/services/anim_code_generator.cpp`

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

### ~~3b. `AnimCodeParser` — parse `NUM_TILES_IN_PRIMARY + X` pattern~~

**File**: `Porytiles/lib/infra/services/anim_code_parser.cpp`

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

### ~~3c. Compiler animation registration — global-to-local offset conversion~~

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
- `Porytiles/include/porytiles/infra/services/anim_code_generator.hpp` — thread `is_primary` to queue fn
- `Porytiles/lib/infra/services/anim_code_generator.cpp` — conditional offset format + is_primary threading
- `Porytiles/lib/infra/services/anim_code_parser.cpp` — add secondary offset pattern
- `Porytiles/lib/domain/services/tileset_compiler.cpp` — global-to-local offset in compile_animations

---

## ~~Phase Pre-4: Decouple `is_secondary` from `paired_primary` Pointer~~

**Goal**: The current compiler uses `paired_primary_ != nullptr` as the sole signal for secondary compilation. This conflates two independent facts: (1) the tileset is secondary, and (2) a real primary is available. `PrimaryPairingMode::off` needs secondary compilation behavior with no primary -- passing `nullptr` would incorrectly trigger primary compilation logic. A synthetic "blank primary" (Null Object) doesn't work either: zeroed palettes pollute the color index map, causing false matches when secondary tiles contain `Rgba(0,0,0,0)`.

**Change**: Add explicit `bool is_secondary` parameter to `compile()`, decoupling the two concerns.

### ~~Pre-4a. Update `TilesetCompiler::compile()` signature~~

```c++
[[nodiscard]] ChainableResult<std::unique_ptr<Tileset>>
compile(const Tileset &tileset, bool is_secondary = false, const Tileset *paired_primary = nullptr) const;
```

- `is_secondary == false` -> primary compilation (existing behavior), `paired_primary` ignored
- `is_secondary == true, paired_primary != nullptr` -> secondary compilation against a real primary
- `is_secondary == true, paired_primary == nullptr` -> standalone secondary compilation (no primary)

Update all existing call sites to pass `/*is_secondary=*/false` explicitly (or rely on default).

### ~~Pre-4b. Update `CompilerTask` internals~~

Store both `bool is_secondary_` and `const Tileset *paired_primary_` independently:
- `is_secondary()` -> returns `is_secondary_` (no longer derived from pointer)
- New helper: `has_paired_primary()` -> `paired_primary_ != nullptr` (guards all 5 dereference sites)

The 5 dereference sites and their standalone secondary behavior:

1. **Workspace init** (`pipeline_step_setup_working_data`): If `has_paired_primary()`, use `for_secondary()` with real tiles. Else create workspace with capacity `num_tiles_total`, cursor at `num_tiles_in_primary + 1`, positions 0 through `num_tiles_in_primary` filled with transparent tiles. No `canonical_forms_` registrations.
2. **Palette prefill** (`pipeline_helper_run_pal_packing`): If `has_paired_primary()`, lock primary palettes. Else skip prefill. Still restrict `available_pals` to secondary slots (num_pals_in_primary..num_pals_total-1) in both cases.
3. **Color index map** (`pipeline_helper_build_color_index_map`): If `has_paired_primary()`, add primary palette colors. Else skip entirely.
4. **Output palette copy** (`pipeline_step_assemble_output`): If `has_paired_primary()`, copy primary palettes. Else write zeroed palettes to slots 0 through num_pals_in_primary-1.
5. **Export** (`pipeline_step_assemble_output`): Use `export_secondary_image()` when `is_secondary()` (unchanged, now driven by flag instead of pointer).

### Files to modify
- `Porytiles/include/porytiles/domain/services/tileset_compiler.hpp` -- new `is_secondary` param
- `Porytiles/lib/domain/services/tileset_compiler.cpp` -- `CompilerTask` changes + 5 guarded dereference sites
- `Porytiles/lib/app/use_cases/compile_primary_tileset.cpp` -- update call site (explicit `false`)

---

## ~~Phase 4: Use Cases and CLI~~

### ~~4a. New use case: `CompileSecondaryTileset`~~

**New files**:
- `Porytiles/include/porytiles/app/use_cases/compile_secondary_tileset.hpp`
- `Porytiles/lib/app/use_cases/compile_secondary_tileset.cpp`

**Config-driven primary pairing**: Two new config values control how the secondary finds its partner primary:

- `tileset.primary_pairing.mode` (`PrimaryPairingMode`): `off`, `manual`, `automatic`
- `tileset.primary_pairing.partners` (`std::vector<std::string>`): tileset names, use `std::vector<std::string>` parser for CLI

Orchestration:
1. Validate secondary tileset exists and is Porytiles-managed
2. Resolve partner primary based on `primary_pairing_mode`:
   - **`off`**: Skip primary loading entirely. `paired_primary` stays `nullptr`. If `partners` is non-empty, warn that it is ignored in `off` mode.
   - **`manual`**: Read `primary_pairing_partners` from config. Error if empty ("Manual pairing mode requires at least one partner primary."). Use the first entry as the partner primary (plural list supports eventual multi-primary). Validate partner is Porytiles-managed.
   - **`automatic`**: Scan layouts via `LayoutMetadataProvider` -- iterate all layouts, find those where `secondary_tileset == this tileset`, collect their `primary_tileset` values. Use the first found primary as the partner. **Warn** if multiple distinct primaries are found across layouts. If `partners` is non-empty, warn that the provided list is ignored in `automatic` mode. Error if no layout pairs this secondary with any primary. Add a TODO comment that eventually we'll support multiple distinct primaries.
3. If a partner primary was resolved (modes `manual` and `automatic`):
   - Validate the partner primary is Porytiles-managed (error if not: "Secondary compilation requires a Porytiles-managed primary.")
   - Load the compiled primary tileset via `TilesetRepo`
4. Load secondary tileset
5. Call `compiler_->compile(secondary_tileset, /*is_secondary=*/true, paired_primary_ptr)` -- `nullptr` for `off` mode, `&primary_tileset` otherwise
6. Save compiled secondary via `TilesetRepo`
7. Wire animation code (using `is_primary=false`)

Dependencies: same as `CompilePrimaryTileset` plus `LayoutMetadataProvider` (only needed for `automatic` mode, but always wired for simplicity).

### ~~4b. New use case: `CreateSecondaryTileset`~~

**New files**:
- `Porytiles/include/porytiles/app/use_cases/create_secondary_tileset.hpp`
- `Porytiles/lib/app/use_cases/create_secondary_tileset.cpp`

Similar to `CreatePrimaryTileset` but adds the paired-primary lookup and passes `&primary_tileset` to compiler. Uses `TilesetCreator::create_sample_secondary_porytiles_component` to generate starter assets appropriate for secondary tilesets.

### ~~4c. CLI command dispatch~~

**File**: `Porytiles/tools/driver/command_compile_tileset.hpp`

After resolving the tileset name, query `TilesetMetadataProvider::is_secondary(tileset_name)`:
- If secondary → construct and run `CompileSecondaryTileset`
- If primary → existing `CompilePrimaryTileset`

Same pattern for `command_create_tileset.hpp`.

Both commands need `ProjectLayoutMetadataProvider` setup added to their dependency wiring.

### ~~4d. Secondary-specific validations~~

New validations for secondary compilation (in `pipeline_step_validate_input` or use case layer):

1. **Primary must be Porytiles-managed** — checked in use case before compilation (step 3 of 4a orchestration)
2. **Metatile count fits secondary range** — count <= `num_metatiles_total - num_metatiles_in_primary`
3. **Color count within secondary palette budget** — `(num_pals_total - num_pals_in_primary) * 15` unique colors max
4. **Global color count** includes primary palette colors for the ColorIndexMap since secondary tiles can reference primary palettes (needed for tile matching)

### Files to create
- `Porytiles/include/porytiles/app/use_cases/compile_secondary_tileset.hpp`
- `Porytiles/lib/app/use_cases/compile_secondary_tileset.cpp`
- `Porytiles/include/porytiles/app/use_cases/create_secondary_tileset.hpp`
- `Porytiles/lib/app/use_cases/create_secondary_tileset.cpp`

### Files to modify
- `Porytiles/tools/driver/command_compile_tileset.hpp` — dispatch by is_secondary
- `Porytiles/tools/driver/command_create_tileset.hpp` — dispatch by is_secondary
- `Porytiles/CMakeLists.txt` — add new source files
- `Porytiles/lib/domain/algorithms/tileset_compile_validators.cpp` — add secondary-aware validators
- `Porytiles/lib/domain/services/tileset_compiler.cpp` — call secondary validators when paired_primary_ != nullptr

---

## Implementation Order

1. **Phase 1** (rename + paired_primary parameter) — zero behavior change, safe refactor
2. **Phase 2** (workspace + palette changes) — core secondary compilation logic
3. **Phase 3** (animation system) — code gen/parse + offset conversion
4. **Phase Pre-4** (decouple is_secondary from paired_primary pointer) — enable standalone secondary compilation
5. **Phase 4** (use cases, CLI, and validation) — wire it all together with secondary-specific validations

---

## Verification Plan

1. **Build**: `cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1` — must compile on both GCC and Clang
2. **Existing tests**: `./porytiles-build-debug/Porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1` — all must pass (no regressions from rename)
3. **New unit tests**: AnimCodeGenerator secondary output, AnimCodeParser secondary parsing, TilesPngWorkspace::for_secondary, palette packing with prefilled primary palettes
4. **Integration test**: Compile a secondary tileset from `pokeemerald-expansion` testbed (e.g., `gTileset_Petalburg` against `gTileset_General`), verify output artifacts match or are equivalent to vanilla
5. **Round-trip**: Decompile → recompile a secondary tileset, verify idempotency
6. **Error cases**: attempt secondary compilation against non-Porytiles-managed primary → expect clear error

---

## ~~Phase 5: Primary Animation References for Secondary Tilesets~~

**Goal**: Allow secondary tilesets to manually link metatile entries to primary animation tile ranges.

### Context

When compiling a secondary tileset, metatiles may reference tiles from primary animations (e.g., water, flowers). In `FrameLinking::auto` mode, this works out of the box via workspace tile matching (primary key frame tiles are pre-loaded at their correct global indices). In `FrameLinking::manual` mode, there is no mechanism for users to specify overrides referencing primary animation tiles.

### ~~5a. anim.json extension: `primary_references`~~

Add an optional `primary_references` top-level key to secondary tilesets' anim.json. Each entry maps a primary animation name to a list of override entries (same `AnimOverrideEntry` format as existing overrides):

```json
{
  "red_flower": {
    "frames": [
      "center",
      "right",
      "left"
    ],
    "frame_order": [
      "center",
      "right",
      "center",
      "left"
    ],
    "tile_offset": 1
  },
  "primary_references": {
    "flower": {
      "overrides": [
        { "id": 5, "layer": "bottom", "subtile": "nw", "frame_subtile": 0, "pal_index": 2, "hflip": false, "vflip": false },
        { "id": 5, "layer": "bottom", "subtile": "ne", "frame_subtile": 1, "pal_index": 2, "hflip": false, "vflip": false }
      ]
    }
  }
}
```

### ~~5b. Data model: `PorytilesTilesetComponent`~~

**File**: `Porytiles/include/porytiles/domain/models/porytiles_tileset_component.hpp`

Add a new field and accessors:

```c++
// Private:
std::map<DynamicCasedName, std::vector<AnimOverrideEntry>> primary_anim_overrides_;

// Public:
[[nodiscard]] const std::map<DynamicCasedName, std::vector<AnimOverrideEntry>> &primary_anim_overrides() const;
void primary_anim_overrides(std::map<DynamicCasedName, std::vector<AnimOverrideEntry>> overrides);
```

### ~~5c. Parser: `AnimJsonParser::parse_primary_references()`~~

**Files**: `Porytiles/include/porytiles/infra/services/anim_json_parser.hpp`, `Porytiles/lib/infra/services/anim_json_parser.cpp`

Add a new method (additive, existing `parse()` unchanged):

```c++
[[nodiscard]] ChainableResult<std::map<DynamicCasedName, std::vector<AnimOverrideEntry>>>
parse_primary_references(const std::filesystem::path &json_path) const;
```

Reads the same anim.json but extracts only the `primary_references` key. Returns empty map if absent. Reuses existing override parsing logic. Also extend `write()` to round-trip `primary_references`.

### ~~5d. Tileset loading~~

**File**: `Porytiles/lib/infra/repos/project_tileset_artifact_reader.cpp` (or `tileset_repo.cpp`)

After loading porytiles animations, call `parse_primary_references()` on the same anim.json path and store on `PorytilesTilesetComponent::primary_anim_overrides_`.

### ~~5e. Compiler: apply primary animation overrides~~

**File**: `Porytiles/lib/domain/services/tileset_compiler.cpp`

In `pipeline_helper_apply_manual_overrides()`, after the existing loop over secondary animations, add:

```c++
if (!is_secondary()) {
    if (!tileset_.porytiles_component().primary_anim_overrides().empty()) {
        // Error: primary tilesets cannot have primary_references
    }
    return;
}

const auto &primary_overrides = tileset_.porytiles_component().primary_anim_overrides();
const auto &primary_anims = paired_primary_->porymap_component().anims();

for (const auto &[primary_anim_name, overrides] : primary_overrides) {
    // Validate: primary animation exists in paired_primary
    // tile_offset from primary AnimParams is LOCAL to primary = GLOBAL offset (primary starts at 0)
    // For each override: absolute_tile = tile_offset + entry.frame_subtile
    // Write to metatiles_bin
}
```

### ~~5f. Validations~~

1. **Primary animation exists**: Error if referenced name not in `paired_primary_->porymap_component().anims()`.
2. **frame_subtile in bounds**: Error if `entry.frame_subtile >= tile_count`.
3. **Primary-only guard**: Error if a primary tileset has `primary_references`.
4. **Metatile ID in bounds**: Error if `entry.metatile_id` exceeds secondary metatile count.

### Files to modify

| File                                                                          | Change                                                                |
|-------------------------------------------------------------------------------|-----------------------------------------------------------------------|
| `Porytiles/include/porytiles/domain/models/porytiles_tileset_component.hpp` | Add `primary_anim_overrides_` field and accessors                     |
| `Porytiles/include/porytiles/infra/services/anim_json_parser.hpp`           | Add `parse_primary_references()` method                               |
| `Porytiles/lib/infra/services/anim_json_parser.cpp`                          | Implement `parse_primary_references()`, extend `write()`              |
| `Porytiles/lib/infra/repos/project_tileset_artifact_reader.cpp`              | Call `parse_primary_references()` during anim loading                 |
| `Porytiles/lib/domain/services/tileset_compiler.cpp`                         | Apply primary overrides in `pipeline_helper_apply_manual_overrides()` |

---

## Resolved Design Decisions

1. **Transparent tile at secondary base**: **Reserve as transparent** at position `num_tiles_in_primary` for vanilla compatibility.
2. **Locked/patch mode for secondary**: **Deferred** — implement optimize mode first, handle locked/patch as follow-up.
3. **Primary pairing strategy**: **Config-driven** via `tileset.primary_pairing.mode` (`off`/`manual`/`automatic`) and `tileset.primary_pairing.partners` (list of tileset names, settable via CLI or YAML). Default is `automatic` (layout.json scan). `manual` uses the provided partners list. `off` compiles with no primary. Partners list is plural to support eventual multi-primary.
6. **Standalone secondary (off mode)**: Decoupled `is_secondary` flag from `paired_primary` pointer (Phase Pre-4). A Null Object / synthetic blank primary was rejected because zeroed palettes pollute the color index map, causing false matches for secondary tiles containing `Rgba(0,0,0,0)`. Instead, the compiler guards all 5 `paired_primary_` dereference sites with `has_paired_primary()` and provides standalone fallback behavior (transparent workspace, no palette prefill, no color map pollution).
4. **Primary animation matching (auto mode)**: Works via workspace tile matching in optimize mode. Primary tiles (including animation key frames) are pre-loaded at correct global indices. No explicit registration needed for MVP. See code comment in `pipeline_helper_register_animations()`.
5. **Primary animation references (manual mode)**: New `primary_references` section in anim.json (Phase 5). Uses same `AnimOverrideEntry` format. Primary tile_offset from `paired_primary_->porymap_component().anims()` is local-to-primary = global (no adjustment needed).

## Future Work (Out of Scope)

- Multiple partner primary compilation (partners list supports multiple entries, but compiler currently uses only the first)
- Locked/patch mode for secondary compilation
- Primary Palette Fixing (out-of-band pals, see `topic_staging_area.md`)
- Register primary animations in `AnimTileMatcher` for robustness when secondary patch/locked modes are added
