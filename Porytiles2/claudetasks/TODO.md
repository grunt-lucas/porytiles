# Test Creation for ShapeTile and CanonicalShapeTile

## Goal
Create comprehensive test files for `shape_tile.hpp` and `canonical_shape_tile.hpp` following the existing test patterns used for `pixel_tile_test.cpp` and `canonical_pixel_tile_test.cpp`.

## Plan

### Phase 1: Create ShapeTile Tests
- [ ] Create `shape_tile_test.cpp` in `Porytiles2/tests/unit/domain/model/tile/`
- [ ] Test default construction
- [ ] Test equality operator (operator==)
- [ ] Test spaceship operator (operator<=>)
- [ ] Test compare_shape_only() static method - this is critical and unique
- [ ] Test is_transparent() method
- [ ] Test flip() operations (no flip, h, v, both)
- [ ] Test flip symmetry (flipping twice returns to original)
- [ ] Test colors() accessor
- [ ] Test set() method for adding shape masks
- [ ] Test with IndexPixel type
- [ ] Test with other pixel types if applicable

### Phase 2: Create CanonicalShapeTile Tests
- [ ] Create `canonical_shape_tile_test.cpp` in `Porytiles2/tests/unit/domain/model/tile/`
- [ ] Test finding canonical representation (one of 4 flip combinations)
- [ ] Test choosing minimal representation based on SHAPE ONLY (not colors)
- [ ] Test handling symmetric tiles
- [ ] Test producing consistent results (same input → same canonical form)
- [ ] Test handling all flip variations (all flips of same tile → same canonical form)
- [ ] Test that flip flags correctly indicate transformation back to original
- [ ] Test equality operator (operator==)
- [ ] Test spaceship operator (operator<=>)
- [ ] Test with IndexPixel type
- [ ] Test critical difference: tiles with same shape but different colors canonicalize to same shape structure

### Phase 3: Build and Test
- [ ] Run format script: `./Scripts/format.sh 2> /dev/null`
- [ ] Build project: `cmake --build build -j7 > /tmp/build.log 2>&1`
- [ ] Run all tests: `./build/Porytiles2/tests/Porytiles2AllTests`
- [ ] Fix any issues that arise

## Key Design Considerations

### ShapeTile Unique Features
- `compare_shape_only()` - compares ONLY shape masks, ignoring pixel values
- Uses `std::map<ShapeMask, PixelType>` for storage
- Shape-based comparison vs full comparison

### CanonicalShapeTile Unique Features
- Uses `ShapeTile::compare_shape_only()` for canonical form selection
- This means tiles with same shape but different colors will have same canonical SHAPE
- But their pixel values will differ
- This is the key difference from CanonicalPixelTile

## Test Patterns to Follow
- Use `using namespace porytiles2;`
- Use GoogleTest (TEST macro)
- Create helper functions in anonymous namespace if needed
- Test both positive and negative cases
- Test edge cases (empty, symmetric, asymmetric tiles)
- Follow existing naming conventions: `ShapeTileTests` and `CanonicalShapeTileTests`

## Review Section
(To be filled in after implementation)
