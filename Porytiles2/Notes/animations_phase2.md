# Animations - Phase 2

## Summary
Porytiles "phase 1" animation handling has been implemented,
and it is moderately successful.
We can round-trip, losslessly compile/decompile simple animations like flower,
as well as some of the more complex pokeemerald animations like General's water animation.
However, there are still some gaps preventing clean handling of some tilesets,
including FireRed's General tileset.
This document outlines the "Phase 2" changes we'll need to finalize the animation handling system.

## Animation Edge Cases
TODO

## Mini-Projects
The following is a step-by-step list of mini-projects that will solve for the issues identified in the sections above.

### ~~Change `anim.yaml` to `anim.json`~~
Since Porytiles overwrites it every time, YAML seems unintuitive. Why?
Because users might reasonably do things like put comments,
and wonder why their comments keep getting clobbered.
JSON is better since comments are disallowed by default and syntax is simpler.
Users won't be confused when their idiosyncrasies get clobbered.
Normally, per Porytiles design rules, user-edited config files use YAML.
However, based on the analysis above, JSON is a better fit in this case.
Also, this file isn't technically a config file.
It's really more of a text-based format for domain layer concepts.

```yaml
flower:
  frames: [center, right, left]
  frame_order: [center, right, center, left]
  frame_factor: 8
  frame_offset: 2
  counter_max: 256
```

```json
{
  "flower": {
    "frames": ["center", "right", "left"],
    "frame_order": ["center", "right", "center", "left"],
    "frame_factor": 8,
    "frame_offset": 2,
    "counter_max": 256 // this is sPrimaryTilesetAnimCounter
  }
}
```

### ~~Refactor `tileset.animations` configuration options to be animation/subtile specific~~
Currently, we only allow users to specify per-animation config for palette resolution strategy:
```yaml
tileset:
  animations:
    palette_resolution_strategy_overrides:
      flower: palette-00
      water: internal-png-palette
```
This is not really granular enough.
We need to support a cleaner way to do per-anim, per subtile configuration for more than just palette resolution.
Perhaps a domain layer config class called `AnimConfig` that can be set via the YAML.

```c++
struct AnimConfig
{
  std::string anim_name;
  std::optional<AnimPalResolutionStrategy> pal_resolution_strategy{std::nullopt};
  std::vector<std::optional<AnimPalResolutionStrategy>> per_tile_pal_resolution_strategies;
  FrameLinking linking{FrameLinking::automatic};
};
```

The new YAML format could look like this, with anim names as keys into a `std::map<string, AnimConfig>`:
```yaml
tileset:
  animations:
    global_palette_resolution_strategy: scan-local-metatiles
    global_key_frame_resolution_strategy: warning
    configs:
      flower:
        frame_linking: automatic
        palette_resolution_strategy: pal2
        # Array must have same size as frame.subtiles.size()
        # If present, must specify a strategy for each subtile.
        # If user wants default global to be used, specify a "_" to signify std::nullopt.
        per_tile_palette_resolution_strategies: [pal0, scan-local-metatiles, _, pal0]
      water:
        frame_linking: manual
```

One TODO: should we allow per subtile `key_frame_resolution_strategy` config?
It might be nice, if users want to manually mangle one tile but not another.
But this is probably a very niche case and thus not high priority.
What we should do though, is rename to `global_key_frame_resolution_strategy` to future-proof ourselves.

**Note**
I don't think we can reasonable support CLI options for this config map.
That means that if users need a complex configuration for a first-time import,
they'll either need to:
- add the config temporarily to their global config yaml
- create the gTileset_Foo directory themselves and set up the config yaml there

This is fine. They can always run `dump-tileset-config` before importing to ensure the config looks correct.

### Support manual animation overrides in `anim.json`
The goal is to provide users an alternative to `key.png`-based animation linking.

#### Example `anim.json` file with overrides
```json
{
  "flower": {
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
    "frame_factor": 8,
    "frame_offset": 2,
    "counter_max": 256,
    // Overrides are an array of objects that connect a layer PNG location to a tilemap entry.
    // This effectively bypasses the need to do key.png matching.
    "overrides": [
      {
        // This is the canonical ordering for these fields, use nohlman::ordered_json to preserve
        "id": 22,
        "layer": "bottom",
        "subtile": "northwest",
        // This value is internal to the animation's frame, so it's relative within tiles.png.
        // To compute absolute position of tile, do tile_offset_ + frame_subtile, where tile_offset_
        // is this animation's offset within tiles.png (defined in AnimParams).
        "frame_subtile": 0,
        "pal_index": 2,
        "hflip": true,
        "vflip": true
      },
      {
        "id": 22,
        "layer": "bottom",
        "subtile": "northeast",
        "frame_subtile": 1,
        "pal_index": 0,
        "hflip": true,
        "vflip": true
      }
    ]
  }
}
```

