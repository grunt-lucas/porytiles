# Porytiles

[![Porytiles Develop Branch Build](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml)
[![Porytiles Nightly Release](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [
`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon
Generation III decompilation projects from [`pret`](https://github.com/pret). Also compatible with [
`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [
`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from
RGBA (or indexed) tile assets.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the newest stable version. If you want
the latest, possibly unstable changes from the [`develop`](https://github.com/grunt-lucas/porytiles/tree/develop)
branch, grab the nightly release instead.

For detailed documentation about Porytiles internal workings, please
see [the Go package page here.](https://pkg.go.dev/github.com/grunt-lucas/porytiles) For tutorials and usage
documentation please see [the wiki](https://github.com/grunt-lucas/porytiles/wiki) (a video tutorial series is coming at
a later date).

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/Resources/Readme/PokemonHearth.png?raw=true)
*Pokémon Hearth by PurrfectDoodle. Tile art inserted via Porytiles. Used with permission.*

- [Porytiles](#porytiles)
    - [Why Should I Use This Tool?](#why-should-i-use-this-tool)
    - [Getting Started](#getting-started)
    - [Compilation Information](#compilation-information)
    - [Note For Aseprite Users](#note-for-aseprite-users)

## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this
way: [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, takes a `.script` file and
generates a corresponding `.inc` file. Comparably, Porytiles takes a source folder containing RGBA (or indexed) tile
assets and generates a corresponding `metatiles.bin`, `metatile_attributes.bin`, indexed `tiles.png`, indexed `anim`
folder, and a populated `palettes` folder -- all as part of your build!

For more info, please
see [this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, go ahead and
follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release).
Alternatively, intrepid users may choose
to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source). Once you've got
Porytiles working, try the demo steps
located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo). Everything else you need to
know about Porytiles can be found [in the wiki](https://github.com/grunt-lucas/porytiles/wiki)
or [in this video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ). I highly recommend reading the wiki
articles in order, or watching the video series in order. The wiki and video series are meant to be complementary. If
you have further questions, I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Compilation Information

Clang+LLVM is the "official" Porytiles build toolchain -- the Porytiles formatting/coverage/tidy scripts rely on
LLVM tools to function. However, most reasonable C++ compilers should be able to build the executable, assuming they
have support for the C++20 standard. I try to maintain compatibility with GCC, should you prefer it over Clang+LLVM.
Once again, [please see this wiki page](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source) if you'd
like to try building Porytiles from source.

## Note For Aseprite Users

GitHub user [PKGaspi](https://github.com/PKGaspi) has created a collection
of [useful scripts here.](https://github.com/PKGaspi/AsepriteScripts) Of particular interest is this [
`export_layers`](https://github.com/PKGaspi/AsepriteScripts/blob/main/scripts/gaspi/export_layers.lua) script, which
allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer
in a separate PNG file.
