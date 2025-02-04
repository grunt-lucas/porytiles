# Porytiles

[![Porytiles Develop Branch Build](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml)
[![Porytiles Nightly Release](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon Generation III decompilation projects from [`pret`](https://github.com/pret). Also compatible with [`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from RGBA (or indexed) tile assets.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the newest stable version. If you want the latest, possibly unstable changes from the [`develop`](https://github.com/grunt-lucas/porytiles/tree/develop) branch, grab the nightly release instead.

For detailed documentation about Porytiles internal workings, please see [the Go package page here.](https://pkg.go.dev/github.com/grunt-lucas/porytiles) For tutorials and usage documentation please see [the wiki](https://github.com/grunt-lucas/porytiles/wiki) (a video tutorial series is coming at a later date).

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/Resources/Wiki/PokemonHearth.png?raw=true)
*Pokémon Hearth by PurrfectDoodle. Tile art inserted via Porytiles.*

- [Porytiles](#porytiles)
  - [Why Should I Use This Tool?](#why-should-i-use-this-tool)
  - [Getting Started](#getting-started)
  - [Compilation Information](#compilation-information)
  - [Note For Aseprite Users](#note-for-aseprite-users)
  - [Porytiles 0.x](#porytiles-0x)
    - [What Is Porytiles 0.x?](#what-is-porytiles-0x)
  - [Porytiles 1.x](#porytiles-1x)
    - [What Is Porytiles 1.x?](#what-is-porytiles-1x)
    - [Why?](#why)
    - [What Happened To C++?](#what-happened-to-c)
    - [Porytiles 1.x Goals \& Highlights](#porytiles-1x-goals--highlights)


## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way: [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, takes a `.script` file and generates a corresponding `.inc` file. Comparably, Porytiles takes a source folder containing RGBA (or indexed) tile assets and generates a corresponding `metatiles.bin`, `metatile_attributes.bin`, indexed `tiles.png`, indexed `anim` folder, and a populated `palettes` folder -- all as part of your build!

For more info, please see [this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, go ahead and follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release). Alternatively, intrepid users may choose to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source). Once you've got Porytiles working, try the demo steps located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo). Everything else you need to know about Porytiles can be found [in the wiki](https://github.com/grunt-lucas/porytiles/wiki) or [in this video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ). I highly recommend reading the wiki articles in order, or watching the video series in order. The wiki and video series are meant to be complementary. If you have further questions, I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Compilation Information

Clang+LLVM is the "official" Porytiles 0.x build toolchain -- the Porytiles formatting/coverage/tidy scripts rely on LLVM tools to function. However, most reasonable C++ compilers should be able to build the executable, assuming they have support for the C++20 standard. In addition to Clang+LLVM, the Porytiles CI pipeline runs a build job with GCC. I try to maintain compatibility with GCC, should you prefer it over Clang+LLVM. Once again, [please see this wiki page](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source) if you'd like to try building Porytiles 0.x from source.

## Note For Aseprite Users
GitHub user [PKGaspi](https://github.com/PKGaspi) has created a collection of [useful scripts here.](https://github.com/PKGaspi/AsepriteScripts) Of particular interest is this [`export_layers`](https://github.com/PKGaspi/AsepriteScripts/blob/main/scripts/gaspi/export_layers.lua) script, which allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer in a separate PNG file.

## Porytiles 0.x

### What Is Porytiles 0.x?
Porytiles 0.x is the "legacy" version of Porytiles, written in C++. Currently, the [Porytiles Releases Tab](https://github.com/grunt-lucas/porytiles/releases) contains builds from the code in the `Porytiles-0.x` directory. This will be the supported version of Porytiles for the forseeable future. It's very usable in its current state, and there is significant documentation [over at the wiki](https://github.com/grunt-lucas/porytiles/wiki) to get you started. There will be occasional tweaks and bugfixes, which should show up in the [Porytiles Releases Tab](https://github.com/grunt-lucas/porytiles/releases) as nightlies. Check back occasionally and always download the latest version. However, most large new features (and some bugs too) will not be fixed in this version. Rather, they will be fixed in an upcoming Porytiles 1.x redux. More on that below.

## Porytiles 1.x

### What Is Porytiles 1.x?
Porytiles 1.x is a from-the-ground-up refactor of Porytiles, now written in Go.

### Why?
Instead of releasing a 1.0.0 Porytiles based on the C++ codebase, I have instead decided to start working on a from-the-ground-up refactor of Porytiles, which will be known as Porytiles 1.x until it officially releases. The reason for this: the process of rapid iterative development on Porytiles 0.x has accrued significant technical debt. At the moment, the Porytiles 0.x code is so messy that I am having trouble adding features or fixing bugs without introducing further issues and gotchas. As such, building Porytiles 1.x "from scratch" will give it the best foundation for a bright future. Porytiles 1.x will begin as a feature-for-feature port in the Go language. Once it has parity with Porytiles 0.x functionality, I will release a Porytiles 1.0.0 based on the Go codebase. Then I will begin adding new features, fixing the complex bugs and issues, etc. Porytiles 1.x Go code lives in the root of this repository.

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