```c++
/**
 * @brief A manual override that maps a specific metatile entry to an animation subtile.
 *
 * @details
 * When using manual frame linking, users explicitly declare which metatile entries reference which animation subtiles.
 * Each override entry specifies the metatile position (id, layer, subtile), flip flags, palette index, and which
 * subtile within the animation frame to use.
 *
 * All fields are required when specifying overrides in anim.json.
 */
struct AnimOverrideEntry {
    /// The metatile ID this override applies to (corresponds to JSON "id" field).
    std::size_t metatile_id;
    /// The layer within the metatile (bottom, middle, or top).
    metatile::Layer layer;
    /// The subtile position within the layer (northwest, northeast, southwest, southeast).
    metatile::Subtile subtile;
    /// Zero-based index into the animation's tile range (tile_offset + frame_subtile = actual tile index).
    std::size_t frame_subtile;
    /// The palette index to use for this tile.
    std::size_t pal_index;
    /// Whether the tile is horizontally flipped.
    bool h_flip;
    /// Whether the tile is vertically flipped.
    bool v_flip;
};
```

#### `FrameLinking::automatic` mode
This is the default setting.

If user specifies `FrameLinking::automatic` for an animation but also provides manual overrides in `anim.json`,
we should generate a warning and then ignore them.

##### Compilation Notes
No changes from current Porytiles behavior.
Current behavior is already `automatic` mode.
Error if `key.png` isn't present.

##### Decompilation Notes
No changes from current Porytiles behavior.
Current behavior is already `automatic` mode.

#### `FrameLinking::manual` mode
It should be pretty easy to modify the compiler logic slightly to do this.
Then we just emit the TilemapEntry based on what the user is asking for here.
We'll need to modify the `AnimParams` class to contain a vector of `AnimOverrideEntry`.
We should keep the animation registration in place,
so that we can support `tiles-edit-mode: optimize`.
That way, each animation has a fixed allocated offset within `tiles.png`,
and the `frame_subtile` field of the override indexes into that range.

##### Compilation Notes
If `AnimConfig` for this animation specifies `FrameLinking::manual`,
use the `"overrides"` entries in `anim.json`.
If none were provided, throw an error.
If `key.png` is present, generate a warning that it will be ignored.

The regular frames can be compiled as normal using palette matching,
choosing the first matching palette to resolve the RGBA tiles into indexed tiles.
The user is responsible for ensuring that the overrides reference the correct palette
so that the in-game assets look correct.

##### Decompilation Notes
Manual frame linking means we'll write "overrides" to anim.json when decompiling,
and use those overrides when creating metatiles (effectively ignoring key frame entirely).
Config values related to the automatic linking system will be effectively ignored.

#### `FrameLinking::hybrid` mode
Same as `FrameLinking::automatic`,
except after doing regular `key.png`-based assignment,
do another pass that applies any manual overrides that are present.

No need to implement this yet. Just include a branch for it that panics with "TODO: implement" message.

#### Config Updates
Add a global `FrameLinking` setting to mirror the other global fallbacks, i.e.:
```yaml
tileset:
  animations:
    global_frame_linking: manual
```

### Solve the "multiple palettes for a single frame subtile" issue
`AnimDecompiler` has this branch and note:
```c++
if (found_for_subtile.size() > 1) {
   /*
    * A single tile index can be referenced by multiple metatile entries with different palette indices.
    * This is valid GBA behavior — the hardware selects palette per metatile entry, not per tile.
    *
    * TODO: ANIM: a more sophisticated approach could support multi-palette variants per subtile. For now,
    * we treat this as an error because picking one palette arbitrarily would produce incorrect RGBA output
    * in the layer PNGs, breaking recompilation (the other palette version would be lost).
    * 
    * We need to figure out a better way to handle this.
    */
// ...
}
```
We need to develop a sound way to handle this case. It gets triggered by FireRed General. Ideas:

1. Create another branching path for first-time imports. If we detect this case, create a separate animation for each palette variant.
2. Implement support for palette alignment (see borytiles). The problem here is that this isn't always reliable, given the limitations of the pagination problem solver. It also doesn't work cleanly with the key frame system. You'd still have a fundamental ambiguity.
3. Leave as error condition and force users to resolve manually before importing for the first time.

For animations that hit this case, the main issues are:
1. Key frame ambiguity (with only one key frame, you can't signal to Porytiles which pal you want)
2. Palette alignment: you don't hit this if you import and keep pals:locked (pals already aligned), but we don't have a clean way to support this use-case for net-new tilesets

I think the cleanest way forward is to simply keep this as an error condition.
Users can then import by specifying `frame_linking: manual`
along with another config option that skips the code that throws this error.
We'll figure out exactly how to do this later.

### TODO: How to handle VDests system? See Evergrande/Mauville city flowers, Route104 windy water, etc
```c++
static void QueueAnimTiles_Rustboro_WindyWater(u16 timer_div, u8 timer_mod) {
    timer_div -= timer_mod;
    timer_div %= ARRAY_COUNT(gTilesetAnims_Rustboro_WindyWater);
    if (gTilesetAnims_Rustboro_WindyWater[timer_div]) {
        AppendTilesetAnimToBuffer(gTilesetAnims_Rustboro_WindyWater[timer_div], gTilesetAnims_Rustboro_WindyWater_VDests[timer_mod], 4 * TILE_SIZE_4BPP);
    }
}
```
The timer div and mod parameters can be specified in the anim params file.
Everything else in this function is stock.
Spend some time analyzing the VDests stuff to figure out the best way to handle it.

### TODO: figure out how to support BattleDome floor light blending
The Battle Dome tileset does some fancy stuff. See bottom of `pokeemerald`'s `tileset_anims.c`.
