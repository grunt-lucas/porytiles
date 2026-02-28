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
    "counter_max": 256
  }
}
```

### Refactor `tileset.animations` configuration options to be animation/subtile specific
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
  std::vector<std::optional<AnimPalResolutionStrategy>> pal_resolution_strategies;
  FrameLinking linking{FrameLinking::automatic};
};
```

The new YAML format could look like this, with anim names as keys into a `std::map<string, AnimConfig>`:
```yaml
tileset:
  animations:
    global_palette_resolution_strategy: scan-local-metatiles
    key_frame_resolution_strategy: warning
    configs:
      flower:
        frame_linking: automatic
        # Array must have same size as frame.subtiles.size()
        # If present, must specify a strategy for each subtile.
        # If user wants default global to be used, specify a "_" to signify std::nullopt.
        palette_resolution_strategies: [pal0, scan-local-metatiles, _, pal0]
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
Example:
```json
{
  "flower": {
    "frames": ["center", "right", "left"],
    "frame_order": ["center", "right", "center", "left"],
    "frame_factor": 8,
    "frame_offset": 2,
    "counter_max": 256, // this is sPrimaryTilesetAnimCounter
    "overrides": [
      {
        "id": 22,
        "layer": "bottom",
        "subtile": "northwest",
        "hflip": true,
        "vflip": true,
        // This value is internal to the animation's frame, so it's relative within tiles.png.
        // To compute absolute position of tile, do tile_offset_ + frame_subtile, where tile_offset_
        // is this animation's offset within tiles.png (defined in AnimParams).
        "frame_subtile": 0
      },
      {
        "id": 22,
        "layer": "bottom",
        "subtile": "northeast",
        "hflip": true,
        "vflip": true,
        "frame_subtile": 1
      }
    ]
  }
}
```
If `AnimConfig` for this animation specifies `FrameLinking::manual`,
use these hardcoded entries.
It should be pretty easy to modify the compiler logic slightly to do this.
Then we just emit the TilemapEntry based on what the user is asking for here.
We'll need to modify the `AnimParams` class to contain a set of manual overrides.

Automatic frame linking means we'll try to generate key.png when decompiling,
and try to use key frame linking when compiling. This is the default.

Manual frame linking means we'll write "overrides" to anim.json when decompiling,
and use those overrides when creating metatiles (effectively ignoring key frame entirely).
Config values related to the automatic linking system will be effectively ignored.

After making this change, `key.png` becomes optional.
Porytiles will error if it's not present and `FrameLinking::automatic` is set,
otherwise it carries on.

If user specifies `FrameLinking::automatic` for an animation but also provides manual overrides, should we:
- throw an error?
- warn the user and then ignore them?
- use them anyway, applying them *before* we try linking via key frame?
- use them anyway, applying them *after* (and possibly overwriting) key frame links?

Likewise, if user specifies `FrameLinking::manual` for an animation but also provides a `key.png`, should we:
- throw an error?
- warn the user and then ignore it?
- use it anyway, applying it *before* we write the manual overrides?
- use them anyway, applying it *after* (and possibly overwriting) the manual overrides?

It's worth thinking through this.
Which behavior makes the most sense?
Or should it be configurable?

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
Users can then import by specifying `frame_linking: manual`,
which will pull links directly from metatiles and skip past the code that throws the error.
This will work for both the import and net-new case, but it's not as clean as supporting natively.

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
