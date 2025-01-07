# Porytiles

[![Porytiles Develop Branch Build](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml)
[![Porytiles Nightly Release](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon Generation III decompilation projects from [`pret`](https://github.com/pret). Also compatible with [`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from RGBA (or indexed) tile assets.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the newest stable version. If you want the latest, possibly unstable changes from the [`develop`](https://github.com/grunt-lucas/porytiles/tree/develop) branch, grab the nightly release instead.

For detailed documentation about Porytiles internal workings, please see [the Go package page here.](https://pkg.go.dev/github.com/grunt-lucas/porytiles) For tutorials and usage documentation please see [the wiki](https://github.com/grunt-lucas/porytiles/wiki) (a video tutorial series is coming at a later date).

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/Resources/Wiki/PokemonHearth.png?raw=true)
*Pokémon Hearth by PurrfectDoodle. Tile art inserted via Porytiles.*

## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way: [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, takes a `.script` file and generates a corresponding `.inc` file. Comparably, Porytiles takes a source folder containing RGBA (or indexed) tile assets and generates a corresponding `metatiles.bin`, `metatile_attributes.bin`, indexed `tiles.png`, indexed `anim` folder, and a populated `palettes` folder -- all as part of your build!

For more info, please see [this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, go ahead and follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release). Alternatively, intrepid users may choose to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source). Once you've got Porytiles working, try the demo steps located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo). Everything else you need to know about Porytiles can be found [in the wiki](https://github.com/grunt-lucas/porytiles/wiki) or [in this video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ). I highly recommend reading the wiki articles in order, or watching the video series in order. The wiki and video series are meant to be complementary. If you have further questions, I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Compiler Information

Clang+LLVM 16 is the "official" Porytiles build toolchain -- the Porytiles formatting/coverage/tidy scripts rely on LLVM tools to function. However, most reasonable C++ compilers should be able to build the executable, assuming they have support for the C++20 standard. In addition to Clang+LLVM, the Porytiles CI pipeline runs a build job with GCC 13. I try to maintain compatibility with GCC, should you prefer it over Clang+LLVM.

## Note For Aseprite Users
GitHub user [PKGaspi](https://github.com/PKGaspi) has created a collection of [useful scripts here.](https://github.com/PKGaspi/AsepriteScripts) Of particular interest is this [`export_layers`](https://github.com/PKGaspi/AsepriteScripts/blob/main/scripts/gaspi/export_layers.lua) script, which allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer in a separate PNG file.

## Porytiles 1.x

### What Is Porytiles 1.x?
Porytiles 1.x is a from-the-ground-up refactor of Porytiles, now written in Go.

### What Happened To C++?
<sub><sup>I'm borderline ADHD and bored of C++ for the moment...</sup></sub>

I decided that working in Go will be better for the long-term viability of the project for a few reasons:

1. [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, is written in Go. I'd like to foster around Porytiles a community of contribution much like Poryscript's. To help in that effort, Porytiles should be written in a non-arcane language that the community is already using.
2. Go software is easier to distribute. Binaries are self-contained by design. Compilation-from-source is incredibly simple, so folks with non-standard workstations can build Porytiles without having to learn CMake arcana.
3. Go is overall an easier language to use, so it will be easier for me to maintain Porytiles long-term. I have found it is difficult to dive back into a complex C++ codebase when I've been on hiatus (which I often am).

The actual goals of future Porytiles have not changed and are listed in the next section.

### Porytiles 1.x Goals & Highlights
+ Modern, clean Go codebase
+ Fixes for many of the pesky bugs listed in the [Issues](https://github.com/grunt-lucas/porytiles/issues) bank
+ Support for completely custom metatile attributes, like Porymap
+ Better support for spritesheet builds, i.e. builds that don't need to generate metatiles or don't want to follow input sheet size restrictions
+ Support for map-based builds, i.e. draw a layered map as RGBA PNGs and Porytiles will generate Porymap-format tileset and map files
+ Smarter palette assignment using more sophisticated bin-packing solutions
+ Automatic generation of animation C driver code
+ Tons more useful warnings and diagnostics
+ Shell TAB completion for bash, zsh, and fish
+ More decompilation features like animations, maps, etc
+ A GUI client
+ And much more!
