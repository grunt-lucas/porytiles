# Porytiles

[![Actions Status](https://github.com/grunt-lucas/porytiles/workflows/Build%20Porytiles/badge.svg)](https://github.com/grunt-lucas/porytiles/actions)

Create an indexed tileset from a master RGB PNG tilesheet. For use with
the [`pokeruby`](https://github.com/pret/pokeruby), [`pokeemerald`](https://github.com/pret/pokeemerald), and
[`pokefirered`](https://github.com/pret/pokefirered) Pokémon Generation 3 decompilation
projects.

`porytiles` generates "stable" tilesets. That is:

1. For the same input `master.png`, `porytiles` will always generate the exact same `tiles.png` and palette files.
2. `porytiles` reads the input `master.png` left-to-right, top-to-bottom. If you make sure to add new tiles at the
   bottom of the master sheet, this will not disturb palettes and tiles allocated for a previous version of
   your `master.png`. So e.g. say you decide you want to add a new tree type to a tileset that you are already
   generating using `porytiles`, and that you are already using for metatiles in Porymap. If you make sure to add the
   new tree at the bottom of the `master.png`, it is guaranteed to not break your metatiles. You just need to make sure
   you aren't adding so many new colors that the palette allocation fails.

## Limitations - PLEASE READ FIRST

This is an early release of `porytiles`. I cannot guarantee there are no bugs. The palette allocation is also a
first-attempt algorithm, so sometimes it may behave in an unintuitive way. I am working on some additional features to
help with these issues.

Also: while `porytiles` does aim to guarantee stability of your final `tiles.png`, since this is an early release I
cannot guarantee I won't break that stability with future releases. So any tilesets you create may have broken metatiles
when you upgrade to future versions of porytiles. Upon official release, I will have more stringent stability guarantees
in place so that you never have to worry about `porytiles` updates breaking your metatiles.

## Who Is This For?

`porytiles` is not intended for tileset pros who know all the ins-and-outs of palette selection, PNG indexing, and tile
layout. If you are able to allocate palettes with ease and lay out your `tiles.png` with your preferred editor, then
`porytiles` is probably not for you.

However, if you are someone who loves [Porymap](https://github.com/huderlem/porymap) but is overwhelmed at the
prospect of creating a completely new tileset from scratch, `porytiles` can smooth things over! All you have to do is
draw (or copy-paste 😜) your art however you like onto an RGB tilesheet PNG. Then let `porytiles` figure out how to
construct your palettes and dedupe/transform your tiles.

## Getting Started

Porytiles requires that you have a C++17 compatible compiler. The latest Clang or GCC should work fine. Porytiles also
requires that you have `libpng` and `zlib` installed. If you are building `pokeemerald` or a similar repo, you probably
already have these. Once you are all set up, build the project with:

```shell
make
```

For basic usage information, try:

```shell
./porytiles --help
```

To build your first tileset with some sensible defaults, go ahead and run:

```shell
./porytiles /path/to/mytiles.png /path/to/output/folder
```

where `mytiles.png` is an RGB PNG tilesheet, and `folder` is the folder where you want the output. If `folder` does not
already exist, `porytiles` will create it for you.

For much more detailed usage information, please check the wiki!
