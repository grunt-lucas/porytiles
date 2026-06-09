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
- [User documentation](https://grunt-lucas.github.io/porytiles-user-docs/)
- [Developer documentation](https://grunt-lucas.github.io/porytiles-dev-docs/)
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
*TODO: replace this stub with a Quick Start walkthrough, or link out to the [user docs](https://grunt-lucas.github.io/porytiles-user-docs/) once that page is written.*

For now, install instructions live in [Release Cadence](#release-cadence) below.

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

Alternatively, download platform-specific zip files directly from the
[releases page](https://github.com/grunt-lucas/porytiles/releases).

Either install path provides two binaries:
`porytiles` (the modern compiler) and `porytiles-legacy` (the preserved
pre-1.0.0 compiler).
[Homebrew](https://brew.sh) works on Linux, macOS, and WSL.
On Linux and WSL, follow the [Homebrew on Linux](https://docs.brew.sh/Homebrew-on-Linux) setup instructions.

## Building From Source
See the [developer documentation](https://grunt-lucas.github.io/porytiles-dev-docs/) for build instructions.

## Note For Aseprite Users
GitHub user [PKGaspi](https://github.com/PKGaspi) has created a collection
of [useful scripts here.](https://github.com/PKGaspi/AsepriteScripts) Of particular interest is this [
`export_layers`](https://github.com/PKGaspi/Asepritescripts/blob/main/scripts/gaspi/export_layers.lua) script, which
allows you to save each sprite layer to a different file. This may be useful, since Porytiles requires each tile layer
in a separate PNG file.
