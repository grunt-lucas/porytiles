# Service vs Free Function Architecture

## Overview
This document provides a principled framework for deciding whether code should be organized as:
- Services (in `domain/services/`)
- Free template functions (like `tile_converters.hpp`)
- Domain class methods

## Decision Framework

### Free Functions (like `tile_converters.hpp`)

**Use when:**
- **Pure transformations** between types with no dependencies
- **Template-based** generic operations
- **Type conversions** that would create circular dependencies if in domain classes
- Function signature is clean (≤2 parameters beyond the data being transformed)

**Examples:**
```c++
// ✅ Perfect free function
auto shape_tile = from_pixel_tile(pixel_tile, color_index_map);

// ✅ Perfect free function
auto pixel_tile = from_shape_tile(shape_tile, color_index_map);

// ✅ Perfect free function
auto shape_tile_colors = shape_tile_to_pixel_colors(shape_tile, color_index_map);
```

### Services (in `domain/services/`)

**Use when you need 2 or more of:**
1. **Dependencies**: TextFormatter, UserDiagnostics, configuration objects
2. **Multiple steps**: Orchestrating several transformations
3. **Side effects**: Logging, validation messages, I/O operations
4. **Stateful processing**: Accumulating results across multiple calls
5. **Reusable context**: Same dependencies used across multiple methods

**Examples:**
- `PrimaryTilesetCompiler` - multi-step process with diagnostics
- `ColorSetBuilder` - complex building with validation
- `TileValidator` - needs diagnostics to report errors

**Service Pattern:**
```c++
class TilesetCompiler {
  public:
    TilesetCompiler(TextFormatter &formatter, UserDiagnostics &diagnostics)
        : formatter_{formatter}, diagnostics_{diagnostics} {}

    Result compile(const Input &input);

  private:
    TextFormatter &formatter_;
    UserDiagnostics &diagnostics_;
};
```

### Domain Class Methods

**Use for:**
- **Intrinsic operations**: `size()`, `empty()`, `is_valid()` on the object itself
- **Internal transformations**: operations that don't change the type
- **Accessors/mutators**: simple getters/setters

**Avoid:**
```c++
// ❌ Creates tight coupling
class PixelTile {
    ShapeTile to_shape_tile() const;  // PixelTile now depends on ShapeTile
};

// ❌ Creates tight coupling
class ShapeTile {
    static ShapeTile from_pixel_tile(const PixelTile &);  // ShapeTile now depends on PixelTile
};
```

## The "Clean Signature" Test

A key indicator for when to use a Service vs Free Function:

```c++
// Clean signature - perfect for free function
auto shape_tile = from_pixel_tile(pixel_tile, color_index_map);

// Messy signature - should be a Service
auto result = compile_tileset(input, text_formatter, diagnostics,
                               config, validator, builder, ...);
```

**Rule of thumb:** When a function would need 3+ dependency parameters, it belongs in a Service where those dependencies are member variables.

## Avoiding Circular Dependencies

Free functions are the cleanest solution for type conversions:

```c++
// ❌ Circular dependency problem
class ClassA {
    ClassB to_b() const;  // A depends on B
};

class ClassB {
    ClassA to_a() const;  // B depends on A
};

// ✅ No circular dependency
ClassB convert_a_to_b(const ClassA &a);
ClassA convert_b_to_a(const ClassB &b);
```

## Summary

### Free Functions are Best For:
- Pure data transformations
- Type conversions (avoiding circular dependencies)
- Template-based generic operations
- Operations with clean signatures (few parameters)

### Services are Best For:
- Complex multi-step orchestration
- Operations requiring multiple dependencies
- Operations with side effects (logging, diagnostics, I/O)
- Stateful processing

### Domain Methods are Best For:
- Intrinsic operations on a single object
- Accessors and mutators
- Operations that don't change the type

## Real-World Example: domain/algorithms/tile_converters.hpp

The `domain/algorithms/tile_converters.hpp` file exemplifies the correct pattern:
- Pure transformations between tile types
- No external dependencies beyond the data being transformed
- Template-based for type flexibility
- Avoids coupling between PixelTile, ShapeTile, and ColorIndex types
- Clean call sites throughout the codebase

This approach keeps domain classes focused and prevents architectural tangles.
