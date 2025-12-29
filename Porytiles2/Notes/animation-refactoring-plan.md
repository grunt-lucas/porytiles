# Animation Loading Refactoring Plan (Revised)

## Goal

Refactor TilesetRepo and affiliated helper services so that Animations are a first-class concept with coherent loading.

## Problems Being Solved

1. **Infra leaking into domain**: `AnimationCallbackInfo` (infra concept with C file paths) is exposed in `TilesetArtifactKeyProvider` domain interface
2. **Incoherent discovery**: Frames discovered separately from parameters via different code paths
3. **Duplicated logic**: Animation name parsing exists in both `AnimCodeParser` and `ProjectTilesetMetadataProvider`
4. **Complex orchestration**: TilesetRepo::load() has ~100 lines of animation loading logic

## Key Constraint: Checksum Compatibility

**CRITICAL**: `ArtifactChecksumProvider` uses `key_provider_->get_all_artifact_keys()` which calls `discover_*_anims()` methods. Discovery MUST stay in KeyProvider, not move to Reader, or checksums break.

Each animation artifact (frames, key frames, anim.yaml, generated_anim_code.h) needs its own `ArtifactKey` for per-artifact checksumming to work.

## Solution: AnimationLoadingOrchestrator

Instead of holistic `read_*_animations()` methods in Reader (which would break checksums by hiding discovery), create an **Orchestrator** that:

1. Uses KeyProvider for discovery (checksums still work!)
2. Uses Reader for individual frame reads (per-artifact granularity preserved!)
3. TilesetRepo gets simplified to 2 method calls

```
TilesetRepo (domain)
    │
    ▼
AnimationLoadingOrchestrator (domain interface)
    │
    ▼
ProjectAnimationLoadingOrchestrator (infra implementation)
    │
    ├─── calls ──► KeyProvider.discover_*_anims()      // Discovery preserved!
    ├─── calls ──► KeyProvider.key_for_*_anim_frame()  // Keys preserved!
    └─── calls ──► Reader.read_*_anim_frame()          // Per-artifact reads!
```

---

## Implementation Phases

### Phase 1: Create Shared Animation Name Parsing Utility

**New Files:**
- `Porytiles2/include/porytiles2/utilities/anim_name_utils.hpp`
- `Porytiles2/lib/utilities/anim_name_utils.cpp`

**Functions to extract from existing code:**
```c++
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

### Phase 2: Create AnimationLoadingOrchestrator Interface (Domain Layer)

**New File:** `Porytiles2/include/porytiles2/domain/repos/animation_loading_orchestrator.hpp`

```c++
namespace porytiles2 {

/**
 * @brief Abstract interface for orchestrating animation loading.
 *
 * @details
 * Coordinates discovery of animations (via KeyProvider) with reading of
 * individual frames (via Reader). Preserves per-artifact granularity for
 * checksum compatibility while simplifying TilesetRepo.
 */
class AnimationLoadingOrchestrator {
  public:
    virtual ~AnimationLoadingOrchestrator() = default;

    [[nodiscard]] virtual ChainableResult<void>
    load_porymap_animations(Tileset &dest) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    load_porytiles_animations(Tileset &dest) const = 0;
};

} // namespace porytiles2
```

---

### Phase 3: Implement ProjectAnimationLoadingOrchestrator (Infra Layer)

**New Files:**
- `Porytiles2/include/porytiles2/infra/repos/project_animation_loading_orchestrator.hpp`
- `Porytiles2/lib/infra/repos/project_animation_loading_orchestrator.cpp`

**Implementation:**
- Depends on `ProjectTilesetArtifactKeyProvider` (concrete, for `animation_callback_info_for()`)
- Depends on `TilesetArtifactReader` (abstract)
- `load_porymap_animations()`: Calls `discover_porymap_anims()`, then `read_porymap_anim_frame()` for each frame
- `load_porytiles_animations()`: Calls `discover_porytiles_anims()`, then reads key frames and numbered frames

**Extract from TilesetRepo::load():**
- Lines 284-340: Porymap animation frame loading loop
- Lines 475-552: Porytiles animation frame loading loop

---

### Phase 4: Update Reader to Internally Resolve Callback Info

**File:** `Porytiles2/include/porytiles2/infra/repos/project_tileset_artifact_reader.hpp`

**Add constructor parameter:**
```c++
ProjectTilesetArtifactReader(
    // ... existing params ...
    gsl::not_null<const ProjectTilesetArtifactKeyProvider *> key_provider);  // NEW
