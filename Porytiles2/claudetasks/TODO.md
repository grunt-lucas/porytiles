# CreatePrimaryTileset TODO Implementation Plan

## Summary
The `CreatePrimaryTileset::create()` method needs to update three C header files (graphics.h, headers.h, and metatiles.h) after creating a new primary tileset. The TODO comment indicates we should use HeaderFileParser, CSourceGenerator, and CSourceFileModifier services.

## Current Issues
1. Three required services (HeaderFileParser, CSourceGenerator, CSourceFileModifier) are not included as dependencies

## Implementation Plan

### 1. Fix Constructor Dependencies
- Add `HeaderFileParser`, `CSourceGenerator`, and `CSourceFileModifier` as new dependencies
- Update the initializer list to properly initialize all members

### 2. Update Header File
Add the following member variables to the private section:
```cpp
std::unique_ptr<HeaderFileParser> header_parser_;
std::unique_ptr<CSourceGenerator> source_generator_;
std::unique_ptr<CSourceFileModifier> file_modifier_;
```

### 3. Implement TODO Logic
After saving the tileset (line 41), add code to:
1. Use `CSourceFileModifier::append_tileset_declarations()` to update all three header files
2. This method internally calls:
   - `append_to_graphics_header()` - adds palette and tile declarations
   - `append_to_headers_header()` - adds tileset struct definition
   - `append_to_metatiles_header()` - adds metatile/attribute declarations

### 4. Error Handling
The `append_tileset_declarations()` method returns a `Result<void>`, so we need to:
- Check if the operation succeeded
- Return any errors that occur during header file modification

## Code Changes Required
1. `Porytiles2/include/porytiles2/app/create_primary_tileset.hpp` - Update class declaration
2. `Porytiles2/lib/app/create_primary_tileset.cpp` - Update constructor and implement TODO

## Testing
After implementation:
1. Run format script: `./Scripts/format.sh 2> /dev/null`
2. Build: `cmake --build build -j7`
3. Run unit tests: `./build/Porytiles2/tests/Porytiles2UnitTests`
4. Run integration tests: `./build/Porytiles2/tests/Porytiles2IntegrationTests`