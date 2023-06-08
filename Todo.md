# Todo List

## Basic Algorithm Outline

+ If master PNG width/height is not divisible by 8, fail fast

+ Check each tile, if any has more than 16 colors, fail fast

+ Count all unique colors in the master, if greater than `(numPalettes * 15) + 1` (15 since first color of each pal
  is reserved for transparency, add one for the transparency color), fail fast

+ The next step will be to construct the palettes (for now, we will ignore our structure file). We'll need to create
  an `RgbTiledPng` from the master PNG and a `vector` of `Palette` objects sized based on the numTiles parameter

+ Iterate over each `RgbTile` in our `RgbTiledPng`
    + Add each `RgbColor` in the tile to an `unordered_set`
    + Compute `diff = tileColorSet - paletteColorSet` for each palette, where `-` is set difference
        + If `size(diff) == 0`, that means `paletteColorSet` fully covers `tileColorSet`, and so we can break, we have
          found a palette that will work for this tile
        + If `size(diff) == size(tileColorSet)`, that means this tile doesn't share any colors with the current palette.
          If this is true for every palette, then we have a tile with only new colors. In this case, let's add the new
          colors to the palette with the fewest number of colors so far.
        + If `size(diff) > 0`, `tileColorSet` shares some, but not all, colors with this palette. Save off how many. Do
          this for each palette, we'll want to use the palette that is the closest match. If the closest match palette
          doesn't have room for the new colors, try the next closest match. If no palette has room, fail the whole
          program
    + `NOTE:` This algorithm is naive and has numerous problems:
    + This algo isn't perfect, in many cases it will probably get stuck and create sub-optimal palette layouts. One way
      to mitigate this is to "prime" it with a tile containing groups of colors you want in the same palette. So e.g.
      place a tile containing all the wood tone colors just before your log cabin building, that way any individual log
      cabin tile will end up indexing into the same final palette. This isn't strictly required but it will make
      building metatiles easier, since you won't have to search through every palette trying to figure out what the
      allocation algorithm decided to do. The downside is now you have to have some forethought about palette layout,
      however it is still easier than free-styling.

+ If we encounter a "structured" tile (given by checking the current pos in the structure map), then we do a
  sub-iteration and process only other tiles that are part of this structure. Perform the same pal calculations as
  above. Once we get to the end of the image and there are no more tiles in this structure, continue iterating where we
  left off
    + Still need to figure out the right way to handle structure tiles. There are two possibilities I have thought of
      thus far:
        + Structures are keyed by color. Any tile of the same color will be considered the same "struture". This is
          highly granular and flexible, but tbh it is kinda tedious to work with. Especially because it might start to
          be hard to find colors that are visually distinct
        + Structures are keyed on one pre-chosen color, and we count any touching tiles to be part of the same
          structure. This seems much easier to work with. The downside is users will have to make sure that different
          structures in their master PNG do not touch. That should be fairly easy though, since there is no hard limit
          to the size of the master PNG

+ At this point, our palettes should be correctly constructed. Now is the time to convert each tile from the master PNG
  into an index tile, and dedupe along the way. In fact, we could have probably done this simultaneously with allocating
  the palettes. So why didn't we? Because in the future I may want to improve the palette allocation algorithm by having
  it do multiple passes, or changing the palette orders mid-run. And I'd rather not have to refactor the tile allocation
  code out at that point. So we can just do a second pass to make the code more flexible even if it is technically less
  optimal.

+ Perform same iteration over `RgbTiledPng` as before, accounting for structure rules just as before.
    + Add each `RgbColor` in the tile to an `unordered_set` (if this is an all transparent tile, skip it, already added,
      always first tile of final tileset by default hardcode)
    + Compute `diff = tileColorSet - paletteColorSet` for each palette, where `-` is set difference
    + If `size(diff) == 0`, that means `paletteColorSet` fully covers `tileColorSet`, and so we can break, we have
      found a palette that will work for this tile. In fact, if our palette allocation algorithm worked, then this
      should always be the case for at least one palette. If not, we have a fatal error and should bail early.
    + When we find the palette for this tile, construct an indexed tile by comparing each `RgbColor` in the tile to
      those in the palette vector and choosing the matches. Finally, check each transform of this `IndexedTile` if it is
      already in the tileset (flip X, flip Y, flip X and Y). If it is, skip. Otherwise, add it!
    + When we exhaust the `RgbTiledPng` we're done!

+ Last step is to save the final `IndexedTile` vector and palettes to the output directory in the appropriate format.
    + We will need to make sure we make the indexed PNG have the right width, properly padding it out if the final row
      is not completely filled
    + If the final `IndexedTile` vector is too big, we fail with a message saying "max num tiles exceeded" or something

+ How to handle the very common shared-tile technique? I.e. where one tile is valid across multiple palettes, think the
  center/mart roof tiles in emerald
    + sibling_tiles.txt can hold comma separated lines for sibling tiles: e.g. `12,45,32`
    + the palette allocation algorithm will need to allocate colors for sibling tiles first
        + each sibling tile must use its own palette, but the color indices must match
        + we can ensure this by picking the least populated N palettes and push_front
            + iterate over the tile pixels in order and push_front to the corresponding palettes for each unique color
            + if we run out of colors, fail fast