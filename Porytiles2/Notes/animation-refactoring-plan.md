# Animation Loading Refactoring Plan

## Goal
Refactor TilesetRepo and affiliated helper services so that Animations are a first-class concept with coherent loading.

## Problems Being Solved

1. **Infra leaking into domain**: `AnimationCallbackInfo` (infra concept with C file paths) is exposed in `TilesetArtifactKeyProvider` domain interface
2. **Incoherent discovery**: Frames discovered separately from parameters via different code paths
3. **Duplicated logic**: Animation name parsing exists in both `AnimCodeParser` and `ProjectTilesetMetadataProvider`
4. **Porymap frames via filesystem**: Currently discovers Porymap frames via hardcoded paths instead of always parsing C code

## Desired End State

### PorytilesComponent Animations
- Discovered from `porytiles/anim/{anim}/` folders containing `key.png`, `0.png`, `1.png`, etc.
- Parameters from `porytiles/anim/anim.yaml`
- One coherent code path

### PorymapComponent Animations
- **Option 1**: Parse `include/generated_anim_code.h` if present (has INCBINs + code)
- **Option 2**: Parse `header.h` for callback → `tileset_anims.c` for frame paths + parameters
- **NEVER** use hardcoded filesystem paths - always discover via C code parsing

---

## Implementation Phases

### Phase 1: Create Shared Animation Name Parsing Utility

**New Files:**
- `Porytiles2/include/porytiles2/utilities/anim_name_utils.hpp`
- `Porytiles2/lib/utilities/anim_name_utils.cpp`

**Functions to extract from existing code:**
```cpp
namespace porytiles2 {
// From project_tileset_metadata_provider.cpp (lines 41-54)
[[nodiscard]] std::pair<std::string, bool>
extract_tileset_from_callback(const std::string &callback_func);

// From anim_code_parser.cpp (lines 35-62)
[[nodiscard]] std::string extract_anim_name_from_array_ref(
    const std::string &identifier,
    const std::string &tileset_shorthand,
    bool porytiles_managed);

// From project_tileset_metadata_provider.cpp (lines 69-101)
[[nodiscard]] std::optional<std::pair<std::string, std::size_t>>
parse_anim_frame_var(
    const std::string &var_name,
    const std::string &tileset_shorthand,
    bool porytiles_managed);
}
```

**Update existing files to use shared utility:**
- `Porytiles2/lib/infra/services/anim_code_parser.cpp`
- `Porytiles2/lib/infra/repos/project_tileset_metadata_provider.cpp`

---

### Phase 2: Add Holistic Animation Loading to Reader Interface

**File:** `Porytiles2/include/porytiles2/domain/repos/tileset_artifact_reader.hpp`

**Add two new methods:**
```cpp
/**
 * @brief Reads all Porymap animations (frames + parameters) into the tileset.
 *
 * @details
 * Discovers all Porymap animations by parsing C code (generated_anim_code.h or
 * tileset_anims.c), loads frame PNGs from INCBIN paths, and extracts animation
 * parameters. This is the single entry point for loading Porymap animations.
 */
[[nodiscard]] virtual ChainableResult<void>
read_porymap_animations(Tileset &dest) const = 0;

/**
 * @brief Reads all Porytiles animations (frames + parameters) into the tileset.
 *
 * @details
 * Discovers all Porytiles animations from porytiles/anim/ folders, loads key frames
 * and numbered frames, and parses anim.yaml for parameters. This is the single entry
 * point for loading Porytiles animations.
 */
[[nodiscard]] virtual ChainableResult<void>
read_porytiles_animations(Tileset &dest) const = 0;
```

---

### Phase 3: Remove AnimationCallbackInfo from Domain Interface

**File:** `Porytiles2/include/porytiles2/domain/repos/tileset_artifact_key_provider.hpp`

**Changes:**
1. Remove line 9: `#include "porytiles2/infra/repos/animation_callback_info.hpp"`
2. Remove method at lines 200-201: `animation_callback_info_for()`

**File:** `Porytiles2/include/porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp`

**Keep `animation_callback_info_for()` as project-specific method** (not from interface):
```cpp
// Project-specific helper, not part of abstract interface
[[nodiscard]] ChainableResult<std::optional<AnimationCallbackInfo>>
animation_callback_info_for(const std::string &name) const;
```

---

### Phase 4: Implement Holistic Animation Loading in Reader

**File:** `Porytiles2/include/porytiles2/infra/repos/project_tileset_artifact_reader.hpp`

**Add dependency in constructor:**
```cpp
ProjectTilesetArtifactReader(
    // ... existing params ...
    gsl::not_null<const ProjectTilesetMetadataProvider *> metadata_provider,  // NEW
    gsl::not_null<const ProjectTilesetArtifactKeyProvider *> key_provider)    // NEW (typed)
```

