# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles%20GCC/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)
[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles%20Clang/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

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
$ git clone https://github.com/grunt-lucas/porytiles.git
$ cd porytiles
$ make
$ ./release/bin/porytiles --help
```

Once you've cloned and built Porytiles, try the following little demo:

```
./release/bin/porytiles compile-raw -o ./output --tiles-png-pal-mode=true-color res/tests/primary_set.png
```

This should create an `output` folder in the current working directory containing a `tiles.png` and pal files
corresponding to the inputs from `res/tests/primary_set.png`. Porytiles will soon support generation of
`metatiles.bin` as well.

## Planned Features

|  Feature  |  Completed?  |  Links  |
|-----------|--------------|---------|
| Generate `tiles.png`        | ✅ | [unreleased in trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Generate `palettes/*.pal`   | ✅ | [unreleased in trunk](https://github.com/grunt-lucas/porytiles/tree/trunk) |
| Generate `metatiles.bin`    | ❌ |  |
| Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`   | ❌ |  |
| Support for animated tiles, i.e. stable placement in `tiles.png`   | ❌ |  |
| Decompile compiled tilesets back into three layer PNGs   | ❌ |  |

