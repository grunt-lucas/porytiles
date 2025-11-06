# Task: Create Palette Matching Algorithm

## Goal
Create a new function header in the algorithms folder to implement an algorithm that matches a PixelTile<Rgba32> against a Palette<Rgba32>.

## Requirements
The function should return a custom container type that:
1. Flags if the palette can "cover" the tile (accounting correctly for transparency)
2. If the palette doesn't cover:
   - A set of colors the palette is missing
   - A set of colors the palette did in fact cover

## Todo Items

- [ ] 1. Define the result container type (struct/class) in a new header file `palette_matchers.hpp`
- [ ] 2. Implement the helper function in the `details` namespace that takes a transparency predicate
- [ ] 3. Implement the public function with intrinsic transparency support
- [ ] 4. Implement the public function with extrinsic transparency support
- [ ] 5. Format the code using `./Scripts/format.sh`
- [ ] 6. Build the project to check for compilation errors
- [ ] 7. Write unit tests for the new algorithm (if requested)

## Design Notes

### Result Type Structure
```C++
struct PaletteMatchResult {
    bool is_covered;
    std::set<Rgba32> missing_colors;
    std::set<Rgba32> covered_colors;
};
```

### Function Signature
```C++
// Intrinsic transparency version
[[nodiscard]] PaletteMatchResult
match_tile_to_palette(const PixelTile<Rgba32> &tile, const Palette<Rgba32> &palette);

// Extrinsic transparency version
[[nodiscard]] PaletteMatchResult
match_tile_to_palette(const PixelTile<Rgba32> &tile, const Palette<Rgba32> &palette, const Rgba32 &extrinsic);
```

### Algorithm Flow
1. (Extrinsic version only) Check if tile contains any extrinsically transparent pixels
   - If yes, verify that palette.colors().at(0) matches the extrinsic transparency color
   - If no match, panic: "Tile contains extrinsic transparency that does not match palette slot 0"
2. Extract unique non-transparent colors from the tile using `tile.unique_nontransparent_colors()`
3. For each color in the tile, check if it exists in the palette's colors
4. Categorize colors as either "covered" (found in palette) or "missing" (not in palette)
5. Set `is_covered = true` if missing_colors is empty
6. Return result with appropriate flag and sets

## Review
(To be filled in after completion)
