# Todo List

+ [Print tiles directly to the CLI!](https://github.com/eddieantonio/imgcat)

+ Use the `{fmt}` C++ library for colors and better output?
+ Add a `--report` option that prints out various statistics
    + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
        + calculate this by taking the `Number Unique Colors / Number Slots In Use`
    + what else?
+ Set up more CI builds for Windows, Clang on Mac, etc
+ MGriffin tip: `gbagfx` ignores the top 4 bits in 8bpp images. So, we can make the final tiles.png 8bpp and construct
  the PNG palette such that all the colors look correct to human eyes! Basically, use the top 4 pixels as a palette
  selector, and then the bottom four pixels will select into the palette as normal. We'll need to modify the `Tile`
  class so that tiles can store which palettes they are part of. Maybe a final tile that was generated from multiple
  siblings can be greyscale?
+ Implement sibling control tiles properly
    + sibling control tiles will be like primer control tiles, but users can line up multiple colors to make sure they
      share indexes
    + sibling control block must be the first thing in the file, there can only be one
+ Implement structure control tiles properly
    + four corners
    + palette allocation passes should also follow structure rules, so users get consistent results
+ `--verbose` should have filter modes:
    + e.g. `--verbose=index` to only print messages relate to tile indexing, `--verbose=all` for all logs, etc
+ print final tile index even if tile was already present: we can do this by changing the `tilesIndex`
  in `Tileset` class from `unorderd_set` to `unordered_map` where the value is the final tile index
+ output path should be optional: if excluded then the program will run but won't write anything, can be useful for
  querying logs without actually writing anything out to disk
