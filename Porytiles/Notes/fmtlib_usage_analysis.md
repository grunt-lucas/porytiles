# fmtlib Usage Analysis Outside Text Module

## Overview

This document catalogs all uses of the fmtlib dependency outside of the `Porytiles/include/porytiles/utilities/text` module. The goal is to identify locations that should potentially be refactored to use the TextFormatter functionality to maintain implementation agnosticism in domain and app code.

## Summary

- **Total files using fmtlib outside text module:** 35 files
- **Primary usage patterns:**
  1. Panic messages (xcut/panic)
  2. Error message formatting (FormattableError)
  3. Diagnostic messages
  4. String formatting in domain models
  5. Debug/test output

## Detailed Analysis by Layer

### 1. Cross-cutting Concerns (xcut)

#### **Porytiles/include/porytiles/utilities/panic/panic.hpp**
- **Usage:** `fmt::format()` for formatting panic messages
- **Lines:** 55, 73
- **Pattern:** `fmt::format("{}:{} panic: {}\n", s.loc_.file_name(), s.loc_.line(), s.msg_)`
- **Refactor Priority:** HIGH - Core infrastructure used everywhere

### 2. Domain Layer

#### Domain Models (Headers)

##### **Porytiles/include/porytiles/domain/model/tile.hpp**
- **Usage:** `fmt::format()` in panic messages for bounds checking
- **Lines:** 48, 56, 59, 67, 75, 78
- **Pattern:** `panic(fmt::format("index {} out of bounds", i))`
- **Refactor Priority:** MEDIUM - Used in error conditions

##### **Porytiles/include/porytiles/domain/model/normalized_tile.hpp**
- **Usage:** `fmt::format()` in multiple methods
- **Primary uses:** Panic messages, string representations
- **Refactor Priority:** MEDIUM

##### **Porytiles/include/porytiles/domain/model/image.hpp**
- **Usage:** `fmt::format()` for bounds checking and error messages
- **Refactor Priority:** MEDIUM

##### **Porytiles/include/porytiles/domain/model/metatile.hpp**
- **Usage:** `fmt::format()` for error messages
- **Refactor Priority:** MEDIUM

##### **Porytiles/include/porytiles/domain/config/domain_config.hpp**
- **Usage:** `fmt::format()` for configuration-related messages
- **Refactor Priority:** LOW

#### Domain Models (Implementation)

##### **Porytiles/lib/domain/model/porymap_tileset_component.cpp**
- **Usage:** `fmt::format()` for error messages
- **Refactor Priority:** MEDIUM

#### Domain Services

##### **Porytiles/lib/domain/services/rgba_image_tileizer.cpp**
- **Usage:** `fmt::format()` for FormattableError messages
- **Line:** 19-20
- **Pattern:** `FormattableError{fmt::format("Image dimensions must be...")}`
- **Refactor Priority:** HIGH - User-facing error messages

##### **Porytiles/lib/domain/services/rgba_tile_normalizer.cpp**
- **Usage:** Similar FormattableError pattern
- **Refactor Priority:** HIGH

##### **Porytiles/lib/domain/services/rgba_layer_image_metatileizer.cpp**
- **Usage:** FormattableError formatting
- **Refactor Priority:** HIGH

#### Domain Orchestration

##### **Porytiles/lib/domain/orchestration/pipeline.cpp**
- **Usage:** `fmt::format()` for error and status messages
- **Refactor Priority:** MEDIUM

##### **Porytiles/include/porytiles/domain/orchestration/operation.hpp**
- **Usage:** `fmt::format()` for operation messages
- **Refactor Priority:** MEDIUM

#### Domain Repositories

##### **Porytiles/lib/domain/repos/tileset_repo.cpp**
- **Usage:** `fmt::format()` for repository error messages
- **Refactor Priority:** MEDIUM

### 3. Infrastructure Layer

#### Infrastructure Services

##### **Porytiles/lib/infra/services/png_rgba_image_saver.cpp**
##### **Porytiles/lib/infra/services/png_indexed_image_saver.cpp**
##### **Porytiles/lib/infra/services/png_indexed_image_loader.cpp**
##### **Porytiles/lib/infra/services/jasc_pal_saver.cpp**
##### **Porytiles/lib/infra/services/jasc_pal_loader.cpp**
##### **Porytiles/lib/infra/services/project_artifact_checksum_provider.cpp**
- **Usage:** `fmt::format()` for error messages and file I/O errors
- **Refactor Priority:** MEDIUM - Infrastructure error reporting

