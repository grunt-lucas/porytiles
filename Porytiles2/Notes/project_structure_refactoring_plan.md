# Project Structure Refactoring Plan

This document consolidates the project structure and animation refactoring plans into a single master reference.

- [Project Structure Refactoring Plan](#project-structure-refactoring-plan)
  - [1. Overview \& Goals](#1-overview--goals)
    - [Key Principles](#key-principles)
  - [2. The Three-Workflow Model](#2-the-three-workflow-model)
    - [Workflow Diagram](#workflow-diagram)
    - [The Key Invariant](#the-key-invariant)
    - [Why Import Is Not TilesetRepo::load](#why-import-is-not-tilesetrepoload)
  - [3. Porytiles Utility Directory](#3-porytiles-utility-directory)
    - [Directory Structure](#directory-structure)
    - [original\_artifacts.json](#original_artifactsjson)
    - [artifact\_checksums.json](#artifact_checksumsjson)
    - [Layouts (Future)](#layouts-future)
  - [4. Configuration](#4-configuration)
    - [4.1 Configuration Locations](#41-configuration-locations)
    - [4.2 Tileset Path Configuration](#42-tileset-path-configuration)
    - [4.3 Animation Configuration](#43-animation-configuration)
  - [5. Animation System](#5-animation-system)
    - [5.1 Animation Components Overview](#51-animation-components-overview)
    - [5.2 Discovery vs Key Generation](#52-discovery-vs-key-generation)
    - [5.3 Porytiles Animation Component](#53-porytiles-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames)
      - [Artifact: Animation Parameters (anim.yaml)](#artifact-animation-parameters-animyaml)
      - [Project Key Provider](#project-key-provider)
      - [Project Reader](#project-reader)
    - [5.4 Porymap Animation Component](#54-porymap-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames-1)
      - [Artifact: Animation Parameters](#artifact-animation-parameters)
      - [Project Key Provider](#project-key-provider-1)
      - [Project Reader](#project-reader-1)
      - [Project Writer](#project-writer)
  - [6. VanillaAnimationImporter Service](#6-vanillaanimationimporter-service)
    - [Purpose](#purpose)
    - [Interface](#interface)
    - [Implementation](#implementation)
    - [Usage in ImportUseCase](#usage-in-importusecase)
  - [7. Import Workflow](#7-import-workflow)
    - [7.1 Pre-Import Validation](#71-pre-import-validation)
    - [7.2 Project State: Before \& After](#72-project-state-before--after)
      - [Before Import](#before-import)
      - [After Import](#after-import)
      - [File Structure After Import](#file-structure-after-import)
    - [7.3 Error Handling \& Rollback](#73-error-handling--rollback)
  - [8. Restore Workflow](#8-restore-workflow)
    - [8.1 Default Behavior (Headers Only)](#81-default-behavior-headers-only)
    - [8.2 Full Cleanup (--full flag)](#82-full-cleanup---full-flag)
    - [8.3 Error Cases](#83-error-cases)
  - [9. Preconditions \& Invariants](#9-preconditions--invariants)
    - [9.1 Animation Naming Conventions](#91-animation-naming-conventions)
    - [9.2 Path Conventions](#92-path-conventions)
    - [9.3 Variable Naming Convention](#93-variable-naming-convention)
  - [10. Implementation Status](#10-implementation-status)
    - [10.1 Complete](#101-complete)
    - [10.2 Needed](#102-needed)
  - [Appendix A: Code Examples](#appendix-a-code-examples)
    - [TilesetRepo::load Animation Snippet](#tilesetrepoload-animation-snippet)
    - [TilesetRepo::save Porymap Animation Snippet](#tilesetreposave-porymap-animation-snippet)
  - [Appendix B: Summary Tables](#appendix-b-summary-tables)
    - [Discovery vs Key Generation](#discovery-vs-key-generation)
    - [Animation Configuration Behavior](#animation-configuration-behavior)
    - [Operation vs Path Types](#operation-vs-path-types)

---

## 1. Overview & Goals

This refactoring establishes a clean, predictable structure for Porytiles-managed tilesets within pokeemerald projects. The goal is to enable bidirectional workflows (compile and decompile) while keeping original vanilla assets untouched and restorable.

### Key Principles

1. **Deterministic Paths**: After import, all artifact locations are computed from tileset name and configuration—no parsing required.
2. **Surgical Changes**: Porytiles only modifies what it needs to. Original tileset assets remain intact alongside managed assets.
3. **Easy Restore**: Users can undo Porytiles management and return to vanilla state at any time.
4. **Clear Source of Truth**: Each component (Porytiles vs Porymap) has an explicit source of truth for discovery.

---

## 2. The Three-Workflow Model

Porytiles operates in three distinct workflows:

| Workflow | Frequency | Description |
|----------|-----------|-------------|
| **Import** | One-time | Migrate vanilla tileset to Porytiles-managed state |
| **Compile** | Repeatable | Transform Porytiles source → Porymap binary assets |
| **Decompile** | Repeatable | Transform Porymap binary → Porytiles source assets |

### Workflow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         IMPORT (one-time)                       │
│  Vanilla tileset → VanillaImporter → TilesetRepo::save          │
│  (scattered paths)    (reads chaos)    (writes order)           │
└─────────────────────────────────────────────────────────────────┘
                                ↓
                  Tileset is now "Porytiles-managed"
                                ↓
┌─────────────────────────────────────────────────────────────────┐
│                    COMPILE (repeatable)                         │
│  TilesetRepo::load(porytiles) → transform → TilesetRepo::save   │
│  (deterministic)                           (deterministic)      │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                   DECOMPILE (repeatable)                        │
│  TilesetRepo::load(porymap) → transform → TilesetRepo::save     │
│  (deterministic)                         (deterministic)        │
└─────────────────────────────────────────────────────────────────┘
```

**Note:** Compile and decompile operate on Porytiles-managed assets ONLY. These operations never touch the original vanilla assets, making restoration trivial.

### The Key Invariant

> **Once Import completes, the tileset is "Porytiles-managed" and all key generation is deterministic.**

This means:
- `anim.yaml` exists → Porytiles component can be discovered/loaded
- `generated_anim_code.h` exists → Porymap component can be discovered/loaded
- All frames are at deterministic paths → `key_for_*_anim_frame` works for both read and write

### Why Import Is Not TilesetRepo::load

The standard `TilesetRepo::load` assumes discovered artifacts exist at deterministic key locations. For vanilla imports, frames are scattered wherever INCBIN declarations point—not at deterministic paths.

| Operation | Source Paths | Destination Paths |
|-----------|--------------|-------------------|
| **Import** | Scattered (parsed from INCBINs) | Deterministic |
| **Load** | Deterministic | N/A (in-memory) |
| **Save** | N/A (in-memory) | Deterministic |

If we added `load_vanilla_first_time` to TilesetRepo:
1. **Wrong abstraction level** — "vanilla" is a use-case concept, not a persistence concept
2. **Leaky abstraction** — TilesetRepo would need to know about INCBIN parsing, `tileset_anims.c`, etc.
3. **Breaks SRP** — TilesetRepo's job is artifact persistence with deterministic keys, not migration

This follows the **Repository Pattern** correctly: repositories provide a collection-like interface for domain objects. They shouldn't know about data migration or transformation between different storage formats.

---

## 3. Porytiles Utility Directory

Porytiles creates a `porytiles/` directory in the pokeemerald project to store configurations and metadata.

### Directory Structure

```
pokeemerald/
├─ porytiles/
│  ├─ config.yaml              # Project-global config
│  ├─ config.local.yaml        # Local overrides (gitignored)
│  ├─ tilesets/
│  │  ├─ gTileset_General/     # Directory names use exact tileset name from headers.h
│  │  │  ├─ config.yaml        # Tileset-specific config (optional)
│  │  │  ├─ config.local.yaml  # Tileset local overrides (gitignored)
│  │  │  ├─ artifact_checksums.json
│  │  │  ├─ original_artifacts.json
│  │  ├─ gTileset_PorytilesTest/
│  │  │  ├─ ...
│  ├─ layouts/                 # Future: layout management
```

### original_artifacts.json

This file is the **source of truth** for whether a tileset is Porytiles-managed. Its presence indicates the tileset has been imported.

```json
{
    "version": 1,
    ".tiles": "gTilesetTiles_General",
    ".palettes": "gTilesetPalettes_General",
    ".metatiles": "gMetatiles_General",
    ".metatileAttributes": "gMetatileAttributes_General",
    ".callback": "InitTilesetAnim_General"
}
```

**Purpose:**
- Stores the original field values from `src/data/tilesets/headers.h` before import modified them
- Enables `restore-tileset` command to revert changes
- Includes `version` field for future format migrations

**Benefits:**
- No brittle parsing of `.callback` field to detect managed status
- Simple file existence check: `original_artifacts.json` exists = managed tileset
- Easy restore: just read the original values and write them back

For tilesets created via `create-tileset` (not imported from vanilla), there is no `original_artifacts.json` file, so `restore-tileset` will error with a clear message.

### artifact_checksums.json

Stores checksums for all Porytiles-managed artifacts. Used to detect changes and skip unnecessary recompilation.

`ProjectArtifactChecksumProvider::cache_tileset_checksums` writes here instead of the tileset root. Key paths are relativized against the project root.

### Layouts (Future)

Layout management will work similarly. Details TBD.

---

## 4. Configuration

### 4.1 Configuration Locations

Porytiles uses layered configuration with increasing specificity:

| Level | Location | Purpose |
|-------|----------|---------|
| Project-wide | `porytiles/config.yaml` | Default settings for all tilesets |
| Tileset-specific | `porytiles/tilesets/{name}/config.yaml` | Override for specific tileset |
| Local (gitignored) | `*.local.yaml` variants | Machine-specific overrides |

**Note:** `anim.yaml` is for pokeemerald engine parameters (frame timing, frame lists), NOT Porytiles configuration.

### 4.2 Tileset Path Configuration

These config values control where Porytiles places managed assets:

```yaml
tileset:
  paths:
    primary:
      src: data/tilesets/primary   # Porytiles-format assets (porytiles_src/)
      bin: data/tilesets/primary   # Porymap-format assets (porytiles_bin/)
    secondary:
      src: data/tilesets/secondary
      bin: data/tilesets/secondary
```

**Asset Placement:**
- Porytiles-format: `{tileset.paths.primary.src}/{tileset_name}/porytiles_src/`
- Porymap-format: `{tileset.paths.primary.bin}/{tileset_name}/porytiles_bin/`

### 4.3 Animation Configuration

```yaml
animation:
  overwrite_callback: true  # Default: Porytiles manages callback field
```

| overwrite_callback | Writes Frames | Generates Code | Updates .callback |
|-------------------|---------------|----------------|-------------------|
| `true` (default) | Yes | Yes | Yes |
| `false` | Yes | Yes | **No** |

**Use Case for `false`:** Users who want Porytiles to compile animation frames and generate code, but want to wire their own custom callback function in `tileset_anims.c`.

Key generation is **always deterministic**—Porytiles controls where animation frames are written regardless of this setting.

---

## 5. Animation System

### 5.1 Animation Components Overview

Each tileset has two animation representations:

| Component | Source of Truth | Contains |
|-----------|-----------------|----------|
| **Porytiles** | `anim.yaml` | Key frames, named frames, timing parameters |
| **Porymap** | `generated_anim_code.h` | Indexed frames (0, 1, 2...), C code for engine |

**Compile** transforms Porytiles → Porymap.
**Decompile** transforms Porymap → Porytiles.

### 5.2 Discovery vs Key Generation

These are **orthogonal concerns**:

| Concern | Question | Implementation |
|---------|----------|----------------|
| **Discovery** | "WHAT animations/frames exist?" | Scan component's source of truth |
| **Key Generation** | "WHERE should I read/write this artifact?" | Always deterministic |

The Revision 2 gap was conflating these: `key_for_porymap_anim_frame` tried to parse INCBIN paths (discovery) for key generation. This failed for new animations that hadn't been written yet.

**The Fix:** Key generation is always deterministic. Discovery methods scan the source of truth to find what exists.

### 5.3 Porytiles Animation Component

The entire `Animation` object can be constructed from the supplied frames and `anim.yaml`. Key frames are required for Porytiles component animations.

#### Artifact: Animation Frames

Location: `porytiles/anim/{anim_name}/{frame_name}.png`

- Error if no `key.png` frame exists
- Frame names can be user-defined (not just numbers)

#### Artifact: Animation Parameters (anim.yaml)

Location: `porytiles/anim/anim.yaml`

```yaml
# Animation with named frames
flower:
  frames: ["center", "left", "center", "right"]

# Animation with numbered frames
water:
  frame_offset: 1
  frames: ["0", "1", "2", "3"]

# INVALID: MyAnim fails because to_snake_case("MyAnim") != "MyAnim"
# MyAnim:
#   frames: ["0", "1"]
```

Top-level keys are animation names. The `frames` array lists all non-key frames in playback order.

#### Project Key Provider

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) {
    // Read isSecondary from headers.h to determine {primary,secondary}
    // Parse anim.yaml at: data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    // Return all top-level keys
    // Error if any key doesn't satisfy: to_snake_case(key) == key
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) {
    // Extract values from anim.yaml "frames" array under anim_name
    // Manually include "key" frame
    auto frames = extract_yaml_array(anim_name + ".frames").to_set();
    frames.insert("key");
    return frames;
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    // DETERMINISTIC: {tileset_path}/porytiles/anim/{anim_name}/{frame_name}.png
    return ArtifactKey{tileset_path / "porytiles" / "anim" / anim_name / (frame_name + ".png")};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_params(const std::string &tileset_name) {
    // DETERMINISTIC: {tileset_path}/porytiles/anim/anim.yaml
    return ArtifactKey{tileset_path / "porytiles" / "anim" / "anim.yaml"};
}
```

#### Project Reader

TilesetRepo discovers anims and frames via the key provider, then passes them to the reader:

```c++
ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const ArtifactKey &key_frame_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params from anim.yaml
    // Read key frame (required)
    // Read all other frames
    // Panic if params_key doesn't exist
    // Panic if key_frame_key doesn't exist
    // Panic if any expected frame doesn't exist
}
```

### 5.4 Porymap Animation Component

The entire `Animation` object can be constructed from `include/generated_anim_code.h`. For vanilla import (before first compile), parameters are read from `tileset_anims.c`.

**Invariant:** `generated_anim_code.h` uses `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}` for frame array names. Users importing vanilla tilesets must follow this convention in `tileset_anims.c`.

#### Artifact: Animation Frames

Location: `anim/{anim_name}/{frame_name}.png`

Frame names are typically numeric (0.png, 1.png, etc.) for Porymap compatibility.

#### Artifact: Animation Parameters

Location: `include/generated_anim_code.h`

For vanilla import (first-time), read from `tileset_anims.c` callback implementations.

#### Project Key Provider

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) {
    // Try generated_anim_code.h first
    if (exists(generated_anim_code_path)) {
        // Parse gTilesetAnims_{TilesetName}_{AnimName} variables
        // Return animation names as snake_case
        return read_g_tileset_anims_vars(path).map(to_snake_case);
    }
    // Fall back to vanilla tileset_anims.c
    return read_vanilla_g_tileset_anims_vars(tileset_name).map(to_snake_case);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) {
    // Similar: try generated_anim_code.h, fall back to tileset_anims.c
    // Extract frame names from INCBIN declarations (basename without extension)
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    // DETERMINISTIC: {tileset_path}/anim/{anim_name}/{frame_name}.png
    return ArtifactKey{tileset_path / "anim" / anim_name / (frame_name + ".png")};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) {
    // DETERMINISTIC: {tileset_path}/include/generated_anim_code.h
    return ArtifactKey{tileset_path / "include" / "generated_anim_code.h"};
}
```

#### Project Reader

```c++
ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // If params_key doesn't exist, try reading from vanilla tileset_anims.c
    // Return FormattableError if expected frame doesn't exist
}
```

#### Project Writer

Key generation is always deterministic, so writing works reliably:

```c++
// TilesetRepo::save snippet for Porymap animations
for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
    for (std::size_t i = 0; i < porymap_anim.frame_count(); i++) {
        const auto frame_name = std::to_string(i);
        // This succeeds because key_for_porymap_anim_frame is deterministic
        PT_TRY_ASSIGN(frame_key,
            key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), frame_name));
        PT_TRY(writer_->write_porymap_anim_frame(frame_key, tileset, porymap_anim.name(), frame_name));
    }
}
```

---

## 6. VanillaAnimationImporter Service

### Purpose

Isolate all "discovery chaos" for first-time vanilla imports to a dedicated service. This keeps TilesetRepo clean and focused on deterministic operations.

### Interface

```c++
// Located in infra layer (does I/O: parses C files, reads PNGs)
class VanillaAnimationImporter {
  public:
    /**
     * @brief Import animations from a vanilla tileset's tileset_anims.c
     *
     * @details
     * Parses tileset_anims.c to discover animation names, frame paths, and parameters.
     * Produces Animation<Rgba32> objects that can be added to a Tileset's Porytiles component.
     *
     * @param tileset_name The name of the tileset to import animations from
     * @return Map of animation names to Animation<Rgba32> objects
     * @pre Tileset must exist and have animations defined in tileset_anims.c
     * @pre Animation arrays must follow gTilesetAnims_{TilesetName}_{AnimName} naming convention
     */
    ChainableResult<std::map<std::string, Animation<Rgba32>>>
    import_animations(const std::string &tileset_name);
};
```

### Implementation

```c++
ChainableResult<std::map<std::string, Animation<Rgba32>>>
VanillaAnimationImporter::import_animations(const std::string &tileset_name) {
    std::map<std::string, Animation<Rgba32>> result;

    // 1. Parse tileset_anims.c to find all gTilesetAnims_{TilesetName}_* arrays
    const auto anim_names = parse_vanilla_anim_names(tileset_name);

    for (const auto &anim_name : anim_names) {
        // 2. For each animation, find frame INCBIN paths
        const auto frame_paths = parse_vanilla_frame_paths(tileset_name, anim_name);

        // 3. Read frame PNG files from discovered paths
        std::vector<AnimationFrame<Rgba32>> frames;
        for (const auto &path : frame_paths) {
            const auto frame = read_png_as_frame(path);
            frames.push_back(frame);
        }

        // 4. Parse animation parameters from callback code
        const auto params = parse_vanilla_anim_params(tileset_name, anim_name);

        // 5. Construct Animation object
        Animation<Rgba32> animation{anim_name, params, frames};
        result[anim_name] = std::move(animation);
    }

    return result;
}
```

### Usage in ImportUseCase

```c++
// First-time import workflow
if (!key_provider_->artifact_exists(porytiles_anim_params_key)) {
    // No anim.yaml exists - this is a first-time import
    const auto imported_anims = vanilla_importer_->import_animations(tileset_name);
    for (const auto &[name, anim] : imported_anims) {
        tileset->porytiles_component().add_anim(name, anim);
    }
    // After import, tileset is now "managed" and uses deterministic paths
}
```

**Architecture:**

```
ImportUseCase (app layer)
    ├── VanillaAnimationImporter (infra) ← reads from scattered paths
    ├── TilesetFactory (domain)          ← creates empty tileset
    └── TilesetRepo (infra)              ← saves to deterministic paths
```

VanillaAnimationImporter is self-contained:
- Lives in `infra/` because it does I/O
- Has its **own internal path-finding logic** for INCBIN parsing
- Does NOT use `ProjectTilesetArtifactKeyProvider`—it predates the "managed" state
- Returns in-memory objects with frame data already loaded

---

## 7. Import Workflow

### 7.1 Pre-Import Validation

Before import begins, validate:

1. **Tileset exists**: Entry for tileset_name must exist in `src/data/tilesets/headers.h`
2. **Not already managed**: `original_artifacts.json` must NOT exist (unless `--force` flag provided)
3. **Animation naming**: Animation arrays in `tileset_anims.c` must follow `gTilesetAnims_{TilesetName}_{AnimName}` convention

**Error handling:**
- If tileset doesn't exist → error with suggested tilesets
- If already managed → error suggesting `--force` to re-import
- If animation naming is wrong → error with expected format

### 7.2 Project State: Before & After

#### Before Import

`src/data/tilesets/headers.h`:
```c++
const struct Tileset gTileset_General =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
    .metatiles = gMetatiles_General,
    .metatileAttributes = gMetatileAttributes_General,
    .callback = InitTilesetAnim_General,
};
```

#### After Import

`src/data/tilesets/headers.h`:
```c++
const struct Tileset gTileset_General =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_PorytilesManaged_General,
    .palettes = gTilesetPalettes_PorytilesManaged_General,
    .metatiles = gMetatiles_PorytilesManaged_General,
    .metatileAttributes = gMetatileAttributes_PorytilesManaged_General,
    .callback = InitTilesetAnim_PorytilesManaged_General,
};
```

`src/data/tilesets/graphics.h`:
```c++
const u32 gTilesetTiles_PorytilesManaged_General[] = INCBIN_U32("data/tilesets/primary/general/porytiles_bin/tiles.4bpp.lz");

const u16 gTilesetPalettes_PorytilesManaged_General[][16] =
{
    INCBIN_U16("data/tilesets/primary/general/porytiles_bin/palettes/00.gbapal"),
    // ...
    INCBIN_U16("data/tilesets/primary/general/porytiles_bin/palettes/15.gbapal"),
};
```

`src/data/tilesets/metatiles.h`:
```c++
const u16 gMetatiles_PorytilesManaged_General[] = INCBIN_U16("data/tilesets/primary/general/porytiles_bin/metatiles.bin");
const u16 gMetatileAttributes_PorytilesManaged_General[] = INCBIN_U16("data/tilesets/primary/general/porytiles_bin/metatile_attributes.bin");
```

#### File Structure After Import

```
data/
└── tilesets/
    └── primary/
        └── general/
            ├── anim/                    # Original (untouched)
            │   └── ...
            ├── metatiles.bin            # Original (untouched)
            ├── metatile_attributes.bin  # Original (untouched)
            ├── tiles.png                # Original (untouched)
            ├── palettes/                # Original (untouched)
            │   ├── 00.pal
            │   └── ...
            ├── porytiles_src/           # NEW: Porytiles format
            │   ├── anim/
            │   │   ├── anim.yaml
            │   │   └── flower/
            │   │       ├── key.png
            │   │       ├── 0.png
            │   │       └── 1.png
            │   ├── bottom.png
            │   ├── middle.png
            │   ├── top.png
            │   └── attributes.csv
            └── porytiles_bin/           # NEW: Porymap format (compiled)
                ├── anim/
                │   └── flower/
                │       ├── 0.png
                │       └── 1.png
                ├── metatiles.bin
                ├── metatile_attributes.bin
                ├── tiles.png
                └── palettes/
                    ├── 00.pal
                    └── ...
```

### 7.3 Error Handling & Rollback

Import should be atomic with transaction semantics:

1. **Collect all changes** before writing anything
2. **Write to staging** (temporary locations or in-memory)
3. **Commit all at once** or **rollback on any failure**

If import fails partway through:
- Don't leave `original_artifacts.json` written (tileset would appear managed but be broken)
- Don't leave partial header modifications
- Provide clear error message indicating what failed and why

---

## 8. Restore Workflow

The `restore-tileset` command reverts a Porytiles-managed tileset to its vanilla state.

### 8.1 Default Behavior (Headers Only)

Default behavior restores header files but preserves managed directories:

1. Read `original_artifacts.json`
2. Restore `src/data/tilesets/headers.h` with original field values
3. Remove INCBIN declarations added to `graphics.h` and `metatiles.h`
4. Delete `original_artifacts.json`

**Result:** Tileset compiles using original vanilla assets. `porytiles_src/` and `porytiles_bin/` remain for potential re-import.

### 8.2 Full Cleanup (--full flag)

With `--full`, also delete managed directories:

1. All steps from default behavior
2. Delete `{tileset_path}/porytiles_src/`
3. Delete `{tileset_path}/porytiles_bin/`
4. Delete `porytiles/tilesets/{tileset_name}/` utility directory

**Result:** Complete removal of all Porytiles traces for this tileset.

### 8.3 Error Cases

| Condition | Behavior |
|-----------|----------|
| No `original_artifacts.json` | Error: "Tileset is not Porytiles-managed" |
| Tileset created via `create-tileset` | Error: "Cannot restore tileset created by Porytiles (no original to restore)" |
| Original vanilla files deleted | Warning: "Original files not found, headers restored but tileset may not compile" |

---

## 9. Preconditions & Invariants

Porytiles minimizes required conventions while maximizing their effectiveness.

### 9.1 Animation Naming Conventions

Animation names have two forms:

| Form | Example | Used For |
|------|---------|----------|
| **Canonical** | `flower_red` | anim.yaml keys, diagnostic messages, internal maps |
| **PascalCase** | `FlowerRed` | C variable names: `gTilesetAnims_{TilesetName}_{AnimName}` |

**Requirement:** Canonical name MUST be snake_case: `to_snake_case(canonical) == canonical`

**Involution Property:** The snake_case ↔ PascalCase transformations must be lossless:
```
to_snake_case(to_pascal_case("flower_red")) == "flower_red"
to_pascal_case(to_snake_case("FlowerRed")) == "FlowerRed"
```

This enables lossless compile/decompile round-trips.

### 9.2 Path Conventions

| Component | Convention |
|-----------|------------|
| Porytiles frames | `porytiles/anim/{anim_name}/{frame_name}.png` |
| Porymap frames | `anim/{anim_name}/{frame_name}.png` |

The absolute location of `porytiles/` is configurable, but the internal structure is fixed.

### 9.3 Variable Naming Convention

Animation frame arrays in C code must follow:
```
gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}
```

Examples:
- `gTilesetAnims_General_Flower`
- `gTilesetAnims_General_Flower_Frame0`
- `gTilesetAnims_General_Water`

Users importing vanilla tilesets must ensure their `tileset_anims.c` follows this convention.

---

## 10. Implementation Status

### 10.1 Complete

- **TilesetRepo** with load/save/exists
- **ProjectTilesetArtifactKeyProvider** with key_for_* and discover_* methods
- **Animation models** (Animation<T>, AnimationParams, AnimationFrame)
- **Config system** with layered approach
- **anim.yaml parser**

### 10.2 Needed

- [ ] Tileset path config values (`tileset.paths.primary.src/bin`, etc.)
- [ ] `animation.overwrite_callback` config value
- [ ] VanillaAnimationImporter service
- [ ] `porytiles/` utility directory support
- [ ] `original_artifacts.json` handling
- [ ] `restore-tileset` command
- [ ] Pre-import validation
- [ ] Atomic import with rollback

---

## Appendix A: Code Examples

### TilesetRepo::load Animation Snippet

```c++
// Only read animations if params artifact file exists (source of truth)
if (key_provider_->artifact_exists(porytiles_params_key)) {
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_anims,
        key_provider_->discover_porytiles_anims(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);

    for (const auto &porytiles_anim_name : porytiles_anims) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            key_frame_key,
            key_provider_->key_for_porytiles_anim_key_frame(tileset->name(), porytiles_anim_name),
            "tileset load failed",
            std::unique_ptr<Tileset>);

        if (!key_provider_->artifact_exists(key_frame_key)) {
            return FormattableError{missing_required_artifact_msg, FormatParam{key_frame_key.key(), Style::bold}};
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porytiles_anim_frames(tileset->name(), porytiles_anim_name),
            "tileset load failed",
            std::unique_ptr<Tileset>);

        std::vector<ArtifactKey> frames_keys{};
        for (const auto &frame : frames) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porytiles_anim_frame(tileset->name(), porytiles_anim_name, frame),
                "tileset load failed",
                std::unique_ptr<Tileset>);

            if (!key_provider_->artifact_exists(frame_key)) {
                return FormattableError{missing_required_artifact_msg, FormatParam{frame_key.key(), Style::bold}};
            }
            frames_keys.push_back(frame_key);
        }

        const auto anim_result = reader_->read_porytiles_anim(
            *tileset, porytiles_anim_name, porytiles_params_key, frames_keys);
    }
}
```

### TilesetRepo::save Porymap Animation Snippet

```c++
for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
    for (std::size_t i = 0; i < porymap_anim.frame_count(); i++) {
        const auto frame_name = std::to_string(i);
        // Deterministic key generation - always works
        PT_TRY_ASSIGN_CHAIN_ERR(
            frame_key,
            key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), frame_name),
            "tileset save failed",
            void);

        if (auto result = writer_->write_porymap_anim_frame(
                frame_key, tileset, porymap_anim.name(), frame_name);
            !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }
}
```

---

## Appendix B: Summary Tables

### Discovery vs Key Generation

| Method | Discovery Source | Key Generation |
|--------|-----------------|----------------|
| `discover_porytiles_anims` | Scan `anim.yaml` | N/A |
| `discover_porytiles_anim_frames` | Scan `anim.yaml` | N/A |
| `discover_porymap_anims` | Scan `generated_anim_code.h` or `tileset_anims.c` | N/A |
| `discover_porymap_anim_frames` | Scan `generated_anim_code.h` or `tileset_anims.c` | N/A |
| `key_for_porytiles_anim_frame` | N/A | **Always Deterministic** |
| `key_for_porymap_anim_frame` | N/A | **Always Deterministic** |

### Animation Configuration Behavior

| overwrite_callback | Writes Frames | Generates Code | Updates .callback |
|-------------------|---------------|----------------|-------------------|
| `true` (default) | Yes | Yes | Yes |
| `false` | Yes | Yes | **No** |

### Operation vs Path Types

| Operation | Source Paths | Destination Paths |
|-----------|--------------|-------------------|
| **Import** | Scattered (INCBIN) | Deterministic |
| **Load** | Deterministic | N/A (in-memory) |
| **Save** | N/A (in-memory) | Deterministic |
