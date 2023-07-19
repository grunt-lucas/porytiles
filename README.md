# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby),
[`pokeemerald`](https://github.com/pret/pokeemerald), and [`pokefirered`](https://github.com/pret/pokefirered) Pokémon
Generation 3 decompilation projects. Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from an RGBA
tilesheet.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the latest stable version, or check out
the [`trunk`](https://github.com/grunt-lucas/porytiles/tree/trunk) branch to get the upcoming changes listed in the
[changelog](https://github.com/grunt-lucas/porytiles/blob/trunk/CHANGELOG.md).

## Why Would I Use This?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way:
just like [Poryscript](https://github.com/huderlem/poryscript) takes a `.script` file and generates a corresponding `.inc`
file as part of your build, Porytiles takes an RGBA `top.png`, `middle.png`, and `bottom.png` and generates a corresponding
`metatiles.bin`, `tiles.png`, and `palettes/*.pal` as part of your build!

For more info, please see
[this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, clone and build Porytiles like so. You must have a version of Clang or GCC that supports C++20 features.

```
git clone https://github.com/grunt-lucas/porytiles.git
cd porytiles
make
./release/bin/porytiles --help
```

Once you've cloned and built Porytiles, try the following little demo.

1. Open a pokeemerald project in Porymap, one that has triple-layer metatiles enabled. Porytiles requires that you use
   triple layer metatiles. If you don't know what this is or how to enable, please see here:
   https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles

2. In Porymap, select `Tools -> New Tileset`. Create a primary set called `PorytilesTest`.

3. In Porymap, right click one of the map groups and create a new map called `PorytilesTestMap`. For this map's primary
   tileset, select `gTileset_PorytilesTest`.

4. Run the following command, replacing `path/to/project` with the path to your project:

```
./release/bin/porytiles compile \
-o path/to/project/data/tilesets/primary/porytiles_test \
res/examples/basic_primary_set/bottom.png \
res/examples/basic_primary_set/middle.png \
res/examples/basic_primary_set/top.png
```

5. In Porymap, select `File -> Reload Project`.

6. The metatile picker on the right should now show a basic tileset! Start mapping on your new map, and then save.

7. Open one of the layer PNGs in `res/examples/basic_primary_set` and edit it. Re-run the command from Step 4, and then
   reload Porymap again like in Step 5. You should see your changes reflected in both the map and the metatile picker.

8. Enjoy!

## Planned Features

|  Feature  |  Completed?  |  Links  |
|-----------|--------------|---------|
| Generate `palettes/*.pal`   | ✅ | [unreleased in trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Generate `tiles.png`        | ✅ | [unreleased in trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Generate `metatiles.bin`    | ✅ | [unreleased in trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Proper support for secondary tilesets (i.e. tile/palette sharing with a given primary set)    | ❌ |  |
| Support for animated tiles, i.e. stable placement in `tiles.png`   | ❌ |  |
| Decompile compiled tilesets back into three layer PNGs   | ❌ |  |
| Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`   | ❌ |  |
| Proper support for dual layer tiles (requires modifying metatile attributes file)    | ❌ |  |
| Support `.ora` files as input to compile command   | ❌ |  |
