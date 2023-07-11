# 1.0.0 Notes

## Palette Assignment Algorithm
```C++
// This is pseudo-C++ to get the point across

struct AssignState {
  // one color set for each hardware palette, bits in color set will indicate which colors this HW will have
  // size of vector should be fixed to maxPalettes
  vector<ColorSet> hardwarePalettes;

  // All the ColorSets from the normalization process, there is one for each normalized tile
  vector<ColorSet> unassigned;
}

boolean assign(AssignState state) {
  if (state.unassigned.isEmpty) {
    // no tiles left to assign, found a solution!
    // here we can return the hardwarePalettes ColorSet vector from this stack frame's state struct, it is the solution

    // One way to return the solution here more easily would be to have assign take a `vector<ColorSet>& soln' that we
    // update here with state.hardwarePalettes. Since the only time we hit this branch is when a solution is found, at
    // the callsite we know that vector will contain a valid solution
    return true;
  }

  // we will try to assign the last element to one of the 6 hw palettes
  ColorSet toAssign = state.unassigned.last;

  // For this next step, we want to sort the hw palettes before we try iterating.
  // Sort them by the size of their intersection with the toAssign ColorSet. Effectively, this means that we will
  // always first try branching into an assignment that re-uses hw palettes more effectively. We can also secondarily
  // sort by total palette size as a tie breaker. E.g. if two different palettes have an intersection size of 1 with
  // toAssign, then choose the palette with the fewest number of assignments.
  sort(state.hardwarePalettes, [&toAssign](const auto& pal1, const auto& pal2){
    int pal1IntersectSize = (pal1 | toAssign).count();
    int pal2IntersectSize = (pal2 | toAssign).count();

    // Instead of just using palette count, maybe can we check for color distance here and try to choose the palette
    // that has the "closest" colors to our toAssign palette? That might be a good heuristic for attempting to keep
    // similar colors in the same palette. I.e. especially in cases where there are no palette intersections, it may
    // be better to first try placing the new colors into a palette with similar colors rather than into the smallest
    // palette
    if (pal1IntersectSize == pal2IntersectSize) {
      return pal1.count() < pal2.count();
    }

    return pal1IntersectSize > pal2IntersectSize;
  });

  // Once we have the sort, iterate over them in order and try each one, making a recursive call.
  // If palette is unassignable (too many colors already), skip it.
  // Recursive call assign with copy and updated state:
  for (auto& palette : state.hardwarePalettes) {
    // > 15 because we need to save a slot for transparency
    if ((palette | toAssign).count() > 15) {
      // Skip this palette, cannot assign
      // If we end up skipping all of them that means the palettes are all too full and we cannot assign this tile
      continue;
    }
    // We need to do all of this without modifying the current callstack's state variable
    // So this pseudocode is not exactly correct, but it's the gist
    vector<ColorSet> newUnassigned = state.unassigned;
    newUnassigned.removeLast();
    palette |= toAssign;
    AssignState newState = {state.hardwarePalettes, newUnassigned};
    if (assign(newState)) {
      return true;
    }
  }

  // no solution found
  return false;
}
```

# Basic Algorithm Outline

+ If master PNG width/height is not divisible by 8, fail fast

+ Check each tile, if any has more than 16 colors, fail fast

+ Count all unique colors in the master, if greater than `(numPalettes * 15) + 1` (15 since first color of each pal
  is reserved for transparency, add one for the transparency color), fail fast

+ First, we prefill some palettes based on the user's sibling control tiles, if present. Sibling control tiles allow
  users to implement the common shared-tile technique, where one tile is valid in multiple palettes. This is common in
  e.g. PokeCenter and PokeMart roof tiles, which have the same pattern but different colors.
    + iterate over each sibling tile, up to `numPalettes`
        + iterate over the tile pixels in order and push_back to the corresponding palettes for each unique color
        + if we see a duplicate color, fail fast
        + we are guaranteed not to exceed the unique color limit since at this point, all palettes are empty and we have
          already verified that no tile has more than 15 unique colors (not counting transparency)

+ The next step will be to construct the palettes (for now, we will ignore our structure file). We'll need to create
  an `RgbTiledPng` from the master PNG and a `vector` of `Palette` objects sized based on the numTiles parameter

+ Iterate over each `RgbTile` in our `RgbTiledPng`
    + If this is a primer-marker tile, skip it
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

+ If at any point in the palette alloc or tile index steps, we encounter a "structured" tile (given by checking the
  current pos in the structure map), then we do a sub-iteration and process only other tiles that are part of this
  structure. Perform the same pal calculations as above. Once we get to the end of the image and there are no more tiles
  in this structure, continue iterating where we left off
    + Still need to figure out the right way to handle structure tiles. There are three possibilities I have thought of
      thus far:
        + Structures are keyed by color. Any tile of the same color will be considered the same "structure". This is
          highly granular and flexible, but tbh it is kinda tedious to work with. Especially because it might start to
          be hard to find colors that are visually distinct
        + Structures are keyed on one pre-chosen color, and we count any touching tiles to be part of the same
          structure. This seems much easier to work with. The downside is users will have to make sure that different
          structures in their master PNG do not touch. That should be fairly easy though, since there is no hard limit
          to the size of the master PNG
        + Don't have structure tiles at all: instead, encourage users to put one structure per logical "row" of their
          master PNG. That is, the master PNG should be a very tall PNG where each coherent structure has only blank
          tiles to its right.
        + Structure tiles are one color, but require the user to mark the "four corners" of each structure. This will
          give well defined boundaries for each structure and allow us to do a prescan "syntax" check that makes sure
          everything is valid

+ Last step is to save the final `IndexedTile` vector and palettes to the output directory in the appropriate format.
    + We will need to make sure we make the indexed PNG have the right width, properly padding it out if the final row
      is not completely filled
    + If the final `IndexedTile` vector is too big, we fail with a message saying "max num tiles exceeded" or something
