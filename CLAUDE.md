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
# Configure debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Configure debug build with coverage
cmake -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fcoverage-mapping -fprofile-instr-generate"

# Configure release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release

# Build project
cmake --build build -j7
```
Alternatively, if there is a build directory called `clion-build-debug`, use that instead of `build`.

## Testing
- Doctests for legacy version at `./build/Porytiles1/tests/Porytiles1Tests`
- GoogleTest unit tests at `./build/Porytiles2/tests/Porytiles2UnitTests`
- GoogleTest integration tests at `./build/Porytiles2/tests/Porytiles2IntegrationTests`
- GoogleTest all test runner at `./build/Porytiles2/tests/Porytiles2AllTests

Prefer to simply run all tests using the all test runner.

Run all tests:
```bash
./build/Porytiles2/tests/Porytiles2AllTests # this runs both Porytiles2UnitTests and Porytiles2IntegrationTests
```

## Code Quality Tools
Located in `Scripts/` directory:

### Formatting
```bash
# Format all source files
# SEND stderr TO /dev/null SO YOU DON'T POLLUTE YOUR CONTEXT WITH CRAP
./Scripts/format.sh 2> /dev/null
```
Uses `clang-format` with project-specific style configuration.

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

## 7 Claude rules
1. First think through the problem, read the codebase for relevant files, and write a plan to `Porytiles2/claudetasks/TODO.md`.
2. The plan should have a list of todo items that you can check off as you complete them.
3. Before you begin working, check in with me and I will verify the plan.
4. Then, begin working on the todo items, marking them as complete as you go.
5. Every step of the way, give me a high-level explanation of what changes you made.
6. Make every task and code change you do as simple as possible. We want to avoid making any massive or complex changes. Every change should impact as little code as possible. Everything is about simplicity. Run the format script, unit, and integration tests after you make a code change.
7. Finally, add a review section to the `Porytiles2/claudetasks/TODO.md` file with a summary of the changes you made and any other relevant information.

## Development Workflow Tools
1. Format code: `./Scripts/format.sh`
2. Build: `cmake --build build -j7`
3. Unit Tests: `./build/Porytiles2/tests/Porytiles2UnitTests`
4. Integration Tests: `./build/Porytiles2/tests/Porytiles2IntegrationTests`

## C++ Code Style
Use the following example snippet as a guide for code style.
@./STYLE.md

## **CRITICAL RULES - DO NOT VIOLATE**
- **ALWAYS use the code style outlined in the C++ Code Style section above**
- **Ignore contents of `Porytiles1/` directory** unless I explicitly tell you to work with those files
- **NEVER create mock data or simplified components** unless explicitly told to do so
- **NEVER replace existing complex components with simplified versions** - always fix the actual problem
- **ALWAYS work with the existing codebase** - do not create new simplified alternatives
- **ALWAYS find and fix the root cause** of issues instead of creating workarounds
- When debugging issues, **focus on fixing the existing implementation,** not replacing it
- When something doesn't work, debug and fix it - **don't start over with a simple version**
- Use braced initialization **where possible**, but make sure it won't confuse the compiler (e.g. when ambiguous constructors exist)
- **Never** include header files using relative paths
- Follow const correctness principles
- Always use namespace `porytiles2`, don't create child namespaces
- When creating private helper functions, **PREFER TO PLACE THEM IN AN ANONYMOUS NAMESPACE IN THE CPP FILE** instead of the `private:` section of the header file
- Both GCC and Clang are supported compilers, so any proposed code **should not be compiler-specific**
- WHEN RUNNING THE CMAKE BUILD COMMAND, SEND OUTPUT TO A TEMPORARY FILE SO YOU DON'T POLLUTE YOUR CONTEXT. You can then check if the build succeeded by looking at the exit code. If non-zero, inspect the file and see what went wrong.

