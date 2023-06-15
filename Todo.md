# Todo List

+ [Print tiles directly to the CLI!](https://github.com/eddieantonio/imgcat)

+ Use the `{fmt}` C++ library for colors and better output?
+ Add a `--report` option that prints out various statistics
    + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
        + calculate this by taking the `Number Unique Colors / Number Slots In Use`
    + what else?
+ Set up more CI builds for Windows, Clang on Mac, etc
+ Implement structure control tiles properly
    + four corners
    + palette allocation passes should also follow structure rules, so users get consistent results
+ `--verbose` should have filter modes:
    + e.g. `--verbose=index` to only print messages relate to tile indexing, `--verbose=all` for all logs, etc
+ output path should be optional: if excluded then the program will run but won't write anything, can be useful for
  querying logs without actually writing anything out to disk

## Big Feature Ideas

+ metatile generation:
    + this could be done by taking three PNGs as input, one for each layer
    + enforce these input PNGs are exactly 128 pixels wide (the width of metatile selector in Porymap)
    + any row with a control tile is a control row: no regular tiles may appear on any layer in this row
    + buildPalette and indexTiles iteration now happens over 3 PNGs instead of one, but the logic is identical
    + once a `tiles.png` and pal files are generated, iterate over each "metatile" by looking at the 2x2 tile square
      overlapping between all three layers
    + since we have a map of masterTile => finalTilePos, we can compute the final tile index, determine the palette via
      color matching, and use the transform methods to figure out which flip is at play
    + then simply write the metatile based on all this info!
        + since we enforced the master PNGs to be 128 pix wide, the metatile picker in Porymap should exactly match the
          layout of the master PNGs
