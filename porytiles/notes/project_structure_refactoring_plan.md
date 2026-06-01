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
      - [Frame Name Preservation](#frame-name-preservation)
      - [Project Key Provider](#project-key-provider)
      - [Project Reader](#project-reader)
    - [5.4 Porymap Animation Component](#54-porymap-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames-1)
      - [Artifact: Animation Parameters](#artifact-animation-parameters)
      - [Project Key Provider](#project-key-provider-1)
      - [Project Reader](#project-reader-1)
      - [Project Writer](#project-writer)
  - [6. ProjectVanillaAnimImporter Service](#6-projectvanillaanimimporter-service)
    - [Purpose](#purpose)
    - [Interface](#interface)
    - [Implementation](#implementation)
    - [Why IndexPixel Instead of Rgba32?](#why-indexpixel-instead-of-rgba32)
    - [Key Frames Are NOT Extracted](#key-frames-are-not-extracted)
    - [Usage in ImportUseCase](#usage-in-importusecase)
  - [7. Import Workflow](#7-import-workflow)
    - [7.1 Pre-Import Validation](#71-pre-import-validation)
    - [7.2 Project State: Before \& After](#72-project-state-before--after)
      - [Before Import](#before-import)
      - [After Import](#after-import)
      - [File Structure After Import](#file-structure-after-import)
    - [7.3 Error Handling \& Rollback](#73-error-handling--rollback)
    - [7.4 tileset\_anims.c Integration](#74-tileset_animsc-integration)
      - [Changes Made by Import](#changes-made-by-import)
      - [Component: TilesetAnimsModifier](#component-tilesetanimsmodifier)
      - [Restore Behavior](#restore-behavior)
  - [7.5 C Header File Modification](#75-c-header-file-modification)
    - [Components Needed](#components-needed)
      - [HeaderStructFieldWriter](#headerstructfieldwriter)
      - [IncbinDeclarationWriter](#incbindeclarationwriter)
    - [Write Order for Atomicity](#write-order-for-atomicity)
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
  - [Appendix C: Implementation Roadmap](#appendix-c-implementation-roadmap)
    - [~~Phase 1: Configuration Foundation~~ **COMPLETE**](#phase-1-configuration-foundation-complete)
    - [~~Phase 2: Frame Name Preservation~~ **COMPLETE**](#phase-2-frame-name-preservation-complete)
    - [~~Phase 3: Utility Directory \& Metadata~~ **COMPLETE**](#phase-3-utility-directory--metadata-complete)
    - [~~Phase 4: VanillaAnimationImporter~~ **COMPLETE**](#phase-4-vanillaanimationimporter-complete)
    - [~~Phase 5: C Header File Writers~~ **COMPLETE**](#phase-5-c-header-file-writers-complete)
    - [~~Phase 6: Import Use Case~~ **COMPLETE**](#phase-6-import-use-case-complete)
    - [Phase 7: Fix TilesetRepo project implementations to use deterministic paths](#phase-7-fix-tilesetrepo-project-implementations-to-use-deterministic-paths)
    - [Phase 8: Restore Use Case](#phase-8-restore-use-case)
    - [Phase 9: Polish \& Edge Cases](#phase-9-polish--edge-cases)
    - [Implementation Notes](#implementation-notes)

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

This file is the **source of truth** for whether a tileset is Porytiles-managed. Its presence indicates the tileset is managed by Porytiles.

The `imported` field distinguishes between two cases:

**Imported tileset** (`imported: true`): Tileset was imported from vanilla pokeemerald. Original field values are stored for restoration.

```json
{
    "version": 1,
    "imported": true,
    ".tiles": "gTilesetTiles_General",
    ".palettes": "gTilesetPalettes_General",
    ".metatiles": "gMetatiles_General",
    ".metatileAttributes": "gMetatileAttributes_General",
    ".callback": "InitTilesetAnim_General"
}
```

**Created tileset** (`imported: false`): Tileset was created from scratch by Porytiles. No original values to restore.

```json
{
    "version": 1,
    "imported": false
}
```

**Purpose:**
- Stores the original field values from `src/data/tilesets/headers.h` before import modified them (when `imported: true`)
- Enables `restore-tileset` command to revert changes (only for imported tilesets)
- Includes `version` field for future format migrations
- The `imported` field indicates whether restoration is possible

**Benefits:**
- No brittle parsing of `.callback` field to detect managed status
- Simple file existence check: `original_artifacts.json` exists = managed tileset
- Easy restore: just read the original values and write them back (when `imported: true`)
- Clear error handling: `imported: false` means `restore-tileset` will error with a helpful message

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
  frame_names: ["center", "left", "right"]  # Unique frame file names (for round-trip preservation)

# Animation with numbered frames (frame_names optional when using numbers)
water:
  frame_offset: 1
  frames: ["0", "1", "2", "3"]

# Complete example with all timing parameters
flower_red:
  frames: ["center", "left", "center", "right"]
  frame_names: ["center", "left", "right"]
  frame_factor: 16      # Modulus divisor for timer % frame_factor (default: 16)
  frame_offset: 0       # Remainder for timer check (default: 0)
  counter_max: 256      # Timer wrap-around value (default: 256)

# INVALID: MyAnim fails because to_snake_case("MyAnim") != "MyAnim"
# MyAnim:
#   frames: ["0", "1"]
```

Top-level keys are animation names. The `frames` array lists all non-key frames in playback order.

#### Frame Name Preservation

Named frames (e.g., `center.png`, `left.png`) are a Porytiles authoring convenience. During compile, they become numeric (`0.png`, `1.png`). To preserve named frames across compile→decompile round-trips:

**The `frame_names` field** stores unique frame file names in index order:
- `frame_names: ["center", "left", "right"]` means index 0 = "center", index 1 = "left", etc.
- During compile: named frames → numeric frames (index based on position in `frame_names`)
- During decompile: numeric frames → named frames (lookup by index in `frame_names`)

**Compile behavior:**
```
porytiles/anim/flower/center.png → anim/flower/0.png
porytiles/anim/flower/left.png   → anim/flower/1.png
porytiles/anim/flower/right.png  → anim/flower/2.png
```

**Decompile behavior (with frame_names present):**
```
anim/flower/0.png → porytiles/anim/flower/center.png
anim/flower/1.png → porytiles/anim/flower/left.png
anim/flower/2.png → porytiles/anim/flower/right.png
```

**Decompile behavior (without frame_names):** Numeric names preserved as-is.

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

## 6. ProjectVanillaAnimImporter Service

### Purpose

Isolate all "discovery chaos" for first-time vanilla imports to a dedicated service. This keeps TilesetRepo clean and focused on deterministic operations.

### Interface

```c++
// Located in infra layer (does I/O: parses C files, reads PNGs)
class ProjectVanillaAnimImporter {
  public:
    /**
     * @brief Constructs a ProjectVanillaAnimImporter.
     *
     * @param project_root The path to the pokeemerald project root directory
     * @param format Formatter for error message styling (non-owning, must outlive importer)
     * @param diag UserDiagnostics for warnings and info messages (non-owning, must outlive importer)
     */
    ProjectVanillaAnimImporter(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    /**
     * @brief Import animations from a vanilla tileset's tileset_anims.c
     *
     * @details
     * Parses tileset_anims.c to discover animation names, frame paths, and parameters.
     * Produces Animation<IndexPixel> objects that keep tiles in their original indexed format.
     *
     * This importer does NOT extract key frames from tiles.png - that responsibility belongs to
     * AnimationDecompiler, which needs to understand VRAM layout and palette assignment.
     *
     * @param tileset_name The name of the tileset to import animations from
     * @return Map of animation names to Animation<IndexPixel> objects
     * @pre Tileset must exist and have animations defined in tileset_anims.c
     * @pre Animation arrays must follow gTilesetAnims_{TilesetName}_{AnimName} naming convention
     * @post Each returned Animation has has_key_frame() == false
     * @post Each returned Animation has frames populated with IndexPixel tile data
     */
    [[nodiscard]] ChainableResult<std::map<std::string, Animation<IndexPixel>>>
    import_animations(const std::string &tileset_name) const;
};
```

### Implementation

The implementation leverages existing parser services rather than duplicating parsing logic:

```c++
ChainableResult<std::map<std::string, Animation<IndexPixel>>>
ProjectVanillaAnimImporter::import_animations(const std::string &tileset_name) const {
    std::map<std::string, Animation<IndexPixel>> result;

    // Step 1: Get tileset metadata (callback function name) from headers.h
    ProjectTilesetMetadataProvider metadata_provider{project_root_, format_, diag_};
    auto metadata = metadata_provider.metadata_for(tileset_name);
    if (!metadata.has_animations()) {
        return result;  // No animations - return empty map
    }
    const std::string callback_func = metadata.callback_func().value();
    const std::string pascal_tileset = extract_tileset_shorthand(tileset_name);

    // Step 2: Parse AnimationParams from tileset_anims.c callback chain
    AnimCodeParser anim_parser{format_, diag_};
    auto anim_params_map = anim_parser.parse_from_callback(
        tileset_anims_path, callback_func, pascal_tileset, /*porytiles_managed=*/false);

    // Step 3: Parse INCBIN declarations to find PNG frame file paths
    CParserFacade c_parser{tileset_anims_path, format_};
    const std::string incbin_prefix = "gTilesetAnims_" + pascal_tileset + "_";
    auto incbin_decls = c_parser.parse_incbin_arrays(incbin_prefix);

    // Build map: frame variable name -> .png file path
    std::map<std::string, std::filesystem::path> frame_paths;
    for (const auto &decl : incbin_decls) {
        if (decl.variable_name().find("_Frame") != std::string::npos) {
            frame_paths[decl.variable_name()] = project_root_ / fourBpp_to_png_path(decl.paths().front());
        }
    }

    // Step 4: Load frame PNGs and extract IndexPixel tiles
    PngIndexedImageLoader png_loader;
    for (const auto &[anim_name, params] : anim_params_map) {
        Animation<IndexPixel> anim{anim_name};
        anim.params(params);

        const std::string pascal_anim_name = to_pascal_case(anim_name);
        for (const auto &frame_name : params.frame_names()) {
            const std::string frame_var =
                "gTilesetAnims_" + pascal_tileset + "_" + pascal_anim_name + "_Frame" + frame_name;

            const auto &frame_png_path = frame_paths.at(frame_var);
            auto frame_png = png_loader.load_from_file(frame_png_path);

            // Step 5: Extract tiles using domain algorithm
            std::vector<PixelTile<IndexPixel>> tiles = extract_tiles_from_image(*frame_png);

            AnimationFrame<IndexPixel> frame{frame_name, std::move(tiles)};
            anim.put_frame(frame_name, std::move(frame));
        }

        result[anim_name] = std::move(anim);
    }

    return result;
}
```

### Why IndexPixel Instead of Rgba32?

The implementation uses `Animation<IndexPixel>` instead of `Animation<Rgba32>` for several reasons:

1. **Preserves original format**: Vanilla animation frames are indexed-color PNGs. Converting to RGBA would lose palette information.
2. **Matches Porymap component**: The animations populate the Porymap component, which uses IndexPixel for tiles.
3. **Key frame responsibility**: Extracting key frames requires RGBA (for palette matching). This is AnimationDecompiler's job, not the vanilla importer.
4. **Simpler workflow**: No unnecessary color conversion overhead.

### Key Frames Are NOT Extracted

The `ProjectVanillaAnimImporter` deliberately does NOT extract key frames from `tiles.png`. Key frame extraction requires:
- Understanding VRAM tile layout
- Palette assignment knowledge
- Pixel-level comparison to identify which tile in tiles.png corresponds to the animation

This is `AnimationDecompiler`'s responsibility. The importer only handles the "easy" part: reading animation parameters and frame PNGs from vanilla assets.

### Usage in ImportUseCase

```c++
// During vanilla import workflow
ProjectVanillaAnimImporter vanilla_importer{project_root, format, diag};
auto imported_anims = vanilla_importer.import_animations(tileset_name);

// The returned animations have IndexPixel frames but no key frames
// Key frames are extracted later by AnimationDecompiler during the full decompilation process
for (const auto &[name, anim] : imported_anims) {
    porymap_component.add_anim(name, anim);
}
```

**Architecture:**

```
ImportPrimaryTileset (app layer use case)
    ├── PrimaryTilesetImporter (domain)   ← orchestrates decompilation
    │   └── ProjectPrimaryTilesetImporter (infra) ← import_from_vanilla()
    │       └── ProjectVanillaAnimImporter (infra) ← reads animation frames
    ├── TilesetFactory (domain)           ← creates empty tileset
    └── TilesetRepo (infra)               ← saves to deterministic paths
```

ProjectVanillaAnimImporter is self-contained:
- Lives in `infra/` because it does I/O
- Reuses existing parser services (AnimCodeParser, CParserFacade, PngIndexedImageLoader)
- Does NOT use `ProjectTilesetArtifactKeyProvider`—it predates the "managed" state
- Returns in-memory `Animation<IndexPixel>` objects with frame data already loaded
- Does NOT extract key frames (that's AnimationDecompiler's job)

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

### 7.4 tileset_anims.c Integration

Porytiles automatically wires generated animation code into `src/tileset_anims.c` during import.

#### Changes Made by Import

1. **Add #include directive** at the top of the file (after existing includes):
```c
#include "data/tilesets/primary/general/include/generated_anim_code.h"
```

2. **Modify the callback function** to delegate to generated code:
```c
void InitTilesetAnim_General(void)
{
    // [Porytiles] Original code preserved below
    // sSecondaryTilesetAnimCounter = 0;
    // sSecondaryTilesetAnimCounterMax = 256;
    // sSecondaryTilesetAnimCallback = TilesetAnim_General;

    InitTilesetAnim_PorytilesManaged_General();
}
```

The original callback body is preserved as comments for reference and potential manual restoration.

#### Component: TilesetAnimsModifier

A new infra-layer service responsible for parsing and modifying `tileset_anims.c`:

```c++
class TilesetAnimsModifier {
  public:
    /**
     * @brief Wire generated animation code into tileset_anims.c
     *
     * @param tileset_name The tileset to wire (e.g., "gTileset_General")
     * @param generated_header_path Path to generated_anim_code.h
     * @pre Callback function for tileset must exist in tileset_anims.c
     * @post #include directive added, callback modified to delegate
     */
    ChainableResult<void> wire_generated_code(
        const std::string &tileset_name,
        const std::filesystem::path &generated_header_path);

    /**
     * @brief Restore original callback from comments
     *
     * @param tileset_name The tileset to restore
     * @pre Callback must have Porytiles comment markers
     * @post Original code restored, #include removed
     */
    ChainableResult<void> restore_original_callback(const std::string &tileset_name);
};
```

#### Restore Behavior

During restore, `TilesetAnimsModifier::restore_original_callback`:
1. Finds the callback function for the tileset
2. Extracts original code from preserved comments
3. Replaces the delegation call with original code
4. Removes the `#include` directive for the generated header

---

## 7.5 C Header File Modification

Import modifies three C header files to point to Porytiles-managed assets. Each requires careful parsing and modification to preserve existing content.

### Components Needed

#### HeaderStructFieldWriter

Modifies `src/data/tilesets/headers.h` to update Tileset struct field values:

```c++
class HeaderStructFieldWriter {
  public:
    /**
     * @brief Update field values in a Tileset struct declaration
     *
     * @param tileset_name Which tileset struct to modify
     * @param field_updates Map of field name → new value
     * @pre Tileset struct must exist in headers.h
     * @post Field values updated, formatting preserved
     */
    ChainableResult<void> update_struct_fields(
        const std::string &tileset_name,
        const std::map<std::string, std::string> &field_updates);
};
```

#### IncbinDeclarationWriter

Modifies `src/data/tilesets/graphics.h` and `metatiles.h` to add INCBIN declarations:

```c++
class IncbinDeclarationWriter {
  public:
    /**
     * @brief Add INCBIN declarations for a Porytiles-managed tileset
     *
     * @param tileset_name Which tileset these declarations are for
     * @param declarations List of {variable_name, incbin_path, type} tuples
     * @post New declarations added to appropriate header file
     */
    ChainableResult<void> add_declarations(
        const std::string &tileset_name,
        const std::vector<IncbinDeclaration> &declarations);

    /**
     * @brief Remove INCBIN declarations for a tileset (during restore)
     */
    ChainableResult<void> remove_declarations(const std::string &tileset_name);
};
```

### Write Order for Atomicity

Import writes files in this order to ensure crash recovery is possible:

1. Write all new managed asset files (`porytiles_src/`, `porytiles_bin/`)
2. Write `original_artifacts.json` (marks intent to modify headers)
3. Modify `graphics.h` (add new INCBINs)
4. Modify `metatiles.h` (add new INCBINs)
5. Modify `tileset_anims.c` (wire generated code)
6. Modify `headers.h` (swap variable references)

**Recovery:** If a crash occurs after step 2, `original_artifacts.json` exists but headers are inconsistent. The restore command can detect this and complete the restoration.

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
| `imported: false` in `original_artifacts.json` | Error: "Cannot restore tileset created by Porytiles (no original to restore)" |
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
- **Tileset path config values** (`tileset.paths.primary.src/bin`, etc.)
- **animation.overwrite_callback** config value
- **frame_names field support** in AnimYamlParser (reads `frames` as definitions, `frame_order` as playback sequence)
- **porytiles/ utility directory** support
- **original_artifacts.json** model and reader/writer (OriginalArtifacts, ProjectPorytilesTilesetManager)
- **ProjectVanillaAnimImporter** service (returns `Animation<IndexPixel>`, does NOT extract key frames)
- **extract_tiles_from_image()** domain algorithm in `tile_extractors.hpp`
- **PrimaryTilesetImporter** domain service (partial - orchestrates decompilation workflow)
- **ProjectPrimaryTilesetImporter** infra service (stub - needs full implementation)
- **ImportPrimaryTileset** use case (exists, workflow incomplete)

### 10.2 Needed

**C File Modification:**
- [ ] HeaderStructFieldWriter (modify headers.h struct fields)
- [ ] IncbinDeclarationWriter (modify graphics.h, metatiles.h)
- [ ] TilesetAnimsModifier (wire generated code into tileset_anims.c)

**Use Cases:**
- [ ] Complete ImportPrimaryTileset workflow (PrimaryTilesetImporter.import() still returns TODO)
- [ ] RestoreTilesetUseCase
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
        std::unique_ptr<Tileset>,
        "tileset load failed");

    for (const auto &porytiles_anim_name : porytiles_anims) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            key_frame_key,
            key_provider_->key_for_porytiles_anim_key_frame(tileset->name(), porytiles_anim_name),
            std::unique_ptr<Tileset>,
            "tileset load failed");

        if (!key_provider_->artifact_exists(key_frame_key)) {
            return FormattableError{missing_required_artifact_msg, FormatParam{key_frame_key.key(), Style::bold}};
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porytiles_anim_frames(tileset->name(), porytiles_anim_name),
            std::unique_ptr<Tileset>,
            "tileset load failed");

        std::vector<ArtifactKey> frames_keys{};
        for (const auto &frame : frames) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porytiles_anim_frame(tileset->name(), porytiles_anim_name, frame),
                std::unique_ptr<Tileset>,
                "tileset load failed");

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
            void,
            "tileset save failed");

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

---

## Appendix C: Implementation Roadmap

This section outlines a high-level step-by-step plan to implement the refactoring. Each phase builds on the previous, allowing incremental testing and validation.

### ~~Phase 1: Configuration Foundation~~ **COMPLETE**

**Goal:** Add config values needed by the import workflow.

1. **Add tileset path config values** to `config_schema.yaml`:
   - `tileset.paths.primary.src` (default: `data/tilesets/primary`)
   - `tileset.paths.primary.bin` (default: `data/tilesets/primary`)
   - `tileset.paths.secondary.src` (default: `data/tilesets/secondary`)
   - `tileset.paths.secondary.bin` (default: `data/tilesets/secondary`)

2. **Add animation config value**:
   - `tileset.animations.overwrite_callback` (default: `true`)

3. **Regenerate config files** using `scripts/generate_config.py`

4. **Update key providers** to use new config values for path computation

**Verification:** Config values accessible and key generation uses them correctly.

---

### ~~Phase 2: Frame Name Preservation~~ **COMPLETE**

**Goal:** Enable lossless named frame round-trips.

1. **Add `frame_names` field to AnimationParams** model

2. **Update AnimYamlParser** to read/write `frame_names` (now reads `frames` as unique definitions, `frame_order` as playback sequence)

3. **Update compile workflow** to map named frames → numeric indices

4. **Update decompile workflow** to map numeric indices → named frames

**Verification:** Compile a tileset with named frames, decompile it, verify names restored.

---

### ~~Phase 3: Utility Directory & Metadata~~ **COMPLETE**

**Goal:** Establish porytiles/ directory structure and managed-status tracking.

1. **Create OriginalArtifacts model** (`domain/models/`)
   - Fields: version, tiles, palettes, metatiles, metatileAttributes, callback

2. **Create OriginalArtifactsReader/Writer** (`infra/services/`)
   - JSON serialization using nlohmann::json

3. **Update ProjectArtifactChecksumProvider** to write checksums to `porytiles/tilesets/{name}/`

4. **Add utility directory creation** to key provider or a dedicated service

**Verification:** Create porytiles/ structure, write/read original_artifacts.json.

---

### ~~Phase 4: VanillaAnimationImporter~~ **COMPLETE**

**Goal:** Import animations from vanilla tileset_anims.c.

1. **Created ProjectVanillaAnimImporter** class (`infra/services/`)
   - Reuses existing AnimCodeParser for callback chain parsing
   - Reuses CParserFacade for INCBIN declaration parsing
   - Reuses PngIndexedImageLoader for loading frame PNGs
   - Reads frames from INCBIN paths (scattered locations)
   - Constructs `Animation<IndexPixel>` objects (keeps indexed format, no RGBA conversion)

2. **Created `extract_tiles_from_image()` domain algorithm** (`domain/algorithms/tile_extractors.hpp`)
   - Generic template function for extracting 8x8 tiles from images
   - Two overloads: extract all tiles, or extract subset at offset

3. **Key frame extraction is NOT done by this importer**
   - Key frame extraction requires VRAM layout knowledge and palette matching
   - This responsibility stays with AnimationDecompiler
   - Returned animations have `has_key_frame() == false`

4. **anim.yaml generation** happens during the full compile workflow, not during import

**Key Design Decision:** Using `Animation<IndexPixel>` instead of `Animation<Rgba32>` because:
- Preserves original indexed color format
- Matches Porymap component tile format
- Avoids unnecessary RGBA conversion overhead
- Key frame extraction (which needs RGBA) is AnimationDecompiler's responsibility

**Verification:** Integration tests in `project_vanilla_anim_importer_test.cpp` verify import of all 5 animations from gTileset_General (flower, land_water_edge, sand_water_edge, water, waterfall).

---

### ~~Phase 5: C Header File Writers~~ **COMPLETE**

**Goal:** Enable modification of pokeemerald C header files.

1. **Create HeaderStructFieldWriter** (`infra/services/`)
   - Parse headers.h to locate Tileset struct
   - Update field values preserving formatting (including anim callback)
   - Handle edge cases (multiline values, comments)
   - Support restore to original values from original_artifacts.json

2. **Create IncbinDeclarationWriter** (`infra/services/`)
   - Add new INCBIN declarations to graphics.h
   - Add new INCBIN declarations to metatiles.h
   - Support removal during restore

3. **Create TilesetAnimsModifier** (`infra/services/`)
   - Insert #include directive that points to Porytiles-managed `include/generated_anim_code.h` for relevant tileset
   - Support restore (remove relevant include directive)

**Verification:** Modify test headers, verify compilation succeeds.

---

### ~~Phase 6: Import Use Case~~ **COMPLETE**

**Goal:** Orchestrate full import workflow.

**Completed:**

1. **Created ImportPrimaryTileset** use case (`app/use_cases/`)
   - Pre-import validation (tileset exists via TilesetMetadataProvider)
   - Check if already Porytiles-managed (via PorytilesTilesetManager)
   - Calls PrimaryTilesetImporter.import() to orchestrate decompilation

2. **Created PrimaryTilesetImporter** domain service (`domain/services/`)
   - Abstract base class with `import_from_vanilla()` pure virtual method
   - `import()` method orchestrates decompilation:
     - Calls `import_from_vanilla()` to get PorymapTilesetComponent
     - Triple-layerizes via LayerModeConverter
     - Decompiles metatiles via MetatileDecompiler
   - **Incomplete:** Returns `TODO: impl` after decompilation steps

3. **Created ProjectPrimaryTilesetImporter** infra service (`infra/services/`)
   - Implements `import_from_vanilla()` abstract method
   - **Stub:** Currently returns `FormattableError{"TODO: impl"}`

4. **Added CLI command** `porytiles import-tileset <tileset_name>` via ImportTilesetCommand

**Verification:** Import gTileset_General, verify all files created, project compiles.

---

### Phase 7: Fix TilesetRepo project implementations to use deterministic paths

**Goal:** Refactor TilesetRepo and its dependencies so that load/save operations for Porytiles-managed tilesets use exclusively deterministic paths, eliminating INCBIN parsing from the normal workflow.

1. ~~Refactor ProjectTilesetArtifactKeyProvider for Porymap deterministic paths~~ **COMPLETE**
   - Update `key_for_tiles_png()` to return `<tileset_root>/porytiles_bin/tiles.png` for managed tilesets
   - Update `key_for_metatiles_bin()` to return `<tileset_root>/porytiles_bin/metatiles.bin`
   - Update `key_for_metatile_attributes_bin()` to return `<tileset_root>/porytiles_bin/metatile_attributes.bin`
   - Update `key_for_porymap_pal_n()` to return `<tileset_root>/porytiles_bin/palettes/<n>.gbapal`
   - Update `key_for_porymap_anim_frame()` to return `<tileset_root>/anim/<anim_name>/<frame_name>.png`

2. ~~Isolate INCBIN parsing to import-only code paths~~ **COMPLETE**
   - Remove INCBIN fallback logic from normal load/save code paths in the reader/writer

3. ~~Update discovery methods for managed tilesets~~ **COMPLETE**
   - `discover_porymap_anims()`: Scan `<tileset_root>/porytiles_bin/anim/` directory instead of parsing C files
   - `discover_porymap_anim_frames()`: Glob `<tileset_root>/porytiles_bin/anim/<anim_name>/*.png`
   - `discover_porytiles_anims()`: Scan `<tileset_root>/porytiles_src/anim/` directory instead of parsing anim.yaml
   - `discover_porytiles_anim_frames()`: Glob `<tileset_root>/porytiles_src/anim/<anim_name>/*.png`
   - directory names must be snake_case, i.e. `snake_case(dir_name) == dir_name`, if this doesn't hold, throw an error

4. **Clean up hardcoded values and path hacks**
   - Extract `"key"` frame name to a constant or domain-layer definition
   - Remove path derivation hack in `write_porymap_anim_params()` (currently uses `parent_path().parent_path()`)
   - Use config values (`tileset.paths.primary.bin`, etc.) consistently for path computation

**Key Design Decision:** After this phase, the `TilesetRepo::load`/`save` workflow for Porytiles-managed tilesets will be completely independent of C source file parsing. INCBIN parsing becomes an import-time concern only, matching the document's Three-Workflow Model where Import handles "discovery chaos" and subsequent operations are deterministic. The ProjectKeyProvider class will now return managed keys only, so any TilesetRepo using it MUST operate on managed tilesets only. 

**Verification:**
- Unit tests verify key provider returns deterministic paths for managed tilesets
- Integration tests perform load/save round-trip for managed tileset without triggering INCBIN parsing
- Verify INCBIN parsing code is only reachable from import workflow
- Compile test pokeemerald project after load/save to confirm generated paths are correct

---

### Phase 8: Restore Use Case

**Goal:** Revert to vanilla state.

1. **Create RestoreTilesetUseCase** (`app/use_cases/`)
   - Read original_artifacts.json
   - Call header writers to restore original values
   - Call TilesetAnimsModifier to restore callback
   - Delete original_artifacts.json
   - Optionally delete porytiles_src/, porytiles_bin/ (`--full` flag)

2. **Add CLI command** `porytiles restore-tileset <tileset_name> [--full]`

3. **Integration tests** for restore workflow

**Verification:** Import, then restore, verify vanilla state returns.

---

### Phase 9: Polish & Edge Cases

**Goal:** Handle edge cases and improve UX.

1. **Improve error messages** for common failures
   - Wrong animation naming convention
   - Missing frames
   - Already managed tileset

2. **Add `--force` flag** to re-import already-managed tilesets

3. **Handle tilesets without animations** (import non-animated tilesets)

4. **Documentation** and usage examples

**Verification:** Test edge cases, verify clear error messages.

---

### Implementation Notes

**Testing Strategy:**
- Unit tests for each new service (parsers, writers)
- Integration tests using `pokeemerald_porytilestesttilesets` test project
- Round-trip tests: import → compile → decompile → verify equality

**Key Files to Modify:**
- `porytiles/config_templates/config_schema.yaml` — new config values
- `porytiles/include/porytiles/domain/models/animation_params.hpp` — frame_names field
- `porytiles/lib/infra/services/anim_yaml_parser.cpp` — frame_names parsing
- New files in `infra/services/` for header writers
- New files in `app/use_cases/` for import/restore

**Dependencies Between Phases:**
```
Phase 1 (Config) ─────────────────────────────────────────────┐
                                                              │
Phase 2 (Frame Names) ────────────────────────────────────────┤
                                                              │
Phase 3 (Utility Dir) ────────────────────────────────────────┤
                                                              ▼
Phase 4 (VanillaImporter) ──► Phase 6 (Import UseCase) ──► Phase 8 (Polish)
                                        │
Phase 5 (Header Writers) ───────────────┤
                                        │
                                        ▼
                              Phase 7 (Restore UseCase)
