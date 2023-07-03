# Todo List

+ Print tiles directly to the CLI!
    + https://github.com/eddieantonio/imgcat

+ Use the `{fmt}` C++ library for colors and better output?
    + https://github.com/fmtlib/fmt

+ Continue to create more in-file unit tests that maximize coverage
    + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add a `--report` option that prints out various statistics
    + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
        + calculate this by taking the `Number Unique Colors / Number Slots In Use`
    + what else?

+ Set up more CI builds
    + Windows MSVC
    + Clang on MacOS

+ Remove code for structure control tiles
    + these overcomplicate things, won't really be necessary once metatile generation is in place

+ `--verbose` should have filter modes:
    + e.g. `--verbose=index` to only print messages relate to tile indexing, `--verbose=all` for all logs, etc

+ output path should be optional: if excluded then the program will run but won't write anything, can be useful for
  querying logs without actually writing anything out to disk

+ Remove `--8bpp-output` option and make it default once Porymap properly supports
    + add a `--out-palette=<index>` to output the PNG 4bpp with the given palette
    + add a `--out-palette-greyscale` to output with 4bpp greyscale

+ Change the palette writing code so it always writes 12 palettes (for easier importing and makefile integration)
    + `--num-pals-in-primary=<num>` will default to `6`, but will allow user to adjust
    + porytiles will default to generating a primary tileset, so the `max-palettes` option will basically be replaced
      by `num-pals-in-primary` for this case
    + `--secondary` may be supplied, which makes the old `max-palettes` equal to `12 - num-pals-in-primary`
        + this will also start writing the palettes at index `num-pals-in-primary`, and write blank palettes for the
          first N
    + these changes will be very helpful once we start working on metatile generation

+ How could we improve the palette allocation algorithm to remove the need for primer tiles?
    + right now, the algo is greedy and so can generate suboptimal palettes
    + we'd need some kind of backtracking mechanism to do this correctly
    + mrgriffin suggested the following improved greedy approach:
        + I think you could fix that particular problem with only a minor tweak: build the tile palette sets, but defer
          picking a hardware palette until all tiles have been processed. At this point you can ensure that any tile
          palette which is a subset of another tile palette should go to the same hardware palette. (Probably achievable
          by iterating your tile palettes in a particular order—say largest to smallest)
        + (The largest to smallest ordering should work because by definition a [strict] subset must contain fewer
          elements than its superset, so if there was a superset of this tile's colors, it will already be allocated)
    + See mgriffin's work in the `normalize` branch
    + Once we include these improvements, we can remove primer (and possibly also sibling) control tiles

+ Test

# Big Feature Ideas

+ metatile generation:
    + this could be done by taking three PNGs as input, one for each layer
    + enforce these input PNGs are exactly 128 pixels wide (the width of metatile selector in Porymap)
    + any row with a control tile is a control row: no regular tiles may appear on any layer in this row
        + there would be no need for structure control regions in metatile mode, since the final metatiles.bin layout
          will exactly mirror the layout of the layer PNGs (since we will enforce 128 pixel width)
    + buildPalette and indexTiles iteration now happens over 3 PNGs instead of one, but the logic is identical
    + once a `tiles.png` and pal files are generated, iterate over each "metatile" by looking at the 2x2 tile square
      overlapping between all three layers
    + since we have a map of masterTile => finalTilePos, we can compute the final tile index, determine the palette via
      color matching, and use the transform methods to figure out which flip is at play
    + then simply write the metatile based on all this info!
        + since we enforced the master PNGs to be 128 pix wide, the metatile picker in Porymap should exactly match the
          layout of the master PNGs
    + One important question: how do we handle the ability of secondary tilesets to borrow tiles from their paired
      primary set when creating metatiles?
        + one solution would be to have a `--primary / --secondary` option, when you specify `--secondary` you must also
          specify:
            + a `tiles.png` from the primary tileset
            + pal files from the primary tileset
            + the code would then construct a `Tileset` object that represents the primary tileset, and code logic must
              be changed
                + e.g. when looking at a tile on a given layer in the secondary tileset, we have to first check the
                  primary tileset to see if this tile exists under any of the primary palettes
                    + this won't handle cross-tileset siblings properly: e.g. suppose you want to have a secondary tile
                      that borrows the shape of one of the primary tiles but uses a secondary palette, this would be
                      pretty hard to support, but maybe that is OK since that is a very niche use-case
                    + users could still simulate this by simply accepting having a duplicate tile in the secondary
                      tiles.png that matches one of the secondary palettes
