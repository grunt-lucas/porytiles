- [Animation Loading Refactoring Plan (Revision 2)](#animation-loading-refactoring-plan-revision-2)
  - [Goal](#goal)
  - [Problems Being Solved](#problems-being-solved)
  - [Animation Components](#animation-components)
    - [Porytiles Animation Component](#porytiles-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames)
        - [Project Key Provider](#project-key-provider)
      - [Artifact: Animation Parameters](#artifact-animation-parameters)
        - [Project Key Provider](#project-key-provider-1)
      - [Project Reader](#project-reader)
    - [Porymap Animation Component](#porymap-animation-component)
      - [Artifact: Animation Frames](#artifact-animation-frames-1)
        - [Project Key Provider](#project-key-provider-2)
      - [Artifact: Animation Parameters](#artifact-animation-parameters-1)
        - [Project Key Provider](#project-key-provider-3)
      - [Project Reader](#project-reader-1)
  - [Precondition / Invariant Summaries](#precondition--invariant-summaries)
    - [Porytiles/Porymap Animation Name Conversions](#porytilesporymap-animation-name-conversions)
    - [Porytiles Animations Must Follow `porytiles/anim/{anim_name}` Structure](#porytiles-animations-must-follow-porytilesanimanim_name-structure)
    - [Animation Frame Pointer Array Must Follow Name Convention: `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}`](#animation-frame-pointer-array-must-follow-name-convention-gtilesetanims_tilesetname_animname_optional_suffix)

# Animation Loading Refactoring Plan (Revision 2)

## Goal
Refactor TilesetRepo and affiliated helper services so that Animations are a first-class concept with coherent loading.

## Problems Being Solved
1. **Infra leaking into domain**: `AnimationCallbackInfo` (infra concept with C file paths) is exposed in `TilesetArtifactKeyProvider` domain interface
2. **Incoherent discovery**: Frames discovered separately from parameters via different code paths
3. **Duplicated logic**: Animation name parsing exists in both `AnimCodeParser` and `ProjectTilesetMetadataProvider`
4. **Complex orchestration**: TilesetRepo::load() has ~100 lines of animation loading logic

## Animation Components

### Porytiles Animation Component
The entire `Animation` object for the Porytiles component can be constructed from the supplied frames and the `anim.yaml` file.
For Porytiles component animations, key frames are required.

#### Artifact: Animation Frames
`porytiles/anim/{anim_name}/{frame_name}.png`

Error if there is no frame called `key.png`.
Frames can have any name the user wants.

##### Project Key Provider
Invariant: For Project impl, Porytiles anim frames must be stored in `porytiles/anim/{anim_name}/{frame_name}`.
`{anim_name}` and `{frame_name}` will be inferred from the user-supplied `anim.yaml` file.

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, anim.yaml location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    // read anim.yaml from this location and return all top level keys
    // if any top level key doesn't satisfy `to_snake_case(key) == key`, return an "invalid key" error pointing to that key
    return top_level_yaml_keys(asset_path).to_set();
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, anim.yaml location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    // extract values from anim.yaml "frames" array under the relevant key (anim_name)
    auto frames = extract_yaml_array(anim_name + ".frames").to_set();
    frames.insert("key"); // insert key frame, it probably won't be mentioned in the frame array, but Porytiles anims MUST have one 
    return frames;
}

ChainableResult<ArtifactKey> 
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/{anim_name}/{frame_name}.png
    // at some point, we can support a config override for the 'data/tilesets/{primary,secondary}' path prefix
    return asset_path;
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
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/anim.yaml
    return asset_path;
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
            frames,
            key_provider_->discover_porytiles_anim_frames(tileset->name(), porytiles_anim_name),
            "tileset load failed",
            std::unique_ptr<Tileset>);

        if (!frames.contains("key")) {
            /*
             * Should this be a required postcondition for discover_porytiles_anim_frames?
             * Or should we just bake this logic directly into TilesetRepo?
             */
            panic("TilesetArtifactKeyProvider::discover_porytiles_anim_frames implementation did not return a 'key' frame");
        }

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
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params and frames in one shot
    // We have the artifact keys that were constructed from anim.yaml values
    // Panic if the params_key doesn't exist
    // Panic if expected frame from frames_keys doesn't exist (this vector will include key.png)
}
```

### Porymap Animation Component
The entire `Animation` object for the Porymap component can be constructed from `include/generated_anim_code.h` file.
If this file doesn't exist yet (first-time import case), then we have a special code-path to figure out everything from `tileset_anims.c`.

**Invariant:** `generated_anim_code.h` will always use `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}`
as the name format for a tileset animation frame array.
Users who want to first-time import a tileset into Porytiles will need to update `tileset_anims.c` to follow this invariant as well.

#### Artifact: Animation Frames
For the Porymap component, by default animation frames are defined via frame variables in `include/generated_anim_code.h`.
However, if this file doesn't exist (first-time import case), then we'll need to search `src/tileset_anims.c`.

##### Project Key Provider
```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) {
    const auto asset_path = // see below
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
    const auto asset_path = // see below
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
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porymap assets, try hardcoded location data/tilesets/{primary,secondary}/{tileset_name}/include/generated_anim_code.h
    if (exists(asset_path)) {
        // This function should find any gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} variables associated
        // with tileset_name and anim_name. For each unique array element, find the associated var declaration.
        // Find the path inside the INCBIN_U16 call, take the basename of the path and remove file extensions.
        // If it matches given frame_name, return the full path from the INCBIN as the key
        return read_g_tileset_anims_var_frame_paths(asset_path, tileset_name, anim_name, frame_name);
    }
    // otherwise, we need to parse vanilla tileset_anims.c to find anim frame names
    // invariant: in tileset_anims.c users are REQUIRED to name their frame arrays following gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} format
    // this function can work similarly to read_g_tileset_anims_var_frame_paths, except it's looking in tileset_anims.c instead
    return read_vanilla_g_tileset_anims_var_frame_paths(tileset_name, anim_name, frame_name);
}
```

#### Artifact: Animation Parameters
asd

##### Project Key Provider
```c++
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porymap assets, return hardcoded location data/tilesets/{primary,secondary}/{tileset_name}/include/generated_anim_code.h
    return asset_path;
}
```

#### Project Reader
The Project reader gets the whole anim in one shot.
TilesetRepo works the same as before, it discovers anims and anim frames via the key provider,
then passes discovered anims/frames into the read_porymap_anim function.

```c++
// TilesetRepo::load snippet

// Unlike the Porytiles case, we can't treat the params key as source of truth,
// since this might be a first-time import with params scattered about tileset_anims.c
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
    // Return FormattableError if expected frame from frames_keys doesn't exist (this vector will include key.png)
}
```

## Precondition / Invariant Summaries
Porytiles should minimize the number of preconditions / invariants users must follow
while maximizing the effectiveness of the preconditions / invariants it chooses.

### Porytiles/Porymap Animation Name Conversions
Key Fact: if a variable name is already in valid PascalCase or snake_case, the `PascalCase <=> snake_case` transformation is an involution.
E.g. you can do `tosnakecase(topascalcase("my_snake_case")) == "my_snake_case"` and `topascalcase(tosnakecase("MyPascalCase")) == "MyPascalCase"`.

Animation names will have three "forms":
- canonical name: user selected, this name appears in anim.yaml
  - NOTE: this name **MUST*** already be in snake_case, `to_snake_case(canonical) == canonical`
  - used for diagnostic messages, it's the key in the tileset component Animation maps and the Animation `name_` field
- PascalCase-ified: generated via `to_pascal_case(canonical)`
  - this name is used for the `{AnimName}` component in the `gTilesetAnims` variable in `include/generated_anim_code.h`
  - `to_snake_case(to_pascal_case(canonical)) == canonical` MUST hold

These conventions give us a way to compile/import animations back and forth in a loss-less way.
The `canonical <=> snake_case` and `canonical <=> PascalCase` transformations must be ***involutions***.

### Porytiles Animations Must Follow `porytiles/anim/{anim_name}` Structure
Once users have onboarded a tileset to Porytiles, they must store anim frames for a given anim under `porytiles/anim/{anim_name}`.
(The absolute location of `porytiles` folder for a given tileset is configurable, but the structure within the `porytiles` folder is not configurable).
This makes the Porytiles animation component key provider, reader, and writer logic simple.
We just assume anim.yaml as source of truth for anim and frame names, and we can easily construct all the frame location paths.

### Animation Frame Pointer Array Must Follow Name Convention: `gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}`
The pokeemerald engine doesn't care how users store anim frames.
Users could have them strewn anywhere. The frames are defined in `tileset_anims.c` like:
```c++
// Frame variables can have any name, frame location can be any path
const u16 foo_bar[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
const u16 baz_bat[] = INCBIN_U16("data/tilesets/some/other/folder/flower/1.4bpp");

// Frame array defines the actual animation, it can be our required source of truth
const u16 *const gTilesetAnims_General_Flower[] = {
    foo_bar,
    baz_bat,
    // more frames here
};
```
As you can see, there is nothing stopping users from naming the frame variable anything they want,
and having the INCBIN path point to any file in any location.

However, in order for Porytiles to make sane assumptions and have reasonably simple code,
we should force users to follow the convention that the main frame array for an animation follows naming convention:
```
gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix}
```
**NOTE:** it's very important that {TilesetName} and {AnimName} are camelCase or PascalCase, since `_` is reserved to delimit the subfields within the variable name.
Porytiles will automatically snake-ify TilesetName and AnimName when when importing into Porytiles format,
and pascal-ify tileset_name and anim_name when compiling from Porytiles format back to Porymap format.

This is only relevant for first time imports.
After a first time import, you will actually have two animations:
- the original animation defined in `tileset_anims.c`
- the Porytiles-managed version defined in `include/generated_anim_code.h`

Users will update the `.callback` field in `headers.h` to link into the Porytiles-managed animation.
The nice part about this it that means Porytiles can follow any conventions it wants for its managed animations.
We can isolate all the special case handling B.S. to the first-time import `tileset_anims.c` codepath.
