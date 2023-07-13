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

