# Porytiles

[![Porytiles Snapshot Release](https://github.com/grunt-lucas/porytiles/actions/workflows/snapshot_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/snapshot_release.yml)
[![Porytiles Versioned Release](https://github.com/grunt-lucas/porytiles/actions/workflows/versioned_release.yml/badge.svg)](https://github.com/grunt-lucas/porytiles/actions/workflows/versioned_release.yml)

Overworld tileset compiler for use with the [`pokeruby`](https://github.com/pret/pokeruby), [
`pokefirered`](https://github.com/pret/pokefirered), and [`pokeemerald`](https://github.com/pret/pokeemerald) Pokémon
Generation III decompilation projects from [`pret`](https://github.com/pret). Also compatible with [
`pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion) from [
`rh-hideout`](https://github.com/rh-hideout). Builds [Porymap](https://github.com/huderlem/porymap)-ready assets from
RGBA (or indexed) input assets.

## Quick Links
- [Release binaries](https://github.com/grunt-lucas/porytiles/releases)
- [Install via Homebrew](#release-cadence)
- [Doxygen API documentation](https://grunt-lucas.github.io/porytiles)
- [Using Porytiles - Wiki](https://github.com/grunt-lucas/porytiles/wiki)
- [Introductory YouTube Tutorial (made by a community member, not a rick roll this time I promise)](https://www.youtube.com/playlist?list=PLuyjFojPxF7-O5o_mS6uTBtyYcuyFf_Ce)

![PokemonHearth](https://github.com/grunt-lucas/porytiles/blob/develop/resources/readme/PokemonHearth.png?raw=true)
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
Porytiles publishes both versioned releases and a rolling snapshot.

**Versioned releases** follow semantic versioning (`vX.Y.Z`) and are tagged on
the `master` branch.
Each tag triggers a permanent GitHub release with platform-specific zip files
and updates the `porytiles` Homebrew formula.
The [CHANGELOG](./CHANGELOG.md) lists what changed in each release.

**Snapshot releases** are published automatically on every push to `develop`.
They land at the rolling `snapshot` GitHub release and update the
`porytiles-snapshot` Homebrew formula.
Snapshots are for users who want the latest changes;
they are not considered stable and the tag is force-replaced on every push.

Install the latest versioned release via Homebrew:
```sh
brew install grunt-lucas/porytiles/porytiles
```

Install the latest rolling snapshot via Homebrew:
```sh
brew install grunt-lucas/porytiles/porytiles-snapshot
```

Both formulas install two binaries:
`porytiles` (the modern compiler) and `porytiles-legacy` (the preserved
pre-1.0 compiler).
Homebrew works on Linux, macOS, and WSL.

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
then you'll need to modify `legacy/lib/CMakeLists.txt` appropriately.

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
./build/legacy/tests/LegacyTests
```
To run the actual tool:
```
./build/legacy/tools/driver/porytiles-legacy
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
`export_layers`](https://github.com/PKGaspi/Asepritescripts/blob/main/scripts/gaspi/export_layers.lua) script, which
allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer
in a separate PNG file.
