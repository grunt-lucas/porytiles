# Todo List

## Basic Algorithm Outline
+ If master PNG width/height is not divisible by 8, fail fast
+ Check each tile, if any has more than 16 colors, fail fast
+ Count all unique colors in the master, if greater than `(numPalettes * 15) + 1` (15 since first color of each pal
  is reserved for transparency, add one for the transparency color), fail fast
+ Iterate left-to-right over each 8x8 pixel tile, skipping the tile if it is part of a structure we have already processed
    + Compute the set of colors present in the tile
    + Check color set against each palette, if it is a subset of any palette, assign that palette to this tile and move on
    + If the color set is not a subset, we must either add more colors to the closest-match palette (if possible), or
      create a brand new palette for this tile (if possible). If neither are possible, fail fast.
+ If we encounter a "stuctured" tile (given by checking the current pos in the structure map), then we do a sub-iteration
  and process only other tiles that are part of this structure. Perform the same pal calculations as above. Once we get
  to the end of the image and there are no more tiles in this structure, continue iterating where we left off.
+ Now that all tiles in the master have been assigned a palette, iterate through the working tile vector in order, assigning
  the correct index value to each pixel and placing it in the final tile vector
    + Keep a set of used tiles, for each tile before inserting, check under all GBA transforms if it exists in the set,
      and only place it in the final vector if not present
+ Save the final tile vector as an indexed PNG