# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Porytiles%20Develop%20Branch%20Build/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)
[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Porytiles%20Nightly%20Release/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon Generation 3 decompilation projects from [`pret`](https://github.com/pret). Also compatible with [`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready tilesets from RGBA (or indexed) tile assets.

Please see the [Releases](https://github.com/grunt-lucas/porytiles/releases) for the newest stable version. If you want the latest, possibly unstable changes from the [`develop`](https://github.com/grunt-lucas/porytiles/tree/develop) branch, grab the nightly release instead.

For detailed documentation about Porytiles features and internal workings, please see [the wiki](https://github.com/grunt-lucas/porytiles/wiki) or [the video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ).

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/img/PokemonHearth.png?raw=true)
*Pokémon Hearth by PurrfectDoodle. Tile art inserted via Porytiles.*

## Why Should I Use This Tool?

Porytiles makes importing from-scratch tilesets (or editing existing tilesets) easier than ever. Think of it this way: [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, takes a `.script` file and generates a corresponding `.inc` file. Comparably, Porytiles takes a source folder containing RGBA (or indexed) tile assets and generates a corresponding `metatiles.bin`, `metatile_attributes.bin`, indexed `tiles.png`, indexed `anim` folder, and a populated `palettes` folder -- all as part of your build!

For more info, please see [this wiki page which explains what Porytiles can do in more detail.](https://github.com/grunt-lucas/porytiles/wiki/Why-Should-I-Use-This-Tool%3F)

## Getting Started

First, go ahead and follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release). Alternatively, intrepid users may choose to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source). Once you've got Porytiles working, try the demo steps located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo). Everything else you need to know about Porytiles can be found [in the wiki](https://github.com/grunt-lucas/porytiles/wiki) or [in this video tutorial series](https://www.youtube.com/watch?v=dQw4w9WgXcQ). I highly recommend reading the wiki articles in order, or watching the video series in order. The wiki and video series are meant to be complementary. If you have further questions, I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Compiler Information

Clang+LLVM 16 is the "official" Porytiles build toolchain -- the Porytiles formatting/coverage/tidy scripts rely on LLVM tools to function. However, most reasonable C++ compilers should be able to build the executable, assuming they have support for the C++20 standard. In addition to Clang+LLVM, the Porytiles CI pipeline runs a build job with GCC 13. I try to maintain compatibility with GCC, should you prefer it over Clang+LLVM.
