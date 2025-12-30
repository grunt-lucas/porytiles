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
Invariant: For Project impl, Porytiles anim frames must be stored in `porytiles/anim/{anim_name}/{frame_name}`

```c++
ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim
    // read anim.yaml from this location and return all top level keys
    return top_level_yaml_keys(asset_path).to_set();
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/{anim_name}
    // extract values from anim.yaml "frames" array under the relevant key
    auto frames = extract_yaml_array(anim_name + ".frames").to_set();
    frames.insert("key"); // insert key frame
    return frames;
}

ChainableResult<ArtifactKey> 
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    const auto asset_path = // see below
    // read isSecondary for 'tileset_name' from headers.h to figure out {primary,secondary}
    // for Porytiles assets, location is hardcoded to data/tilesets/{primary,secondary}/{tileset_name}/porytiles/anim/{anim_name}/{frame_name}
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
ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params and frames in one shot
    // We have the artifact keys that were constructed from anim.yaml values
    // Return FormattableError if expected anim directory doesn't exist
    // Return FormattableError if expected frame from frames_keys doesn't exist (this vector will include key.png)
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
    if (exists(asset_path)) {
        return read_all_g_tileset_anims_vars(asset_path).to_set();
    }
    // otherwise, we need to parse vanilla tileset_anims.c to find anim names
    // invariant: in tileset_anims.c users are REQUIRED to name their frame arrays following gTilesetAnims_{TilesetName}_{AnimName}{_optional_suffix} format
    // in this case, the anim names will be to_snake_case({anim}), where {anim} is the anim part of the gTilesetAnims variable
    return read_vanilla_tileset_anims().to_set();
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) {
    const auto asset_path = // see below
    
}

ChainableResult<ArtifactKey> 
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) {
    const auto asset_path = // see below
    
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
ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const std::vector<ArtifactKey> &frames_keys) {
    // Read params and frames in one shot
    // We have the artifact keys that were constructed from generated_anim_code.h
    // Return FormattableError if expected frame from frames_keys doesn't exist (this vector will include key.png)
}
```

## Precondition / Invariant Summaries
Porytiles should minimize the number of preconditions / invariants users must follow
while maximizing the effectiveness of the preconditions / invariants it chooses.

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
This is only relevant for first time imports.
After a first time import, you will actually have two animations:
- the original animation defined in `tileset_anims.c`
- the Porytiles-managed version defined in `include/generated_anim_code.h`

Users will update the `.callback` field in `headers.h` to link into the Porytiles-managed animation.
The nice part about this it that means Porytiles can follow any conventions it wants for its managed animations.
We can isolate all the special case handling B.S. to the first-time import `tileset_anims.c` codepath.
