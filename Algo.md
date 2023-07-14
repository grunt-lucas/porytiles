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

bool assign(AssignState state) {
  if (state.unassigned.isEmpty) {
    // no tiles left to assign, found a solution!
    // here we can return the hardwarePalettes ColorSet vector from this stack frame's state struct, it is the solution

    // One way to return the solution here more easily would be to have assign take a `vector<ColorSet>& soln' that we
    // update here with state.hardwarePalettes. Since the only time we hit this branch is when a solution is found, at
    // the callsite we know that vector will contain a valid solution
    return true;
  }

  // we will try to assign the last element to one of the 6 hw palettes, last because it is a vector so
  // easier to add/remove from the end
  ColorSet toAssign = state.unassigned.last;

  // For this next step, we want to sort the hw palettes before we try iterating.
  // Sort them by the size of their intersection with the toAssign ColorSet. Effectively, this means that we will
  // always first try branching into an assignment that re-uses hw palettes more effectively. We can also secondarily
  // sort by total palette size as a tie breaker. E.g. if two different palettes have an intersection size of 1 with
  // toAssign, then choose the palette with the fewest number of assignments.
  sort(state.hardwarePalettes, [&toAssign](const auto& pal1, const auto& pal2){
    int pal1IntersectSize = sizeOfIntersection(pal1, toAssign);
    int pal2IntersectSize = sizeOfIntersection(pal2, toAssign);

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
    // We need to do all of this without modifying the current callstack's state variable, so make copies
    vector<ColorSet> unassignedCopy = makeCopy(state.unassigned);
    unassignedCopy.removeLast();
    hardwarePalettesUpdatedCopy = makeCopy(state.hardwarePalettes);
    hardwarePalettesUpdatedCopy[i] |= toAssign; // no i variable in this loop, translate to regular for loop
    AssignState newState = {hardwarePalettesUpdatedCopy, unassignedCopy};
    if (assign(newState)) {
      return true;
    }
  }

  // no solution found
  return false;
}
```

