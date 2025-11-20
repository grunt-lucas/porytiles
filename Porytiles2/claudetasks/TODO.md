# Refactor MetatileDecompiler extrinsic_transparency

## Goal
Refactor `MetatileDecompiler` to store `extrinsic_transparency` as a private field instead of a method parameter, and implement the transparency normalization TODO.

## Plan

### Task 1: Update MetatileDecompiler header
- [ ] Add `Rgba32 extrinsic_transparency_` as a private field
- [ ] Update constructor to accept `extrinsic_transparency` parameter
- [ ] Remove `extrinsic_transparency` parameter from `decompile_metatiles()` method signature

### Task 2: Update MetatileDecompiler implementation
- [ ] Update constructor implementation (if needed in cpp file)
- [ ] Update `decompile_metatiles()` to use `extrinsic_transparency_` field instead of parameter
- [ ] Update calls to `convert_tile()` to use the field
- [ ] Implement the TODO comment in `convert_tile()` to normalize transparency

### Task 3: Find and update all call sites
- [ ] Search for all places that construct `MetatileDecompiler`
- [ ] Update constructor calls to pass `extrinsic_transparency`
- [ ] Search for all places that call `decompile_metatiles()`
- [ ] Remove `extrinsic_transparency` argument from those calls

### Task 4: Verify changes
- [ ] Run format script
- [ ] Build the project
- [ ] Run all tests

## Notes
- The transparency normalization should compare each pixel's color against `extrinsic_transparency_` and handle it appropriately
- Keep changes minimal and focused
- Ensure all tests pass after changes

## Review

### Changes Summary
Successfully refactored `MetatileDecompiler` to store `extrinsic_transparency` as a private field instead of a method parameter.

### Files Modified
1. **metatile_decompiler.hpp** (Porytiles2/include/porytiles2/domain/services/)
   - Added `Rgba32 extrinsic_transparency_` as a private member field
   - Updated constructor to accept `extrinsic_transparency` parameter
   - Removed `extrinsic_transparency` from `decompile_metatiles()` method signature

2. **metatile_decompiler.cpp** (Porytiles2/lib/domain/services/)
   - Updated `decompile_metatiles()` implementation to use `extrinsic_transparency_` field
   - Updated all three calls to `convert_tile()` to pass `extrinsic_transparency_` instead of parameter
   - **Implemented TODO**: Added transparency normalization logic in `convert_tile()` function
     - Now checks if each palette color matches the extrinsic transparency
     - Normalizes transparent pixels to use the consistent extrinsic transparency value

3. **primary_tileset_compiler.cpp** (Porytiles2/lib/domain/services/)
   - Moved `MetatileDecompiler` construction to after `extrinsic_transparency` config is loaded
   - Updated constructor call to pass `extrinsic_transparency` parameter

### Implementation Details
The transparency normalization ensures that palette slot 0 (the conventional transparent color index) always maps to the user-configured extrinsic transparency value. This means any pixel with color index 0 will consistently use the extrinsic transparency, regardless of what color might be stored in the palette's slot 0.

### Verification
- ✅ Code formatting passed (clang-format)
- ✅ Build successful (cmake --build)
- ✅ All tests passed (Porytiles2AllTests)

### Impact
- No breaking changes to external API
- Single call site in `primary_tileset_compiler.cpp` updated successfully
- Cleaner separation of concerns: transparency configuration is now part of the decompiler's state rather than passed on every call
