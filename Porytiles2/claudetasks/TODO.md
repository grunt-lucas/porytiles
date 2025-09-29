# Task: Refactor BasicError to FormattableError

## Overview
Rename the BasicError class to FormattableError throughout the codebase.

## Understanding
BasicError is a general-purpose error implementation with formatted message support. The name "FormattableError" better emphasizes its parameter formatting capabilities. The class is defined in error.hpp and used extensively throughout the codebase.

## Todo Items

- [ ] Update error.hpp to rename BasicError to FormattableError
- [ ] Update all 18 other files that use BasicError:
  - [ ] text_formatter.hpp
  - [ ] chainable_result.hpp
  - [ ] rgba_image_tileizer.hpp
  - [ ] rgba_layer_image_metatileizer.hpp
  - [ ] verify_primary_tileset.cpp
  - [ ] tileset_repo.cpp
  - [ ] compile_primary_tileset.cpp
  - [ ] png_rgba_image_saver.cpp
  - [ ] project_tileset_artifact_writer.cpp
  - [ ] project_tileset_artifact_reader.cpp
  - [ ] png_indexed_image_saver.cpp
  - [ ] jasc_pal_saver.cpp
  - [ ] pipeline.cpp
  - [ ] primary_tileset_compiler.cpp
  - [ ] rgba_layer_image_metatileizer.cpp
  - [ ] rgba_tile_normalizer.cpp
  - [ ] rgba_image_tileizer.cpp
- [ ] Run format script
- [ ] Build project
- [ ] Run unit tests
- [ ] Run integration tests

## Review

Successfully refactored `BasicError` to `FormattableError` throughout the entire codebase.

### Files Modified

1. **error.hpp** - Renamed the class from `BasicError` to `FormattableError`, including:
   - Class name and all constructors
   - All documentation comments
   - Example code in documentation
   - The clone() method implementation

2. **text_formatter.hpp** - Updated documentation references from `BasicError` to `FormattableError`

3. **chainable_result.hpp** - Updated the default template parameter from `BasicError` to `FormattableError`

4. **rgba_image_tileizer.hpp** - Updated documentation return type from `BasicError` to `FormattableError`

5. **rgba_layer_image_metatileizer.hpp** - Updated documentation return types (2 occurrences)

6. **All .cpp files** (13 files) - Replaced all occurrences of `BasicError` with `FormattableError`:
   - verify_primary_tileset.cpp
   - tileset_repo.cpp
   - compile_primary_tileset.cpp
   - png_rgba_image_saver.cpp
   - project_tileset_artifact_writer.cpp
   - project_tileset_artifact_reader.cpp
   - png_indexed_image_saver.cpp
   - jasc_pal_saver.cpp
   - pipeline.cpp
   - primary_tileset_compiler.cpp
   - rgba_layer_image_metatileizer.cpp
   - rgba_tile_normalizer.cpp
   - rgba_image_tileizer.cpp

### Testing Results

- **Formatting**: ✓ Passed (./Scripts/format.sh)
- **Build**: ✓ Passed (cmake --build build)
- **Unit Tests**: ✓ All 116 tests passed
- **Integration Tests**: ✓ All 43 tests passed

No regressions introduced. All code compiles cleanly and all tests pass.