```

**Add private member:**
```c++
const ProjectTilesetArtifactKeyProvider *key_provider_;
```

**File:** `Porytiles2/lib/infra/repos/project_tileset_artifact_reader.cpp`

**Update `read_anim_code()`:** Now internally calls `key_provider_->animation_callback_info_for()` instead of receiving callback info externally.

---

### Phase 5: Remove AnimationCallbackInfo from Domain Interface

**File:** `Porytiles2/include/porytiles2/domain/repos/tileset_artifact_key_provider.hpp`

**Remove:**
1. Line 9: `#include "porytiles2/infra/repos/animation_callback_info.hpp"`
2. Lines 200-201: `animation_callback_info_for()` method

**File:** `Porytiles2/include/porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp`

**Keep** `animation_callback_info_for()` as a non-virtual, concrete method (project-specific, not from interface):
```c++
// Project-specific helper, NOT part of abstract interface
[[nodiscard]] ChainableResult<std::optional<AnimationCallbackInfo>>
animation_callback_info_for(const std::string &name) const;
```

---

### Phase 6: Simplify TilesetRepo::load()

**File:** `Porytiles2/include/porytiles2/domain/repos/tileset_repo.hpp`

**Add dependency:**
```c++
TilesetRepo(
    // ... existing params ...
    gsl::not_null<const AnimationLoadingOrchestrator *> anim_orchestrator);  // NEW
```

**File:** `Porytiles2/lib/domain/repos/tileset_repo.cpp`

**Replace ~100 lines of animation orchestration (lines 284-397, 475-567) with:**
```c++
// Load all Porymap animations in one call
PT_TRY_VOID_CHAIN_ERR(
    anim_orchestrator_->load_porymap_animations(*tileset),
    "failed to load Porymap animations");

// ... (other Porymap artifact loading) ...

// Load all Porytiles animations in one call
PT_TRY_VOID_CHAIN_ERR(
    anim_orchestrator_->load_porytiles_animations(*tileset),
    "failed to load Porytiles animations");
```

**Remove:**
- AnimationCallbackInfo handling (lines 383-397)
- Direct discovery loops for animations

---

### Phase 7: Update DI Wiring

**Files to update:**
- `Porytiles2/tools/driver/command_compile_tileset.hpp`
- `Porytiles2/tools/driver/command_import_tileset.hpp` (if exists)

**Changes:**
1. Pass `key_provider` to `ProjectTilesetArtifactReader`
2. Create `ProjectAnimationLoadingOrchestrator`
3. Pass orchestrator to `TilesetRepo`

---

## Implementation Order

1. Phase 1 (shared utility) - Safe, additive
2. Phase 2 (domain interface) - Safe, additive
3. Phase 3 (infra implementation) - Can test independently
4. Phase 4 (Reader gets KeyProvider) - Prerequisite for Phase 5
5. Phase 5 (remove infra leak) - After Phase 4
6. Phase 6 (TilesetRepo simplification) - Uses orchestrator
7. Phase 7 (DI wiring) - Final integration

---

## Critical Files Summary

| File | Changes |
|------|---------|
| `utilities/anim_name_utils.hpp` | NEW - shared parsing functions |
| `utilities/anim_name_utils.cpp` | NEW - shared parsing implementations |
| `domain/repos/animation_loading_orchestrator.hpp` | NEW - abstract interface |
| `infra/repos/project_animation_loading_orchestrator.hpp` | NEW - implementation |
| `infra/repos/project_animation_loading_orchestrator.cpp` | NEW - implementation |
| `domain/repos/tileset_artifact_key_provider.hpp` | Remove `AnimationCallbackInfo` include and method |
| `infra/repos/project_tileset_artifact_reader.hpp` | Add key_provider_ member |
| `infra/repos/project_tileset_artifact_reader.cpp` | Update read_anim_code() |
| `domain/repos/tileset_repo.hpp` | Add orchestrator dependency |
| `domain/repos/tileset_repo.cpp` | Simplify to use orchestrator |
| `tools/driver/command_compile_tileset.hpp` | Update DI wiring |
| `infra/services/anim_code_parser.cpp` | Use shared utility |
| `infra/repos/project_tileset_metadata_provider.cpp` | Use shared utility |

---

## Testing Strategy

1. Run existing tests after each phase to ensure no regression
2. Focus on `anim_code_parser_test.cpp` for C parsing validation
3. Verify checksum behavior:
   - Load a tileset with animations
   - Save it
   - Verify `artifact_checksums.json` contains individual frame entries
4. Test both Porymap and Porytiles animation loading paths

---

## Why This Plan Works

1. **Checksum Compatible**: Discovery stays in KeyProvider via `discover_*_anims()` methods. Orchestrator calls KeyProvider methods, doesn't bypass them.

2. **No Infra Leak**: `AnimationCallbackInfo` removed from domain interface. Only concrete `ProjectTilesetArtifactKeyProvider` has it.

3. **Layer Purity**: `AnimationLoadingOrchestrator` is abstract in domain, concrete in infra. TilesetRepo stays pure domain.

4. **TilesetRepo Simplified**: ~100 lines of animation orchestration replaced with 2 method calls.
