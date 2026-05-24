# Architecture

This document describes the high-level architecture of Porytiles.
If you want to familiarize yourself with the code base, you are just in the right place!

- [Architecture](#architecture)
  - [Bird's Eye View](#birds-eye-view)
    - [Layer Dependencies](#layer-dependencies)
    - [Architecture Pattern](#architecture-pattern)
  - [Codemap](#codemap)
    - [`domain/` - Business Logic](#domain---business-logic)
      - [`domain/algorithms/` - Pure Functions](#domainalgorithms---pure-functions)
        - [Why Free Functions?](#why-free-functions)
      - [`domain/config/` - Configuration Interfaces](#domainconfig---configuration-interfaces)
      - [`domain/models/` - Core Data Types](#domainmodels---core-data-types)
      - [`domain/repos/` - Persistence Abstractions](#domainrepos---persistence-abstractions)
      - [`domain/services/` - Orchestrated Operations](#domainservices---orchestrated-operations)
      - [`domain/packing/` - Palette Packing System](#domainpacking---palette-packing-system)
    - [`app/` - Use Cases](#app---use-cases)
      - [`app/config/` - Application Configuration](#appconfig---application-configuration)
      - [`app/use_cases/` - Workflows](#appuse_cases---workflows)
    - [`infra/` - I/O and External Systems](#infra---io-and-external-systems)
      - [`infra/config/` - Configuration System](#infraconfig---configuration-system)
      - [`infra/repos/` - Concrete Persistence](#infrarepos---concrete-persistence)
      - [`infra/services/` - I/O Implementations](#infraservices---io-implementations)
    - [`xcut/` - Cross-Cutting Concerns](#xcut---cross-cutting-concerns)
      - [`xcut/diagnostics/` - User Communication](#xcutdiagnostics---user-communication)
      - [`xcut/config/` - Configuration Utilities](#xcutconfig---configuration-utilities)
      - [`xcut/di/` - Dependency Injection](#xcutdi---dependency-injection)
    - [`utilities/` - Low-Level Helpers](#utilities---low-level-helpers)
      - [`utilities/functional/` - Functional Programming](#utilitiesfunctional---functional-programming)
      - [`utilities/panic/` - Termination](#utilitiespanic---termination)
      - [`utilities/result/` - Error Handling](#utilitiesresult---error-handling)
      - [`utilities/text/` - Text Formatting](#utilitiestext---text-formatting)
      - [Other Utilities](#other-utilities)
  - [Cross-Cutting Concerns](#cross-cutting-concerns)
    - [Configuration System](#configuration-system)
    - [Dependency Injection](#dependency-injection)
    - [Diagnostics Integration](#diagnostics-integration)
    - [Testing Strategy](#testing-strategy)
      - [Unit Tests (`tests/unit/`)](#unit-tests-testsunit)
      - [Integration Tests (`tests/integration/`)](#integration-tests-testsintegration)
  - [Design Principles](#design-principles)
    - [Runtime Safety](#runtime-safety)
    - [Type Safety](#type-safety)
    - [Separation of Concerns](#separation-of-concerns)
    - [Composability](#composability)
    - [Extensibility](#extensibility)
    - [Testability](#testability)
  - [Entry Points](#entry-points)
    - [Driver Program (`tools/driver/`)](#driver-program-toolsdriver)
    - [Extension Points](#extension-points)
  - [Bibliography and Inspiration](#bibliography-and-inspiration)


## Bird's Eye View

Porytiles is a C++ tileset compiler that transforms RGBA image assets into Porymap-ready binary assets for Pokémon Generation III decompilation projects.

The system is organized around a **layered architecture** inspired by domain-driven design.

```
    infra/        -- I/O and external system/library integration
      │
    app/          -- User-facing use cases and workflows
      │
    domain/       -- Pure business logic, no I/O dependencies
      │
    xcut/         -- Cross-cutting concerns (errors, diagnostics, config, di, etc)
      │
    utilities/    -- Generic helpers, zero dependencies
```

### Layer Dependencies

The dependency flow is strictly one-way:

- **infra** can depend on: everything (utilities, xcut, domain, app)
- **app** can depend on: utilities, xcut, domain only
- **domain** can depend on: utilities, xcut only
- **xcut** can depend on: utilities only
- **utilities** can depend on: nothing

This strict layering ensures:
- Domain logic remains testable and independent
- Clear separation of concerns
- Easy to swap infrastructure implementations
- Minimal coupling between components

### Architecture Pattern

The system uses **Domain-Driven Design** with clear separation:

1. **Domain Layer**: Business logic, algorithms, models (pure functions, no side effects)
2. **Application Layer**: Use cases that orchestrate domain operations
3. **Infrastructure Layer**: I/O, persistence, external systems
4. **Cross-Cutting**: Error handling, configuration, diagnostics

## Codemap

This section provides detailed descriptions of important directories and components.

### `domain/` - Business Logic

The domain layer is the heart of the system. It contains all business logic with zero infrastructure dependencies.

#### `domain/algorithms/` - Pure Functions

Domain algorithms are **pure transformations** implemented as free functions.
They typically provide complex conversion operations between the various domain models.
The algorithms are loosely organized into headers so that similar operations on similar types are grouped together.
The algorithms here have clean signatures - more complex operations that require multiple dependencies are implemented as services.

##### Why Free Functions?
- Keep domain types decoupled (e.g. two-way conversion between `PixelTile` ↔ `ShapeTile` without causing circular dependencies)
- Enable generic programming through templates
- Pure functions are trivial to test and reason about
- Clean call sites: `auto shape = from_pixel_tile(pixel, map);`

See `Porytiles/Notes/service_vs_free_function_architecture.md` for the complete decision framework.

#### `domain/config/` - Configuration Interfaces

In addition to the generated `DomainConfig` layer configuration interface,
additional domain-layer configuration helpers are defined here.

#### `domain/models/` - Core Data Types

The `models/` builds out the core domain language for Porytiles.
Fundamental building block value objects like Rgba32 and IndexPixel
provide the meat for template aggregates like PixelTile, Metatile, and Image.

#### `domain/repos/` - Persistence Abstractions

Repositories abstract away persistence details for domain aggregate roots using the **Repository Pattern**.

The centerpiece is `TilesetRepo`, which coordinates all persistence operations for the Tileset aggregate.
Rather than implementing storage directly, it delegates to specialized interfaces:

- `TilesetArtifactKeyProvider`: Discovers and generates artifact identities in the backing store
- `TilesetArtifactReader`: Loads artifacts from storage
- `TilesetArtifactWriter`: Saves artifacts to storage
- `ArtifactChecksumProvider`: Computes/verifies checksums for build artifacts

This separation enables different storage backends (filesystem, database, in-memory) without changing domain logic.
The `TilesetArtifact` value type represents individual artifacts (metatiles.bin, tiles.png, etc.) with optional metadata,
while `ArtifactKey` identifies an artifact's location in the backing store.

Example usage:
```c++
// Load MyTileset from disk
auto tileset_load_result = tileset_repo.load("MyTileset");
if (!tileset_load_result.has_value()) {
    // Handle error
}

auto tileset = std::move(tileset_load_result).value();
// Do some stuff with the tileset...

// Save it back to disk
tileset_repo.save(tileset);
```

#### `domain/services/` - Orchestrated Operations

Services encapsulate **complex domain operations** that require multiple dependencies or stateful coordination.
Unlike pure algorithms, services are classes injected via constructors following the Dependency Inversion Principle.

Examples include:

- `PrimaryTilesetCompiler`: Orchestrates the complete compilation pipeline from Porytiles format to Porymap format
- `DefunctPrimaryTilesetImporter`: (DEPRECATED) Imports Porymap tilesets into Porytiles format (reverse of compilation). A new import system is being developed.
- `ImageTileizer`: Converts layer images into tile data structures
- `LayerImageMetatileizer`: Transforms layer images into metatile assemblies
- `MetatileDecompiler`: Decompiles Porymap metatile data back into RGBA layer images
- `LayerModeConverter`: Converts `metatiles.bin` entry vectors between [dual and triple layer formats](https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles)
- `PaletteValidator`: Validates palettes for compilation, checking for transparency violations and slot mismatches
- `MetatileValidator`: Validates metatiles for compilation, checking alpha channels, color counts, and layer modes
- `BehaviorMapProvider`: Abstract interface for loading behavior constant name-to-value mappings from game headers

Services usually return `ChainableResult<T, E>` to propagate rich error chains through the domain.
Some services define virtual interfaces to enable polymorphic behavior and dependency injection.

When should logic be a service vs. a free function? Services require I/O, state, or orchestrate multiple dependencies.
Pure transformations stay as free functions in `domain/algorithms/`.

See `Porytiles/Notes/service_vs_free_function_architecture.md` for the complete decision framework.

#### `domain/packing/` - Palette Packing System

The packing subsystem solves the **"Pagination Problem"** - a variant of bin packing with overlapping items.
Given a set of tiles (each requiring specific colors) and a limited number of hardware palettes (each with limited slots),
find an optimal assignment of tiles to palettes that minimizes wasted space.

This is a key optimization problem for GBA tileset compilation where hardware constraints are strict.

##### `packing/algorithms/` - Packing Metrics and Initialization

- `packing_metrics.hpp`: Computes global and local multiplicity metrics for color usage across tiles.
  Includes cost functions and efficiency calculations based on academic research (Grange et al. 2017).
- `packing_initializer.hpp`: Initialization helpers for setting up packing data structures.

##### `packing/models/` - Packing Data Types

- `PackableTile`: Wraps a `ColorSet` with a tile ID. Supports three ID variants: `HintId`, `PrefilledPaletteId`, `RegularId`.
- `PaletteHint`: Priority tiles that guide the packing algorithm toward better solutions.
- `PrefilledPalette`: Pre-assigned palette constraints that must be respected during packing.
- `PalettePool`: Represents the available hardware palette slots for assignment.
- `PackedPalette`: Result type representing a fully assigned palette.

##### `packing/services/` - Packing Operations

- `PalettePacker`: High-level orchestration service that coordinates the packing process.
  Takes `PackingParams` (tiles, color map, hints, constraints) and produces `PalettePacking` (final assignments).
- `PackingStrategy`: Abstract interface for pluggable packing algorithms.
- `BestFusionStrategy`: Greedy fusion algorithm that iteratively merges compatible tiles into palettes.
- `OverloadAndRemoveStrategy`: Alternative algorithm that starts with overloaded palettes and removes conflicts.

The Strategy pattern enables experimentation with different packing algorithms without modifying the orchestration layer.

### `app/` - Use Cases

Application layer implements user-facing workflows.

#### `app/config/` - Application Configuration

Defines the **application-level configuration contract** through the auto-generated `AppConfig` interface.

This interface declares all configuration values needed at the application layer, generated from `config_schema.yaml`.

Like `DomainConfig` and `InfraConfig`, this interface is implemented by `LazyLayeredConfig`,
which resolves values through a chain of `ConfigProvider` instances.

#### `app/use_cases/` - Workflows

Use cases represent **complete user-facing workflows**, orchestrating domain services and repositories to accomplish application goals.
Each use case is a focused class following the Single Responsibility Principle.

Some example use cases include:

- `DefunctImportPrimaryTileset`: (DEPRECATED) Imports Porymap tilesets into Porytiles format (first time only). A new import system is being developed.
- `CompilePrimaryTileset`: Orchestrates compilation of a primary tileset from Porytiles RGBA assets to Porymap binary format.

Use cases receive dependencies via constructor injection and return `ChainableResult<T, E>` to propagate errors.
They focus purely on use case orchestration - the actual business logic lives in the various domain services.

### `infra/` - I/O and External Systems

Infrastructure layer implements concrete I/O and system integration.

#### `infra/config/` - Configuration System

Implements the **concrete configuration loading and resolution system** that powers the entire application.

The core is `LazyLayeredConfig`, which implements all three config interfaces (`DomainConfig`, `AppConfig`, `InfraConfig`).
It resolves configuration values through a priority-ordered chain of `ConfigProvider` instances:

1. YAML config file provider
2. Default provider (always succeeds, lowest priority)

(More provider implementations coming soon.)

Values are **lazily evaluated** on first access, then cached for performance.
The system tracks **provenance** - remembering which provider supplied each value for debugging.

Concrete providers include:
- `DefaultProvider`: Hard-coded fallback values, auto-generated from `config_schema.yaml`
- `YamlFileProvider`: Loads configuration from YAML files using a YAML parsing library
- `HeaderDefineProvider`: Parses C/C++ header files for `#define` constants, enabling configuration from game source headers

The `LayerValue<T>` wrapper indicates whether a provider supplied a value (similar to `std::optional`).

Example resolution:
```c++
// User requests config.tile_size()
// System checks providers in order, returns first non-empty LayerValue
// Caches result for future calls
```

#### `infra/repos/` - Concrete Persistence

Provides **filesystem-based implementations** of the repository interfaces defined in `domain/repos/`.

These adapters translate between domain abstractions and concrete file I/O:

- `ProjectTilesetArtifactReader`: Implements `TilesetArtifactReader` using filesystem paths and delegates to PNG/PAL loaders
- `ProjectTilesetArtifactWriter`: Implements `TilesetArtifactWriter` using filesystem paths and delegates to PNG/PAL savers
- `ProjectTilesetArtifactKeyProvider`: Implements `TilesetArtifactKeyProvider` by discovering artifacts in the project directory hierarchy

These classes bridge the domain's persistence abstractions with the infrastructure's I/O services.
They depend on services from `infra/services/` (PNG loaders, palette handlers) to perform actual file operations.

The "Project" prefix indicates these implementations assume a Pokémon decompilation project's directory structure.
Alternative implementations (e.g., `DatabaseTilesetArtifactReader`) could be swapped in via dependency injection.

#### `infra/services/` - I/O Implementations

Contains **low-level file I/O and data format implementations** that handle actual reading and writing of binary data.

These services specialize in specific file formats:

**PNG Handlers**:
- `PngRgbaImageLoader` / `PngRgbaImageSaver`: Load and save RGBA PNG files
- `PngIndexedImageLoader` / `PngIndexedImageSaver`: Load and save indexed (palettized) PNG files

**Palette Handlers**:
- `FilePalLoader` / `FilePalSaver`: Handle binary .pal palette files
- `JascPalLoader` / `JascPalSaver`: Handle JASC-PAL text format palette files

**Checksum Providers**:
- `ProjectArtifactChecksumProvider`: Computes file checksums for projects in the default project format
- `NoopArtifactChecksumProvider`: No-op implementation for testing or when checksums aren't needed

**Output Services**:
- `AsciiTilePrinter`: Renders tiles as ASCII art for debugging
- `ColorPalettePrinter`: Renders palettes as colored output for debugging and visualization

**Behavior Mapping**:
- `HeaderBehaviorMapProvider`: Implements `BehaviorMapProvider` by parsing C header files for metatile behavior constants

These services interact directly with libpng, file streams, and other external libraries.
They return `ChainableResult<T, E>` with infrastructure-specific error types like `ImageLoadError`.
All external dependencies (libpng, filesystem) are isolated to this layer.

### `xcut/` - Cross-Cutting Concerns

Cross-cutting concerns that don't fit neatly into a specific layer.

#### `xcut/diagnostics/` - User Communication

Provides a **structured diagnostic system** for user-facing error reporting and informational messages.

The `UserDiagnostics` abstract interface defines methods for different message severities:
- `note()`: Informational messages (tagged for categorization)
- `warning()`: Non-fatal issues that need attention (tagged)
- `error()`: Serious problems (tagged)
- `fatal()`: Unrecoverable failures with full error chains

All methods support single-line and multi-line messages for formatting flexibility.
The `fatal()` method integrates with `ChainableResult` to visualize complete error chains.

Concrete implementations:
- `StderrStyledUserDiagnostics`: Outputs to stderr with ANSI color codes or plain text based on TTY detection
- `BufferedUserDiagnostics`: Buffers messages in memory for unit testing and verification

This pattern enables:
- **Testability**: Inject buffered diagnostics in tests to verify error messages
- **Flexibility**: Switch output destinations without changing business logic
- **Rich context**: Error chains show the full path from root cause to user-facing error

#### `xcut/config/` - Configuration Utilities

Provides **shared configuration infrastructure** used across all layers.

Key components:

- `ConfigValue<T>`: Template wrapper around configuration values that tracks the value's name and provenance (which provider supplied it). Supports implicit conversion to `T` for transparent usage while preserving metadata for debugging.

- `config_validators.hpp`: Reusable validation functions for common config constraints (ranges, allowed values, format checks). These validators ensure configuration correctness before the system begins processing.

- `unwrap_config.hpp`: Utility functions for extracting raw values from `ConfigValue<T>` wrappers when metadata isn't needed.

These utilities enable the configuration system's core features:
- **Transparency**: `ConfigValue<T>` allows natural usage like `int size = config.tile_size();`
- **Debuggability**: Provenance tracking helps diagnose configuration issues
- **Validation**: Centralized validators ensure consistency across config sources

#### `xcut/di/` - Dependency Injection

Contains **dependency injection components** using the Fruit DI framework.

The `components.hpp` file defines DI component factory functions that wire up the application's object graph.
These components handle conditional binding based on runtime parameters.

Example component:
```c++
fruit::Component<TextFormatter> get_formatter_component(bool no_color) {
    if (no_color) {
        return fruit::createComponent()
            .bind<TextFormatter, PlainTextFormatter>();
    }
    return fruit::createComponent()
            .bind<TextFormatter, AnsiStyledTextFormatter>();
}
```

Components are composed at the application entry point to create an injector that provides fully-wired dependencies.
This enables:
- **Testability**: Swap implementations for testing (e.g., mock services)
- **Flexibility**: Runtime binding decisions (e.g., TTY detection for formatter selection)
- **Decoupling**: Classes depend on interfaces, concrete types bound at composition root

### `utilities/` - Low-Level Helpers

Zero dependencies, reusable utilities.

#### `utilities/functional/` - Functional Programming

Provides **higher-order operations** on containers using modern C++23 features.

The `transform.hpp` header offers two overloads for transforming vectors:

1. **Range-based transform**: Takes a `vector<T>` and a transformation function, returns `vector<U>` using C++23 ranges and views.
2. **Direct construction**: Transforms a `vector<T>` to `vector<U>` via direct element-by-element construction.

These utilities enable functional programming patterns without external dependencies.

Example usage:
```c++
std::vector<int> values = {1, 2, 3, 4};
auto doubled = transform(values, [](int x) { return x * 2; });
// doubled = {2, 4, 6, 8}
```

#### `utilities/panic/` - Termination

Implements **unrecoverable error handling** through program termination rather than exceptions.

The `panic.hpp` header provides:
- `panic(message)`: Immediately terminates with an error message
- `assert_or_panic(condition, message)`: Asserts a condition or terminates if false

Both functions automatically capture source location (file, line, function) and format diagnostic output before calling `std::abort()`.

**Why panic instead of exceptions?**
- Used for **programmer errors** and precondition violations, not recoverable runtime errors
- Avoids exception overhead and complexity
- Makes unrecoverable failures explicit in the API
- Precondition violations indicate bugs that must be fixed, not handled

Panics print formatted diagnostics to stderr:
```
Porytiles/lib/domain/services/primary_tileset_compiler.cpp:307 panic: index 12 out of bounds, size 8
```

For recoverable errors, use `ChainableResult<T, E>` from `utilities/result/` instead.

#### `utilities/result/` - Error Handling

Provides **type-safe error propagation** without exceptions using the Result monad pattern.

**Core types**:

- `ChainableResult<T, E>`: Either a success value `T` or an error chain. Move-only semantics ensure efficient transfers. Supports chaining errors with the constructor `ChainableResult(error, cause_result)` to preserve the full propagation path from root cause to proximate error.

- `Error`: Polymorphic error interface with `details()` for messages, `join()` for multi-error aggregation, and `clone()` for copying.

- `FormattableError`: Concrete implementation supporting plain strings and formatted messages with styled parameters for rich terminal output.

**Why not exceptions?**
- Explicit error paths in function signatures
- Zero overhead when successful
- Compiler-enforced error handling (no silent failures)
- Better debugging through error chains

Each layer adds context. Users see the complete story, not just "compilation failed."
The diagnostic system uses these chains to produce detailed, user-friendly error messages.

Example error chain printout:
```c++
fatal: failed to compile tileset 'MyTileset'
│
├ caused by:
│
│ failed to get config value 'MyTileset:num_pals_total'
│
├ root cause:
│
│ 'MyTileset:num_pals_primary' must be greater than '0'
│
│ MyTileset:num_pals_primary = 0
│ Source: ./data/tilesets/primary/MyTileset/porytiles.local.yaml:4
│
│    1:   foo:
│    2:     bar: baz
│    3:   fieldmap:
│ -> 4:     num_pals_primary: 0
```

#### `utilities/text/` - Text Formatting

Provides **TTY-aware styled text output** for terminal applications.

**Core abstractions**:

- `TextFormatter`: Abstract interface with `style()` (apply style flags to text) and `format()` (substitute styled parameters into format strings). The `Style` enum uses bitmask flags for composable styles (bold, italic, colors).

- `AnsiStyledTextFormatter`: Applies ANSI escape codes for terminal styling (colors, bold, italic, etc.)

- `PlainTextFormatter`: Returns unstyled text for non-TTY output or when colors are disabled

- `FormatParam`: Pairs text with `Style` flags for parameter substitution in format strings

The system integrates with fmtlib for printf-style formatting while adding styled parameter support.
TTY detection happens at the application layer, which selects the appropriate formatter via dependency injection.

Example usage:
```c++
formatter.format("Found {} errors in {}",
    FormatParam{"5", Style::bold | Style::red},
    FormatParam{"tiles.png", Style::italic});
// Output: "Found **5** errors in *tiles.png*" (with ANSI codes in terminals)
```

This enables rich, user-friendly diagnostic output while maintaining plain text compatibility for logging and CI environments.

#### Other Utilities

- `count_map_to_list.hpp`: Converts `std::map<T, size_t>` count maps to sorted vectors of pairs, ordered by count descending. Useful for frequency analysis in packing algorithms.

- `parse_int.hpp`: Type-safe integer parsing from strings with `std::expected` error handling. Supports arbitrary integer types and numeric bases.

- `reverse_bits.hpp`: Constexpr bit reversal for bytes. Used in tile data transformations for GBA hardware compatibility where bit ordering differs.

- `source_locations.hpp`: Extracts clean function names from `std::source_location`, stripping qualifiers and return types for readable panic diagnostics.

- `stream_digest.hpp`: Computes MD5 checksums over input streams via the `StreamDigest` class. Powers the artifact checksum system for incremental compilation.

- `string_utils.hpp`: String manipulation utilities including regex full-match checking (`check_full_string_match`) and in-place whitespace trimming (`trim`).

## Cross-Cutting Concerns

### Configuration System

**Layered resolution** through multiple ConfigProvider instances:

1. Command-line provider (if implemented)
2. Tileset-specific YAML provider
3. Global config provider
4. Default provider (always succeeds)

**Lazy evaluation**: Values computed on first access, then cached.

**Provenance tracking**: Remember which provider supplied each value for debugging.

**Type-safe**: ConfigValue<T> ensures type correctness.

All interfaces and providers are **auto-generated** from a single source of truth: `config_schema.yaml`.

### Dependency Injection

The system uses the **Fruit DI framework** for compile-time dependency injection with runtime binding decisions.

**Current DI Components** (in `xcut/di/components.hpp`):
- `get_formatter_component(bool no_color)`: Conditionally binds `TextFormatter` to either `AnsiStyledTextFormatter` or `PlainTextFormatter` based on TTY detection and user preferences.

**DI Migration Status**:
Most services are currently manually instantiated in command handlers (see TODOs in `tools/driver/command_*.hpp`).
Full DI migration is planned to move service construction to the composition root.

**Why Fruit?**
- Compile-time dependency checking catches wiring errors early
- No reflection or runtime type information needed
- Clean integration with C++ RAII and move semantics
- Supports conditional binding for runtime configuration

### Diagnostics Integration

**Structured output** through UserDiagnostics interface:

- Notes: Informational (tagged)
- Warnings: Non-fatal issues (tagged)
- Errors: Serious issues (tagged)
- Fatal: Complete failure with chain

**TTY-aware**: TextFormatter determines styling (ANSI vs plain).

**Testable**: BufferedUserDiagnostics for unit tests.

### Testing Strategy

#### Unit Tests (`tests/unit/`)

Each unit test evaluates a single component in complete isolation.
All dependencies, if any, are mocked.
There are no external system dependencies, including the filesystem.

#### Integration Tests (`tests/integration/`)

Each integration test evaluates one or more components together.
Dependencies may be mocked or stubbed, or they may be injected using real components.
Integration tests may make use of the external network or filesystem.

## Design Principles

### Runtime Safety

- No raw owning pointers, no `new`, no `delete`
- No unsafe memory access patterns except where necessary for I/O

### Type Safety

- Strong types prevent mixing unrelated values (ArtifactKey ≠ string)
- Generic template aggregates provide pixel type flexibility (PixelTile<Rgba32> vs PixelTile<IndexPixel>)
- Concepts (SupportsTransparency) enforce requirements at compile time

### Separation of Concerns

- **Domain**: Business logic, zero I/O
- **App**: Coordination only, delegates to domain
- **Infra**: All I/O, all external dependencies, injected into domain/app via composition root
- **XCut**: Shared concerns (errors, diagnostics)
- **Utilities**: Pure helpers

### Composability

- Small, focused operations that do one thing well
- Services follow SRP and DIP
- Dependency injection for flexibility

### Extensibility

- Configuration values are generated from a single YAML source of truth
- ConfigProvider pattern for easily adding new config sources
- Repositories use reader/writer interfaces to support multiple artifact formats
- UserDiagnostics for flexible output formats

### Testability

- Domain has no dependencies, trivial to test
- Services can be tested with mock implementations
- BufferedUserDiagnostics for capturing output
- No global state, pure functions where possible

## Entry Points

### Driver Program (`tools/driver/`)

The `porytiles` executable uses the **CLI11** library for argument parsing with a custom command/option abstraction layer.

**Execution Flow**:
1. Parses command-line arguments via CLI11
2. Initializes configuration (DefaultProvider → YamlFileProvider → HeaderDefineProvider → defaults)
3. Creates DI container (Fruit) for cross-cutting services
4. Dispatches to appropriate command handler
5. Command handler constructs domain services and invokes use case
6. Displays results through UserDiagnostics

**Command Pattern**:
The driver uses a `Command` base class that each subcommand inherits from:
- `CompileTilesetCommand`: Compiles Porytiles assets to Porymap format
- `DefunctImportTilesetCommand`: (DEPRECATED) Imports existing Porymap tilesets. A new import system is being developed.

Each command registers itself with CLI11 and implements a `Run()` method that orchestrates the corresponding use case.

**Option Groups**:
Related options are grouped via `OptGroup` abstractions:
- `OptGroupFieldmap`: Base game presets, tile/metatile/palette count overrides
- `OptGroupDiagnostics`: Warning levels, color output, verbosity settings

This organization keeps related configuration together and enables reuse across commands.

### Extension Points

To add new functionality:

1. **New use case**: Create new file in `app/use_cases/`
2. **New config value**: Add to `config_schema.yaml`, regenerate
3. **New config source**: Implement `ConfigProvider`
4. **New artifact format**: Implement `TilesetArtifactReader/Writer`

## Bibliography and Inspiration

- [matklad's ARCHITECTURE.md](https://matklad.github.io/2021/02/06/ARCHITECTURE.md.html)
- [rust-analyzer architecture](https://github.com/rust-lang/rust-analyzer/blob/main/docs/dev/architecture.md)
- Domain-Driven Design by Eric Evans
- Clean Architecture by Robert C. Martin
