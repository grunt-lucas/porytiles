# Refactor: Convert RgbaImageTileizer to Template Class ImageTileizer

## Overview
Convert `RgbaImageTileizer` to a template class `ImageTileizer<T>` that can work with any pixel type, not just `Rgba32`.

## Status: COMPLETED

All tasks completed successfully. All 275 tests passing.

## Review Summary

Successfully refactored `RgbaImageTileizer` to a generic template class `ImageTileizer<T>` that can tileize images with any pixel type.

### Files Modified

**Created:**
- `include/porytiles2/domain/services/image_tileizer.hpp` - New template class with inline implementation

**Updated:**
- `include/porytiles2/domain/services/rgba_layer_image_metatileizer.hpp` - Changed member from `RgbaImageTileizer` to `ImageTileizer<Rgba32>`
- `tests/unit/domain/services/rgba_image_tileizer_test.cpp` - Updated to use `ImageTileizer<Rgba32>`

**Deleted:**
- `include/porytiles2/domain/services/rgba_image_tileizer.hpp`
- `lib/domain/services/rgba_image_tileizer.cpp`

### Changes Made

1. **Created template class `ImageTileizer<T>`** that works with any pixel type
2. **Moved implementation to header file** (required for templates)
3. **Updated all usages** to instantiate with `ImageTileizer<Rgba32>`
4. **Updated documentation** to reflect generic nature of the class

### Test Results

- All 275 tests passing
- Build successful with no warnings or errors
- Code formatting applied successfully

### Benefits

- More flexible and reusable - can now tileize images with any pixel type (e.g., `IndexPixel`, `Rgba32`, etc.)
- Reduced code duplication by using templates
- Same performance characteristics (template instantiation is compile-time)
- All existing functionality preserved
