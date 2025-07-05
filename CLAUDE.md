# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview
Porytiles is a C++ overworld tileset compiler for Pokémon Generation III decompilation projects. It takes RGBA input assets and generates Porymap-ready binary assets (metatiles.bin, metatile_attributes.bin, tiles.png, palettes).

## Architecture
The project is organized into two main versions:
- **Porytiles1**: Legacy version with original functionality
- **Porytiles2**: Next-generation version with domain-driven design architecture inspired by clang

Both versions share a similar structure with `include/`, `lib/`, `tests/`, and `tools/` directories.

## Build System
Uses CMake with C++23 standard. The build system requires:
- CMake 3.20+
- `zlib` and `libpng` static libraries
- GoogleTest for unit testing

### Build Commands
```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build project
cd build && cmake --build . -j7

# Alternative: build from root
cmake --build build -j7
```

### Build Variants
Multiple build configurations are available:
- `cmake-build-debug/` - Debug build
- `cmake-build-release/` - Release build  
- `cmake-build-debug-coverage/` - Debug with coverage
- `cmake-build-debug-gcc/` - Debug with GCC
- `cmake-build-release-gcc/` - Release with GCC

## Testing
Two test suites exist:
- **Porytiles1**: Legacy doctests at `./build/Porytiles1/tests/Porytiles1Tests`
- **Porytiles2**: GoogleTest unit tests at `./build/Porytiles2/tests/Porytiles2UnitTests`

Run all tests:
```bash
./build/Porytiles1/tests/Porytiles1Tests
./build/Porytiles2/tests/Porytiles2UnitTests
```

## Code Quality Tools
Located in `Scripts/` directory:

### Formatting
```bash
./Scripts/format.sh              # Format all source files
./Scripts/format.sh <files>      # Format specific files
```
Uses `clang-format` with project-specific style configuration.

### Linting
```bash
./Scripts/tidy.sh <files>        # Run clang-tidy on specific files
```
Uses `clang-tidy` with `cert-*` checks enabled.

## Project Structure
- `Porytiles1/` - Legacy version codebase, ignore this code unless otherwise instructed
- `Porytiles2/` - Next-generation version with domain-driven design
- `Resources/` - Test assets and example files
- `Documentation/` - Doxygen configuration
- `Scripts/` - Build and quality scripts
- `build/` - CMake build artifacts

## Driver Programs
- `./build/Porytiles1/tools/driver/porytiles` - Legacy CLI tool
- `./build/Porytiles2/tools/driver/porytiles2` - Next-generation CLI tool

## Key Design Patterns
Porytiles2 implements:
- Domain-driven design with clear separation of concerns
- Library-based architecture inspired by clang
- Template-based utilities in `Porytiles2/templates/`

## Development Workflow
1. Make changes to source files
2. Format code: `./Scripts/format.sh`
3. Build: `cmake --build build -j7`
4. Test: Run both test suites

## Important Notes
- Scripts must be run from the main directory (checked via `.porytiles-marker-file`)
- Both GCC and Clang are supported compilers, so any proposed code should not be compiler-specific
- Porytiles2 unit tests use the GoogleTest library
- Ignore contents of `Porytiles1` unless I explicitly tell you to work with those files
- Never include header files using relative paths
- Follow const correctness principles

## Critical Rules - DO NOT VIOLATE

- **NEVER create mock data or simplified components** unless explicitly told to do so

- **NEVER replace existing complex components with simplified versions** - always fix the actual problem

- **ALWAYS work with the existing codebase** - do not create new simplified alternatives

- **ALWAYS find and fix the root cause** of issues instead of creating workarounds

- When debugging issues, focus on fixing the existing implementation, not replacing it

- When something doesn't work, debug and fix it - don't start over with a simple version

## C++ Code Style
- Always use braced initialization where possible

Use the following example snippet as a guide for code style.
```C++
// PascalCase for enum class names
enum class FooBar {
    // kPascalCase for constants, note the leading k
    kFooValue1,
    kFooValue2
};

// PascalCase for class names
class MyClass {
  public:
    MyClass() = default;
    
    // ctor initializer lists always use braced initialization where possible
    // simple ctors can be implemented in the header file
    MyClass(int my_val) : my_val_{my_val} {}
  
    // Method names are PascalCase, but parameter names are snake_case
    int ComputeSomething(int accum_value) const;
    
    // Do something complicated to update my_val_
    // This should be implemented in the cpp file
    void UpdateMyValUsingComplexProcess(int some_param);
  
    // Simple accessors/mutators also use snake_case, but omit the trailing underscore
    // Simple accessors/mutators can be implemented in the header file
    const std::string &cool_value() const {
        return cool_value_;
    }
    
    int my_val() const {
        return my_val_;
    }

    void my_val(int new_val) {
        my_val_ = new_val;
    }
  
  private:
    // Member variables use snake_case_ with trailing underscore
    std::string cool_value_;
    int my_val_;
};

// cpp file implementations
int MyClass::ComputeSomething(int accum_value) const {
    // local variable names are snake_case
    int my_local = 1;
    return my_local + my_val_ + accum_value;
}
```
