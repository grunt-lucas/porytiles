# Task: Refactor Tile Conversion Functions

## Overview
Refactor tile conversion functions from `shape_tile.hpp` into a new `tile_converters.hpp` header, and implement reverse conversion operations.

## Todo List

- [ ] Create `Porytiles2/include/porytiles2/domain/models/tile_converters.hpp`
  - Move `from_pixel_tile` functions from `shape_tile.hpp` to the new header
  - Implement `from_shape_tile` functions for reverse conversion (ShapeTile<ColorIndex> → PixelTile<InputPixelType>)
  - Support both intrinsic and extrinsic transparency overloads

- [ ] Update `shape_tile.hpp`
  - Remove `from_pixel_tile` functions
  - Remove `from_pixel_tile_impl` helper
  - Keep only core ShapeTile functionality

- [ ] Create `Porytiles2/tests/unit/domain/models/tile_converters_test.cpp`
  - Move `from_pixel_tile` tests from `shape_tile_test.cpp`
  - Implement tests for `from_shape_tile` functions
  - Test both intrinsic and extrinsic transparency paths
  - Test round-trip conversions (PixelTile → ShapeTile → PixelTile)

- [ ] Update `shape_tile_test.cpp`
  - Remove moved `from_pixel_tile` tests
  - Keep only ShapeTile-specific functionality tests

- [ ] Run tests and format
  - Format code with `./Scripts/format.sh`
  - Build project
  - Run all tests to ensure nothing broke

## Design Notes

### from_shape_tile signature
```C++
template <SupportsTransparency InputPixelType>
[[nodiscard]] PixelTile<InputPixelType> from_shape_tile(
    const ShapeTile<ColorIndex> &shape_tile,
    const ColorIndexMap<InputPixelType> &color_index_map)
```

### from_shape_tile algorithm
1. Create a default PixelTile<InputPixelType> (all pixels transparent)
2. Iterate through each (ShapeMask, ColorIndex) pair in the ShapeTile
3. Look up the actual color from ColorIndexMap using the ColorIndex
4. For each bit set in the ShapeMask, set the corresponding pixel in the PixelTile to that color
5. Return the completed PixelTile

### Key considerations
- The `from_shape_tile` function doesn't need transparency overloads since it's using ColorIndex which doesn't support transparency
- The function will panic if a ColorIndex in the ShapeTile is not found in the ColorIndexMap
- If masks overlap, panic. That means the programmer did something wrong when constructing the ShapeTile. Also, add an invariant note to ShapeTile explaining that masks must not overlap.

## Review Section

### Completed Changes

1. **Created `tile_converters.hpp`** (Porytiles2/include/porytiles2/domain/models/tile_converters.hpp:1)
   - Moved `from_pixel_tile` functions from `shape_tile.hpp` to the new header
   - Both intrinsic and extrinsic transparency overloads implemented
   - Implemented new `from_shape_tile` function for reverse conversion (ShapeTile<ColorIndex> → PixelTile<InputPixelType>)
   - Added overlap detection in `from_shape_tile` that panics if masks overlap

2. **Updated `shape_tile.hpp`** (Porytiles2/include/porytiles2/domain/models/shape_tile.hpp:1)
   - Removed `from_pixel_tile` static methods (moved to tile_converters.hpp)
   - Removed `from_pixel_tile_impl` helper method
   - Added invariant documentation about non-overlapping masks
   - Cleaned up unused includes

3. **Added `ShapeMask::get` method** (Porytiles2/include/porytiles2/domain/models/shape_mask.hpp:98)
   - New method to check if a bit is set at a given row/col position
   - Implementation in shape_mask.cpp:37
   - Used in `from_shape_tile` for cleaner bit checking

4. **Created `tile_converters_test.cpp`** (Porytiles2/tests/unit/domain/models/tile_converters_test.cpp:1)
   - Moved all `from_pixel_tile` tests from `shape_tile_test.cpp`
   - Added comprehensive tests for `from_shape_tile`:
     - Simple conversion
     - Multiple colors
     - Empty ShapeTile produces transparent tile
     - Panic when ColorIndex not found
     - Panic on overlapping masks
   - Added round-trip conversion tests (PixelTile → ShapeTile → PixelTile)

5. **Updated `shape_tile_test.cpp`** (Porytiles2/tests/unit/domain/models/shape_tile_test.cpp:1)
   - Removed all `from_pixel_tile` tests (moved to tile_converters_test.cpp)
   - Kept only core ShapeTile functionality tests
   - Cleaned up unused includes

### Test Results
- Build: ✓ Success
- All tests: ✓ Passing (exit code 0)
- Code formatted with clang-format

### Key Design Decisions
- `from_shape_tile` doesn't need transparency overloads since ColorIndex doesn't support transparency
- Overlap detection in `from_shape_tile` enforces the ShapeTile invariant that masks must not overlap
- Added `ShapeMask::get` method for cleaner API rather than exposing bit manipulation details
- Factored out duplicate `from_pixel_tile` logic into `details::from_pixel_tile_impl` helper
  - Uses `details` namespace to signal this is an implementation detail not for public use
  - Both public overloads delegate to this helper with appropriate transparency predicates
  - Follows standard C++ practice for internal implementation helpers
