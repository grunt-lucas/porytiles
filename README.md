# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

Overworld tileset compiler for use with the [`pokeemerald`](https://github.com/pret/pokeemerald),
[`pokefirered`](https://github.com/pret/pokefirered), and [`pokeruby`](https://github.com/pret/pokeruby) Pokémon
Generation 3 decompilation projects from [`pret`](https://github.com/pret). Builds
[Porymap](https://github.com/huderlem/porymap)-ready tilesets from an RGBA tilesheet.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the latest stable version, or check out
the [`trunk`](https://github.com/grunt-lucas/porytiles/tree/trunk) branch to get the upcoming changes listed in the
[changelog](https://github.com/grunt-lucas/porytiles/blob/trunk/CHANGELOG.md).

For detailed documentation about Porytiles features and internal workings, please see
[the wiki.](https://github.com/grunt-lucas/porytiles/wiki/Porytiles-Homepage)

## Table of Contents
- [Porytiles](#porytiles)
  - [Table of Contents](#table-of-contents)
  - [Why Should I Use This Tool?](#why-should-i-use-this-tool)
  - [Planned Features](#planned-features)
  - [Building](#building)
  - [Getting Started](#getting-started)

## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way:
just like [Poryscript](https://github.com/huderlem/poryscript) takes a `.script` file and generates a corresponding `.inc`
file as part of your build, Porytiles takes an input folder containing RGBA tile assets and generates a corresponding
`metatiles.bin`, indexed `tiles.png`, and `palettes/*.pal` as part of your build!

For more info, please see
[this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Planned Features

|  Feature  |  Completed?  |  Available In Version:  |
|-----------|--------------|---------|
| Generate `palettes/*.pal`   | ✅ | [0.0.2](https://github.com/grunt-lucas/porytiles/releases/tag/0.0.2) |
| Generate `tiles.png`        | ✅ | [0.0.2](https://github.com/grunt-lucas/porytiles/releases/tag/0.0.2) |
| Generate `metatiles.bin`    | ✅ | [0.0.2](https://github.com/grunt-lucas/porytiles/releases/tag/0.0.2) |
| Proper support for secondary tilesets (i.e. tile/palette sharing with a given primary set)    | ✅ | [0.0.3](https://github.com/grunt-lucas/porytiles/releases/tag/0.0.3) |
| Support for animated tiles, i.e. stable placement in `tiles.png`   | ✅ | [trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Decompile compiled tilesets back into three layer PNGs   | ❌ |  |
| Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`   | ❌ |  |
| Proper support for dual layer tiles (requires modifying metatile attributes file)    | ❌ |  |
| Support `.ora` files as input to compile command   | ❌ |  |

## Building

LLVM 16 (i.e. `clang++`) is the "official" Porytiles build toolchain. However, most reasonable C++ compilers should
work as well, assuming they have support for the C++20 standard. The Porytiles CI pipeline has a GCC 13 target, so that
toolchain is confirmed to build Porytiles correctly. In the future, I intend to add more compiler targets (like MSVC).

## Getting Started

First, clone and build Porytiles. (Alternatively, you can skip the build step by downloading a release binary from the
[Releases](https://github.com/grunt-lucas/porytiles/releases) tab.)

```
git clone https://github.com/grunt-lucas/porytiles.git
cd porytiles
CXX=clang++ make check
./release/bin/porytiles --help
```

Once you've cloned and built Porytiles, try the following little demo.

1. Open a `pokeemerald` project (`pokefirered` and `pokeruby` are also supported via command line options) in Porymap,
   one that has triple-layer metatiles enabled. Porytiles requires that you use triple layer metatiles (for now). If you
   don't know what this is or how to enable, please see here:
   https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles

2. In Porymap, select `Tools -> New Tileset`. Create a primary set called `PorytilesPrimaryTest`.

3. In Porymap, right click one of the map groups and create a new map called `PorytilesTestMap`. For this map's primary
   tileset, select `gTileset_PorytilesPrimaryTest`. Then save the map.

4. Run the following command, replacing `path/to/project` with the path to your project:

```
./release/bin/porytiles compile-primary -o path/to/project/data/tilesets/primary/porytiles_primary_test res/tests/simple_metatiles_2/primary
```

5. In Porymap, select `File -> Reload Project`.

6. The metatile picker on the right should now show a basic tileset! Start mapping on your new map, and then save.

7. Open one of the layer PNGs in `res/examples/basic_primary_set` and edit it. Re-run the command from Step 4, and then
   reload Porymap again like in Step 5. You should see your changes reflected in both the map and the metatile picker.

8. You can stop here and enjoy! Or, if you want to bring in a secondary set as well, please read on.

9. In Porymap, select `Tools -> New Tileset`. Create a secondary set called `PorytilesSecondaryTest`.

10. In Porymap, make sure `PorytilesTestMap` is open. Use the tileset selector to change `PorytilesTestMap`'s secondary
    tileset to `gTileset_PorytilesSecondaryTest`. Then save the map.

11. Run the following command, replacing `path/to/project` with the path to your project:

```
./release/bin/porytiles compile-secondary -o path/to/project/data/tilesets/secondary/porytiles_secondary_test res/tests/simple_metatiles_2/secondary res/tests/simple_metatiles_2/primary
```

12. In Porymap, select `File -> Reload Project`.

13. Your map now has a custom primary and secondary tileset! Feel free to keep editing these sets to see what Porytiles
    can do!