#### Infrastructure Repositories

##### **Porytiles/lib/infra/repos/project_tileset_artifact_reader.cpp**
##### **Porytiles/lib/infra/repos/project_tileset_artifact_writer.cpp**
##### **Porytiles/lib/infra/repos/project_tileset_artifact_key_provider.cpp**
- **Usage:** `fmt::format()` for file path construction and error messages
- **Refactor Priority:** MEDIUM

#### Infrastructure Configuration

##### **Porytiles/lib/infra/config/lazy_layered_config.cpp**
- **Usage:** `fmt::format()` for configuration error messages
- **Refactor Priority:** LOW

#### Infrastructure Diagnostics

##### **Porytiles/include/porytiles/infra/diagnostics/diagnostics.hpp**
##### **Porytiles/include/porytiles/infra/diagnostics/diagnostic_engine.hpp**
##### **Porytiles/lib/infra/diagnostics/diagnostics.cpp**
##### **Porytiles/lib/infra/diagnostics/diagnostic_engine.cpp**
- **Usage:** Heavy use of `fmt::format()` and `fmt::terminal_color` for diagnostic formatting
- **Lines in diagnostics.cpp:** 20, 31, 40, 168-182, 264, 388, 471
- **Refactor Priority:** VERY HIGH - This is a primary consumer of formatting

### 4. Application Layer

##### **Porytiles/lib/app/use_cases/compile_primary_tileset.cpp**
##### **Porytiles/lib/app/use_cases/import_primary_tileset.cpp**
- **Usage:** `fmt::format()` for use case error messages
- **Refactor Priority:** HIGH - User-facing application logic

### 5. Utilities (Outside Text Module)

##### **Porytiles/include/porytiles/utilities/string_utils.hpp**
- **Usage:** `fmt::format()` in regex error handling
- **Line:** 32
- **Pattern:** `panic(fmt::format("regex error: {}", e.what()))`
- **Refactor Priority:** LOW - Utility function

### 6. Tools/Driver

##### **Porytiles/tools/driver/main.cpp**
##### **Porytiles/tools/driver/validators.hpp**
- **Usage:** `fmt::format()` for CLI output and validation messages
- **Refactor Priority:** HIGH - User-facing interface

### 7. Tests

##### **Porytiles/tests/unit/domain/model/tile_test.cpp**
##### **Porytiles/tests/unit/domain/model/rgba_tile_test.cpp**
##### **Porytiles/tests/unit/domain/model/rgba32_test.cpp**
- **Usage:** `fmt::format()` for test assertions and debug output
- **Refactor Priority:** LOW - Test code doesn't need abstraction

## Refactoring Recommendations

### High Priority (Should Refactor)
1. **utilities/panic/panic.hpp** - Core infrastructure affecting entire codebase
2. **All FormattableError uses** in domain services - User-facing error messages
3. **Infrastructure diagnostics** - Primary formatting consumer
4. **Driver/CLI code** - User interface
5. **Application use cases** - User-facing logic

### Medium Priority (Consider Refactoring)
1. **Domain model panic messages** - Error conditions in core models
2. **Infrastructure services** - File I/O error reporting
3. **Domain orchestration** - Pipeline and operation messages

### Low Priority (May Keep As-Is)
1. **Test code** - No need for abstraction
2. **Configuration code** - Internal error handling
3. **String utilities** - Simple utility functions

## Implementation Approach

For refactoring, the following patterns should be used:

1. **For panic messages**: Create a panic formatter utility using TextFormatter
2. **For FormattableError**: Already supports FormatParam, just needs to use TextFormatter internally
3. **For diagnostic messages**: Refactor to use TextFormatter's style() method instead of fmt::terminal_color
4. **For general formatting**: Use TextFormatter::format() with FormatParam

## Notes

- The text module (`Porytiles/lib/utilities/text/text_formatter.cpp`) itself uses fmtlib internally, which is appropriate as it's the abstraction layer
- Some build files (CMakeLists.txt) and documentation (Notes/*.md) also reference fmtlib but are not runtime code