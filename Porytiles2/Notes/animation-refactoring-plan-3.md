- [Animation Loading Refactoring Plan (Revision 3)](#animation-loading-refactoring-plan-revision-3)
  - [Goal](#goal)
  - [Revision 2 Gaps Analysis](#revision-2-gaps-analysis)
    - [Key Computation Fails To Account For Net-New Animations](#key-computation-fails-to-account-for-net-new-animations)
  - [Solution: Configuration-Driven Key Generation](#solution-configuration-driven-key-generation)
    - [The Core Problem](#the-core-problem)
    - [Key Insight: Discovery vs Key Generation Are Orthogonal](#key-insight-discovery-vs-key-generation-are-orthogonal)
    - [Animation Management Modes](#animation-management-modes)
  - [Configuration](#configuration)
    - [Configuration Location](#configuration-location)
    - [Configuration Schema](#configuration-schema)
  - [Animation Components](#animation-components)
    - [Porytiles Animation Component](#porytiles-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames)
        - [Project Key Provider](#project-key-provider)
      - [Artifact: Animation Parameters](#artifact-animation-parameters)
        - [Project Key Provider](#project-key-provider-1)
      - [Project Reader](#project-reader)
      - [Project Writer](#project-writer)
    - [Porymap Animation Component](#porymap-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames-1)
        - [Project Key Provider](#project-key-provider-2)
      - [Artifact: Animation Parameters](#artifact-animation-parameters-1)
        - [Project Key Provider](#project-key-provider-3)
      - [Project Reader](#project-reader-1)
      - [Project Writer](#project-writer-1)
  - [VanillaAnimationImporter Service](#vanillaanimationimporter-service)
    - [Purpose](#purpose)
    - [Interface](#interface)
    - [Implementation](#implementation)
  - [Summary Table](#summary-table)
  - [Precondition / Invariant Summaries](#precondition--invariant-summaries)
    - [Porytiles/Porymap Animation Name Conversions](#porytilesporymap-animation-name-conversions)
    - [Porytiles Animations Must Follow `porytiles/anim/{anim_name}` Structure](#porytiles-animations-must-follow-porytilesanimanim_name-structure)
    - [Porymap Animation Frame Path Convention](#porymap-animation-frame-path-convention)

# Animation Loading Refactoring Plan (Revision 3)

## Goal
Refactor TilesetRepo and affiliated helper services so that Animations are a first-class concept with coherent loading.
This revision addresses the chicken-and-egg problem identified in the implementation of Revision 2.

## Revision 2 Gaps Analysis
Revision 2 of this plan plus implementation got us closer, but there are still some critical gaps.

### Key Computation Fails To Account For Net-New Animations
If the user adds a new Porymap-component anim (or updates frame count) and imports
OR
the user adds a new Porytiles-component anim (or updates frame count) and compiles,
the TilesetRepo::save will fail.

This is because `key_for_porymap_anim_frame` parses `generated_anim_code.h` to find INCBIN paths.
For new animations, those INCBINs don't exist yet - they only get written AFTER save completes.

**Root Cause**: The key provider conflates two fundamentally different operations:
1. **Key Generation** (deterministic): "Where SHOULD this artifact live?" - Pure computation
2. **Key Discovery** (I/O-based): "Where DOES this artifact live?" - Parsing existing files

For Porytiles artifacts, these are the same (deterministic conventions).
For Porymap artifacts, they differ - because vanilla projects could have frames scattered anywhere.

## Solution: Configuration-Driven Key Generation

### The Core Problem

**`key_for_porytiles_anim_frame`** is deterministic:
```c++
return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_name / (frame_name + ".png")};
```
Works for both read and write.

**`key_for_porymap_anim_frame`** requires parsing existing files:
```c++
PT_TRY_ASSIGN_CHAIN_ERR(frame_paths, porymap_animation_frame_paths_for(tileset_name), ...);
// Then searches the parsed frame_paths map from INCBIN declarations
```
Fails for new animations!

### Key Insight: Discovery vs Key Generation Are Orthogonal

- **Discovery** (`discover_*` methods): "WHAT animations/frames exist?" - Always scans component's source of truth
- **Key Generation** (`key_for_*` methods): "WHERE should I read/write this artifact?" - Mode-dependent for Porymap

The chicken-and-egg problem is in **key generation**, not discovery.
New animations get discovered from the in-memory Tileset domain object during save,
but key generation for new Porymap frames fails because it tries to parse files that don't exist yet.

**The fix**: For Porytiles-managed tilesets, `key_for_porymap_anim_frame` uses deterministic paths
(Porytiles controls where things go). Discovery remains independent.

### Animation Management Modes

Two modes with an optional flag for fine-grained control:

- `porytiles_managed` (default): Porytiles owns animation code generation, uses deterministic paths
  - `overwrite_callback: true` (default): Porytiles generates code AND updates `.callback` in headers.h
  - `overwrite_callback: false`: Porytiles generates code but leaves `.callback` alone (user wires their own callback)
- `user_managed`: Porytiles reads existing animations but doesn't write Porymap component files. Uses discovery-based key generation for Porymap artifacts. User fully controls animation file organization and callback wiring.

## Configuration

### Configuration Location

Porytiles uses layered configuration:
- **Project-wide defaults**: `porytiles.yaml` in project root
- **Tileset-specific overrides**: `porytiles/porytiles.yaml` in tileset folder
- **anim.yaml**: Only for pokeemerald engine parameters (frame_factor, frames, etc.), NOT for Porytiles config

### Configuration Schema

Add to config schema:

```yaml
# In project root porytiles.yaml (defaults)
animation:
  management_mode: porytiles_managed  # or "user_managed"
  overwrite_callback: true            # only applies to porytiles_managed mode

# In tileset porytiles/porytiles.yaml (override if needed)
animation:
  management_mode: user_managed  # This tileset keeps manual control
```

#### Mode + Flag Behavior Summary

| Mode | overwrite_callback | Writes Frames | Generates Code | Updates Callback | Key Generation |
|------|-------------------|---------------|----------------|------------------|----------------|
| `porytiles_managed` | `true` (default) | Yes | Yes | Yes | Deterministic |
| `porytiles_managed` | `false` | Yes | Yes | **No** | Deterministic |
| `user_managed` | (ignored) | No | No | No | Discovery-based |

## Animation Components

### Porytiles Animation Component
The entire `Animation` object for the Porytiles component can be constructed from the supplied frames and the `anim.yaml` file.
For Porytiles component animations, key frames are required.

**Source of Truth**: `anim.yaml`

#### Artifact: Animation Frames
`porytiles/anim/{anim_name}/{frame_name}.png`

Error if there is no frame called `key.png`.
Frames can have any name the user wants.

##### Project Key Provider
Invariant: For Project impl, Porytiles anim frames must be stored in `porytiles/anim/{anim_name}/{frame_name}`.
`{anim_name}` and `{frame_name}` will be inferred from the user-supplied `anim.yaml` file.

**Key generation is always deterministic** (no change from Revision 2):

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) {
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, anim.yaml location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    // read anim.yaml from this location and return all top level keys
    // if any top level key doesn't satisfy `to_snake_case(key) == key`, return an "invalid key" error pointing to that key
    return top_level_yaml_keys(asset_path).to_set();
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) {
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, anim.yaml location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    // extract values from anim.yaml "frames" array under the relevant key (anim_name)
    // NOTE: manually include "key" frame if not already present
    auto frames = extract_yaml_array(anim_name + ".frames").to_set();
    frames.insert("key");
    return frames;
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    // DETERMINISTIC: read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/{anim_name}/{frame_name}.png
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_name / (frame_name + ".png")};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_key_frame(const std::string &tileset_name, const std::string &anim_name) {
    // DETERMINISTIC: just calls key_for_porytiles_anim_frame with frame_name = "key"
    return key_for_porytiles_anim_frame(tileset_name, anim_name, "key");
}
```

#### Artifact: Animation Parameters
For Project impl, anim params are in: `porytiles/anim/anim.yaml`

For Porytiles anims, this file is the source of truth.
Top level keys are anim names, `frames:` arrays define all frames.
`frames:` array probably won't contain the "key" frame so we need to make sure to manually include it in `discover_porytiles_anim_frames`.

```yaml
# Assuming flower has frames called "left.png", "center.png", "right.png"
flower:
  frames: ["center", "left", "center", "right"]

# Assuming water has the traditional numbered frames
water:
  frame_offset: 1
  frames: ["0", "1", "2", "3"]

# This anim name will cause the key provider to error out
# to_snake_case(MyAnim) != MyAnim so it's an invalid name for anim.yaml
MyAnim:
  frame_offset: 2
  frames: ["0", "1", "2", "3"]
```

##### Project Key Provider
```c++
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_params(const std::string &tileset_name) {
    // DETERMINISTIC: read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_yaml};
}
```

#### Project Reader
The Project reader gets the whole anim in one shot.
TilesetRepo works the same as before, it discovers anims and anim frames via the key provider,
then passes discovered anims/frames into the read_porytiles_anim function.

```c++
// TilesetRepo::load snippet

// Only read animations if a params artifact file existed, the params artifact is the source of truth
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
        // Assume we already have porytiles_params_key from earlier code
        const auto anim_result = reader_->read_porytiles_anim(*tileset, porytiles_anim_name, porytiles_params_key, frames_keys);
    }
}
```

```c++
ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const ArtifactKey &key_frame_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params and frames in one shot
    // We have the artifact keys that were constructed from anim.yaml values
    // Panic if the params_key doesn't exist
    // Panic if the key_frame_key doesn't exist
    // Panic if expected frame from frames_keys doesn't exist (this vector will include key.png)
}
```

#### Project Writer
Porytiles key generation is deterministic, so writing works the same way as reading.
No changes needed here.

### Porymap Animation Component
The entire `Animation` object for the Porymap component can be constructed from `include/generated_anim_code.h` file.
If this file doesn't exist yet (first-time import case), then we have a special code-path to figure out everything from `tileset_anims.c`.

**Source of Truth for Discovery**: `generated_anim_code.h` or `tileset_anims.c`
**Source of Truth for Key Generation**: Depends on animation management mode

**Invariant:** `generated_anim_code.h` will always use `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}`
as the name format for a tileset animation frame array.
Users who want to first-time import a tileset into Porytiles will need to update `tileset_anims.c` to follow this invariant as well.

#### Artifact: Animation Frames
For the Porymap component, by default animation frames are defined via frame variables in `include/generated_anim_code.h`.
However, if this file doesn't exist (first-time import case), then we'll need to search `src/tileset_anims.c`.

##### Project Key Provider

**KEY CHANGE FROM REVISION 2**: `key_for_porymap_anim_frame` is now mode-aware.
For `porytiles_managed` mode, key generation is **deterministic**.
For `user_managed` mode, key generation uses **discovery** (same as Revision 2).

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) {
    // NOTE: Discovery is INDEPENDENT of management mode
    // Always scan the Porymap source of truth (generated_anim_code.h or tileset_anims.c)
    // This supports bidirectional workflows: Porymap -> Porytiles decompilation

    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porymap assets, try hardcoded location data/tilesets/{primary,secondary}/{tileset_name}/include/generated_anim_code.h
    // if this file exists, pull anim names from all gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} variables
    // in this case, the anim names will be to_snake_case({AnimName}), where {AnimName} is the anim part of the gTilesetAnims variable
    if (exists(asset_path)) {
        return read_g_tileset_anims_vars(asset_path, tileset_name).to_set().map(name -> to_snake_case(name));
    }
    // otherwise, we need to parse vanilla tileset_anims.c to find anim names
    // invariant: in tileset_anims.c users are REQUIRED to name their frame arrays following gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} format
    // in this case, the anim names will be to_snake_case({AnimName}), where {AnimName} is the anim part of the gTilesetAnims variable
    return read_vanilla_g_tileset_anims_vars(tileset_name).to_set().map(name -> to_snake_case(name));
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) {
    // NOTE: Discovery is INDEPENDENT of management mode
    // Always scan the Porymap source of truth

    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porymap assets, try hardcoded location data/tilesets/{primary,secondary}/{tileset_name}/include/generated_anim_code.h
    if (exists(asset_path)) {
        // This function should find any gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} variables associated
        // with tileset_name and anim_name. For each unique array element, find the associated var declaration.
        // Find the path inside the INCBIN_U16 call, take the basename of the path and remove file extensions.
        // That's the frame name.
        return read_g_tileset_anims_var_frame_names(asset_path, tileset_name, anim_name).to_set();
    }
    // otherwise, we need to parse vanilla tileset_anims.c to find anim frame names
    // invariant: in tileset_anims.c users are REQUIRED to name their frame arrays following gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} format
    // this function can work similarly to read_g_tileset_anims_var_contents, except it's looking in tileset_anims.c instead
    return read_vanilla_g_tileset_anims_var_frame_names(tileset_name, anim_name).to_set();
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    // KEY CHANGE: Mode-aware key generation

    const auto tileset_path = tileset_root(tileset_name);
    const auto mode = get_animation_management_mode(tileset_name);

    switch (mode) {
        case AnimationManagementMode::porytiles_managed:
            // DETERMINISTIC: Porytiles controls output location
            // Location is: data/tilesets/{primary,secondary}/{tileset_name}/anim/{anim_name}/{frame_name}.png
            return ArtifactKey{tileset_path / anim_dir / anim_name / (frame_name + ".png")};

        case AnimationManagementMode::user_managed:
            // DISCOVERY-BASED: Parse existing INCBIN declarations
            // This is the same as Revision 2 behavior
            if (exists(generated_anim_code_path)) {
                return read_g_tileset_anims_var_frame_paths(generated_anim_code_path, tileset_name, anim_name, frame_name);
            }
            return read_vanilla_g_tileset_anims_var_frame_paths(tileset_name, anim_name, frame_name);
    }
}
```

#### Artifact: Animation Parameters
For Porytiles-managed tilesets, animation parameters are stored in `generated_anim_code.h`.
For user-managed tilesets, animation parameters may be in `tileset_anims.c`.

##### Project Key Provider
```c++
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) {
    // DETERMINISTIC: This doesn't change based on mode
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porymap assets, return hardcoded location data/tilesets/{primary,secondary}/{tileset_name}/include/generated_anim_code.h
    return ArtifactKey{tileset_path / include_dir / generated_anim_code_header};
}
```

#### Project Reader
The Project reader gets the whole anim in one shot.
TilesetRepo works the same as before, it discovers anims and anim frames via the key provider,
then passes discovered anims/frames into the read_porymap_anim function.

```c++
// TilesetRepo::load snippet

// Discovery always scans the Porymap source of truth (generated_anim_code.h or tileset_anims.c)
// This supports bidirectional workflows
PT_TRY_ASSIGN_CHAIN_ERR(
    porymap_anims,
    key_provider_->discover_porymap_anims(tileset->name()),
    "tileset load failed",
    std::unique_ptr<Tileset>);
for (const auto &porymap_anim_name : porymap_anims) {
    PT_TRY_ASSIGN_CHAIN_ERR(
        frames,
        key_provider_->discover_porymap_anim_frames(tileset->name(), porymap_anim_name),
        "tileset load failed",
        std::unique_ptr<Tileset>);

    std::vector<ArtifactKey> frames_keys{};
    for (const auto &frame : frames) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            frame_key,
            key_provider_->key_for_porymap_anim_frame(tileset->name(), porymap_anim_name, frame),
            "tileset load failed",
            std::unique_ptr<Tileset>);

        if (!key_provider_->artifact_exists(frame_key)) {
            return FormattableError{missing_required_artifact_msg, FormatParam{frame_key.key(), Style::bold}};
        }
        frames_keys.push_back(frame_key);
    }
    // Assume we already have params_key from earlier code
    const auto anim_result = reader_->read_porymap_anim(*tileset, porymap_anim_name, porymap_params_key, frames_keys);
}
```

```c++
ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params and frames in one shot
    // We have the artifact keys that were constructed from generated_anim_code.h or vanilla code
    // if params_key does not exist on disk, we have to try reading params from vanilla code
    // Return FormattableError if expected frame from frames_keys doesn't exist
}
```

#### Project Writer
For `porytiles_managed` mode, key generation is deterministic, so writing works.
For `user_managed` mode, the writer should NOT write Porymap animation files (user manages their own).

The `overwrite_callback` flag controls whether Porytiles updates the `.callback` field in headers.h.
This flag is only relevant in `porytiles_managed` mode.

```c++
// TilesetRepo::save snippet for Porymap animations

const auto mode = get_animation_management_mode(tileset.name());
if (mode == AnimationManagementMode::user_managed) {
    // Skip writing Porymap animations - user manages their own
    // We might still update anim.yaml or emit warnings
} else {
    // porytiles_managed: deterministic key generation works
    for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
        for (std::size_t i = 0; i < porymap_anim.frame_count(); i++) {
            const auto frame_name = std::to_string(i);
            // KEY CHANGE: This now succeeds because key_for_porymap_anim_frame is deterministic in managed mode
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), frame_name),
                "tileset save failed",
                void);
            if (auto result = writer_->write_porymap_anim_frame(frame_key, tileset, porymap_anim.name(), frame_name);
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }
}
```

## VanillaAnimationImporter Service

### Purpose
Isolate all "discovery chaos" for first-time vanilla imports to a dedicated service.
This keeps TilesetRepo clean and focused on deterministic operations.

### Interface
```c++
// Located in infra layer since it does I/O (parses C files)
class VanillaAnimationImporter {
  public:
    /**
     * @brief Import animations from a vanilla tileset's tileset_anims.c
     *
     * @details
     * Parses tileset_anims.c to discover animation names, frame paths, and parameters.
     * Produces Animation<Rgba32> objects that can be added to a Tileset's Porytiles component.
     * After import, the tileset becomes "Porytiles-managed" and uses deterministic paths.
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

**Usage in compile use case:**
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

## Summary Table

### Discovery vs Key Generation by Mode

| Method | Porytiles-Managed | User-Managed |
|--------|-------------------|--------------|
| `discover_porytiles_anims` | Scan `anim.yaml` | Scan `anim.yaml` |
| `discover_porymap_anims` | Scan `generated_anim_code.h` | Scan INCBIN declarations |
| `key_for_porytiles_anim_frame` | **Deterministic** | **Deterministic** |
| `key_for_porymap_anim_frame` | **Deterministic** | Discovery-based |
| Write Porymap animations | Yes | No (user manages) |

### Mode + Flag Behavior Matrix

| Mode | overwrite_callback | Writes Frames | Generates Code | Updates Callback | Key Generation |
|------|-------------------|---------------|----------------|------------------|----------------|
| `porytiles_managed` | `true` (default) | Yes | Yes | Yes | Deterministic |
| `porytiles_managed` | `false` | Yes | Yes | **No** | Deterministic |
| `user_managed` | (ignored) | No | No | No | Discovery-based |

**Key insight**: The `overwrite_callback` flag provides fine-grained control within `porytiles_managed` mode.
Users who want Porytiles to compile animation frames and generate code, but want to wire their own
callback function, can set `overwrite_callback: false`. This is useful for custom animation behaviors
that go beyond what Porytiles auto-generates.

## Precondition / Invariant Summaries
Porytiles should minimize the number of preconditions / invariants users must follow
while maximizing the effectiveness of the preconditions / invariants it chooses.

### Porytiles/Porymap Animation Name Conversions
Key Fact: if a variable name is already in valid PascalCase or snake_case, the `PascalCase <=> snake_case` transformation is an involution.
E.g. you can do `tosnakecase(topascalcase("my_snake_case")) == "my_snake_case"` and `topascalcase(tosnakecase("MyPascalCase")) == "MyPascalCase"`.

Animation names will have three "forms":
- canonical name: user selected, this name appears in anim.yaml
  - NOTE: this name **MUST** already be in snake_case, `to_snake_case(canonical) == canonical`
  - used for diagnostic messages, it's the key in the tileset component Animation maps and the Animation `name_` field
- PascalCase-ified: generated via `to_pascal_case(canonical)`
  - this name is used for the `{AnimName}` component in the `gTilesetAnims` variable in `include/generated_anim_code.h`
  - `to_snake_case(to_pascal_case(canonical)) == canonical` MUST hold

These conventions give us a way to compile/import animations back and forth in a loss-less way.
The `canonical <=> snake_case` and `canonical <=> PascalCase` transformations must be **involutions**.

### Porytiles Animations Must Follow `porytiles/anim/{anim_name}` Structure
Once users have onboarded a tileset to Porytiles, they must store anim frames for a given anim under `porytiles/anim/{anim_name}`.
(The absolute location of `porytiles` folder for a given tileset is configurable, but the structure within the `porytiles` folder is not configurable).
This makes the Porytiles animation component key provider, reader, and writer logic simple.
We just assume anim.yaml as source of truth for anim and frame names, and we can easily construct all the frame location paths.

### Porymap Animation Frame Path Convention
For **Porytiles-managed** tilesets, Porymap animation frames are stored at deterministic paths:
```
data/tilesets/{primary,secondary}/{tileset_name}/anim/{anim_name}/{frame_name}.png
```

For **user-managed** tilesets, Porymap animation frames can be stored anywhere.
The key provider discovers their locations by parsing INCBIN declarations.
Users must ensure their frame arrays follow the naming convention: `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}`

This convention allows Porytiles to find and parse animation definitions while giving users flexibility in file organization.
