# Porytiles

[![Porytiles Develop Branch Build](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/dev_build.yml)
[![Porytiles Nightly Release](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/nightly_release.yml)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [
`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon
Generation III decompilation projects from [`pret`](https://github.com/pret). Also compatible with [
`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [
`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready assets from
RGBA (or indexed) input assets.

## Quick Links
- [Release binaries](https://github.com/grunt-lucas/porytiles/releases)
- [Install via Homebrew](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release#homebrew)
- [Doxygen API documentation](https://grunt-lucas.github.io/porytiles)
- [Using Porytiles - Wiki](https://github.com/grunt-lucas/porytiles/wiki)
- [Introductory YouTube Tutorial (made by a community member, not a rick roll this time I promise)](https://www.youtube.com/playlist?list=PLuyjFojPxF7-O5o_mS6uTBtyYcuyFf_Ce)

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/Resources/Readme/PokemonHearth.png?raw=true)
*Pokémon Hearth by PurrfectDoodle. Tile art inserted via Porytiles. Used with permission.*

- [Porytiles](#porytiles)
  - [Quick Links](#quick-links)
  - [Why Should I Use This Tool?](#why-should-i-use-this-tool)
  - [Getting Started](#getting-started)
  - [Release Cadence](#release-cadence)
  - [Building From Source](#building-from-source)
    - [Dependencies](#dependencies)
    - [Build And Run](#build-and-run)
    - [Notes For macOS](#notes-for-macos)
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
First, go ahead and follow [the release installation instructions in the wiki](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release).
You can use Homebrew,
or you can download a release binary and install it yourself.
Alternatively, intrepid users may choose
to [build Porytiles from source](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source).
Once you've got Porytiles working,
try the demo steps located [at this wiki page](https://github.com/grunt-lucas/porytiles/wiki/My-First-Demo).
Everything else you need to know about Porytiles can be found [in the wiki.](https://github.com/grunt-lucas/porytiles/wiki)
I highly recommend reading the wiki articles in order.
If you have further questions,
I can be found on the `pret` and `RH Hideout` discord servers under the name `grunt-lucas`.

## Release Cadence
Porytiles follows a Continuous Delivery model for release cadence.
Every commit on the `develop` branch gets packaged
and published as a nightly release.
Due to my development style as well as time constraints,
I don't plan on creating versioned releases for Porytiles.
I try to keep the commit history clean,
so you can quickly see what has changed in each nightly release.
Users are encouraged to [install via the brew tap](https://github.com/grunt-lucas/porytiles/wiki/Installing-A-Release#homebrew)
(brew works great on WSL, it's straightforward to set up).
With Homebrew, you can run two quick commands and always be up to date.

## Building From Source
You can use either GCC or Clang,
provided your installation is reasonably recent
and supports most C++20 (and some C++23) features.
[Please see this wiki page](https://github.com/grunt-lucas/porytiles/wiki/Building-From-Source) for more detailed instructions,
should you need them.

### Dependencies
You'll need `zlib` and `libpng` installed on your system,
specifically the static (`.a`) libraries.
Consult your system's package manager for details.
Porytiles's build system will search the system library paths for
`libpng.a` and `libz.a`.
If you'd like to link those libraries dynamically,
or if the CMake configuration is having trouble finding them,
then you'll need to modify `Porytiles1/lib/CMakeLists.txt` appropriately.

You'll also need `cmake` version `3.20` or greater.

### Build And Run
Set up the CMake build folder:
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
```
Then build with:
```
cd build
cmake --build .
```
You can check that everything is working like this:
```
cd ..
./build/Porytiles1/tests/Porytiles1Tests
```
To run the actual tool:
```
./build/Porytiles1/tools/driver/porytiles
```

### Notes For macOS
On macOS,
the CMake configuration command typically finds your system clang compiler.
If you've installed GCC via Homebrew and would like to use that instead,
try this alternative configuration command (assuming you have GCC 15):
```
CXX=g++-15 cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT="" -DCMAKE_CXX_FLAGS="-stdlib=libstdc++ -I/opt/homebrew/opt/gcc/include/c++/15 -L/opt/homebrew/opt/gcc/lib/gcc/15"
```
If you have a different major version of GCC or you are using an Intel Mac,
you may need to tweak this command to match your system.

## Note For Aseprite Users
GitHub user [PKGaspi](https://github.com/PKGaspi) has created a collection
of [useful scripts here.](https://github.com/PKGaspi/AsepriteScripts) Of particular interest is this [
`export_layers`](https://github.com/PKGaspi/AsepriteScripts/blob/main/scripts/gaspi/export_layers.lua) script, which
allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer
in a separate PNG file.
