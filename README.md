# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

Overworld tileset compiler for use with the [`pokeemerald`](https://github.com/pret/pokeemerald),
[`pokefirered`](https://github.com/pret/pokefirered), and [`pokeruby`](https://github.com/pret/pokeruby) Pokémon
Generation 3 decompilation projects from [`pret`](https://github.com/pret). Also compatible with
[`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [`rh-hideout`](https://github.com/rh-hideout).
Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from RGBA (or indexed) tile assets.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the latest stable version, or check out
the [`trunk`](https://github.com/grunt-lucas/porytiles/tree/trunk) branch to get the upcoming changes listed in the
[changelog](https://github.com/grunt-lucas/porytiles/blob/trunk/CHANGELOG.md).

For detailed documentation about Porytiles features and internal workings, please see
[the wiki](https://github.com/grunt-lucas/porytiles/wiki) or [the video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ).

## Table of Contents
- [Porytiles](#porytiles)
  - [Table of Contents](#table-of-contents)
  - [Why Should I Use This Tool?](#why-should-i-use-this-tool)
  - [Getting Started](#getting-started)
  - [Compiler Information](#compiler-information)

## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way:
just like [Poryscript](https://github.com/huderlem/poryscript) takes a `.script` file and generates a corresponding `.inc`
file as part of your build, Porytiles takes an source folder containing RGBA (or indexed) tile assets and generates a
corresponding `metatiles.bin`, `metatile_attributes.bin`, indexed `tiles.png`, indexed `anim` folder, and a populated
`palettes` folder -- all as part of your build!

For more info, please see
[this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, go ahead and follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release).
Alternatively, intrepid users may choose to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source).
Once you've got Porytiles working, try the demo steps located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo).
Everything else you need to know about Porytiles can be found [in the wiki](https://github.com/grunt-lucas/porytiles/wiki)
or [in this video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ). I highly recommend reading the wiki
articles in order, or watching the video series in order. The wiki and video series are meant to be complementary. If
you have further questions, I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Compiler Information

Clang+LLVM 16 is the "official" Porytiles build toolchain -- the Porytiles formatting/coverage/tidy scripts rely on LLVM
tools to function. However, most reasonable C++ compilers should be able to build the executable, assuming they have
support for the C++20 standard. In addition to Clang+LLVM, the Porytiles CI pipeline runs a build job with GCC 13. I
try to maintain compatibility with that compiler, should you prefer it over Clang+LLVM.