**Add private members:**
```cpp
const ProjectTilesetMetadataProvider *metadata_provider_;
const ProjectTilesetArtifactKeyProvider *key_provider_;  // Concrete type for animation_callback_info_for()
```

**File:** `Porytiles2/lib/infra/repos/project_tileset_artifact_reader.cpp`

**Implement `read_porymap_animations()`:**
1. Get animation frame paths via `metadata_provider_->animation_frame_paths_for(tileset.name())`
2. Load each frame PNG using existing `read_porymap_anim_frame()` logic
3. Get callback info via `key_provider_->animation_callback_info_for(tileset.name())`
4. If callback exists, parse C code via `anim_code_parser_->parse_from_callback()`
5. Update animation params in Porymap component

**Implement `read_porytiles_animations()`:**
1. Discover animations by scanning `porytiles/anim/` directory
2. For each animation folder:
   - Load key.png if present
   - Load 0.png, 1.png, 2.png, etc.
3. Parse anim.yaml if exists and update params

---

### Phase 5: Simplify TilesetRepo::load()

**File:** `Porytiles2/lib/domain/repos/tileset_repo.cpp`

**Replace ~100 lines of animation orchestration (lines 284-397) with:**
```cpp
// Load all Porymap animations in one call
PT_TRY_VOID_CHAIN_ERR(
    reader_->read_porymap_animations(*tileset),
    "failed to load Porymap animations");

// Load all Porytiles animations in one call
PT_TRY_VOID_CHAIN_ERR(
    reader_->read_porytiles_animations(*tileset),
    "failed to load Porytiles animations");
```

**Also remove:**
- Lines 344-397: AnimationCallbackInfo handling and `read_anim_code()` call
- Lines 475-567: Porytiles animation discovery/loading loops

---

### Phase 6: Update DI Wiring

**Files to update:**
- `Porytiles2/tools/driver/command_compile_tileset.hpp`
- `Porytiles2/tools/driver/command_import_tileset.hpp`

**Changes:**
- Pass `metadata_provider` and concrete `key_provider` to `ProjectTilesetArtifactReader` constructor

---

### Phase 7: Remove Individual Frame Methods from Interface

**Remove from `TilesetArtifactReader` interface:**
- `read_porymap_anim_frame()`
- `read_porytiles_anim_frame()`
- `read_porytiles_anim_key_frame()`

**Keep as private implementation details in `ProjectTilesetArtifactReader`:**
- Move these methods to the private section or anonymous namespace
- They become internal helpers for `read_porymap_animations()` and `read_porytiles_animations()`

**Optional consolidation (per TODO comment):**
- The TODO in `project_tileset_metadata_provider.hpp` suggests merging its functionality into `ProjectTilesetArtifactKeyProvider`

---

## Critical Files Summary

| File | Changes |
|------|---------|
| `utilities/anim_name_utils.hpp` | NEW - shared parsing functions |
| `utilities/anim_name_utils.cpp` | NEW - shared parsing implementations |
| `domain/repos/tileset_artifact_reader.hpp` | Add `read_porymap_animations()`, `read_porytiles_animations()`; Remove individual frame methods |
| `domain/repos/tileset_artifact_key_provider.hpp` | Remove `AnimationCallbackInfo` include and method |
| `infra/repos/project_tileset_artifact_reader.hpp` | Add metadata_provider, key_provider dependencies |
| `infra/repos/project_tileset_artifact_reader.cpp` | Implement holistic animation loading |
| `domain/repos/tileset_repo.cpp` | Simplify to use new methods |
| `infra/services/anim_code_parser.cpp` | Use shared utility |
| `infra/repos/project_tileset_metadata_provider.cpp` | Use shared utility |

---

## Testing Strategy

1. Run existing tests after each phase to ensure no regression
2. Focus on `anim_code_parser_test.cpp` for C parsing validation
3. Test both Porymap loading strategies:
   - With `generated_anim_code.h` present
   - With only `tileset_anims.c` present
4. Test Porytiles animation loading from `porytiles/anim/` folders

---

## Order of Implementation

1. Phase 1 (shared utility) - Safe, additive change
2. Phase 2 (reader interface) - Safe, additive change
3. Phase 4 (reader implementation) - Implement new methods
4. Phase 5 (TilesetRepo simplification) - Use new methods
5. Phase 3 (remove infra leak) - After TilesetRepo no longer needs it
6. Phase 6 (DI wiring) - Update dependency injection
7. Phase 7 (cleanup) - Optional deprecation
