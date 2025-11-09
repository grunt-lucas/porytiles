# Architecture

This document describes the high-level architecture of Porytiles2.
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
    - [Error Handling Strategy](#error-handling-strategy)
    - [Configuration System](#configuration-system)
    - [Diagnostics Integration](#diagnostics-integration)
  - [Data Flow](#data-flow)
    - [End-to-End Compilation](#end-to-end-compilation)
    - [Incremental Build Detection](#incremental-build-detection)
  - [Design Principles](#design-principles)
    - [Type Safety](#type-safety)
    - [Separation of Concerns](#separation-of-concerns)
    - [Composability](#composability)
    - [Extensibility](#extensibility)
    - [Testability](#testability)
  - [Testing Strategy](#testing-strategy)
    - [Unit Tests (`tests/unit/`)](#unit-tests-testsunit)
    - [Integration Tests (`tests/integration/`)](#integration-tests-testsintegration)
  - [Entry Points](#entry-points)
    - [Driver Program (`tools/driver/`)](#driver-program-toolsdriver)
    - [Extension Points](#extension-points)
  - [Bibliography and Inspiration](#bibliography-and-inspiration)


## Bird's Eye View

Porytiles2 is a C++ tileset compiler that transforms RGBA image assets into Porymap-ready binary assets for Pokémon Generation III decompilation projects.

The system is organized around a **layered architecture** inspired by domain-driven design.

```
    domain/       -- Pure business logic, no I/O dependencies
      │
    app/          -- User-facing use cases and workflows
      │
    infra/        -- I/O and external system/library integration
      │
    xcut/         -- Cross-cutting concerns (errors, diagnostics, config, di, etc)
      │
    utilities/    -- Generic helpers, zero dependencies
```

### Layer Dependencies

The dependency flow is strictly one-way:

- **utilities** can depend on: nothing
- **xcut** can depend on: utilities only
- **domain** can depend on: utilities, xcut only
- **app** can depend on: utilities, xcut, domain only
- **infra** can depend on: everything (utilities, xcut, domain, app)

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
- Keep domain types decoupled (e.g. two-way conversion between PixelTile → ShapeTile without causing circular dependencies)
- Enable generic programming through templates
- Pure functions are trivial to test and reason about
- Clean call sites: `auto shape = from_pixel_tile(pixel, map);`

See `Porytiles2/Notes/service_vs_free_function_architecture.md` for the complete decision framework.

#### `domain/config/` - Configuration Interfaces

In addition to the generated DomainConfig layer configuration interface,
additional domain-layer configuration helpers are defined here.

#### `domain/models/` - Core Data Types

The `models/` builds out the core domain language for Porytiles.
Fundamental building block value objects like Rgba32 and IndexPixel
provide the meat for template aggregates like PixelTile, Metatile, and Image.

#### `domain/repos/` - Persistence Abstractions

TODO

#### `domain/services/` - Orchestrated Operations

TODO

### `app/` - Use Cases

Application layer implements user-facing workflows.

#### `app/config/` - Application Configuration

TODO

#### `app/use_cases/` - Workflows

TODO

### `infra/` - I/O and External Systems

Infrastructure layer implements concrete I/O and system integration.

#### `infra/config/` - Configuration System

TODO

#### `infra/repos/` - Concrete Persistence

TODO

#### `infra/services/` - I/O Implementations

TODO

### `xcut/` - Cross-Cutting Concerns

Cross-cutting concerns that don't fit neatly into a specific layer.

#### `xcut/diagnostics/` - User Communication

TODO

#### `xcut/config/` - Configuration Utilities

TODO

#### `xcut/di/` - Dependency Injection

TODO

### `utilities/` - Low-Level Helpers

Zero dependencies, reusable utilities.

#### `utilities/functional/` - Functional Programming

TODO

#### `utilities/panic/` - Termination

TODO

#### `utilities/result/` - Error Handling

TODO

#### `utilities/text/` - Text Formatting

TODO

#### Other Utilities

- `parse_int.hpp`: string-to-int parsing
- `reverse_bits.hpp`: Bit manipulation
- `stream_digest.hpp`: Hash over stream data
- `source_locations.hpp`: Source location tracking
- `string_utils.hpp`: String manipulation

## Cross-Cutting Concerns

### Error Handling Strategy

**No Exceptions**: Porytiles2 uses error-as-value approach with ChainableResult<T, E>.

**Why not exceptions:**
- Deterministic error handling
- Easier to trace error paths
- Better for performance-critical code
- Clearer documentation of failure modes

**Error Chains**: Full path from root cause to proximate cause preserved.

Example chain for "failed to compile tileset":
```
1. (proximate) file not found
2. (step) failed to load image
3. (root) tileset not readable
```

Users see full context, not just "failed to compile".

### Configuration System

**Layered resolution** through multiple ConfigProvider instances:

1. Command-line provider (if implemented)
2. Tileset-specific YAML provider
3. Global config provider
4. Default provider (always succeeds)

**Lazy evaluation**: Values computed on first access, then cached.

**Provenance tracking**: Remember which provider supplied each value for debugging.

**Type-safe**: ConfigValue<T> ensures type correctness.

### Diagnostics Integration

**Structured output** through UserDiagnostics interface:

- Notes: Informational (tagged)
- Warnings: Non-fatal issues (tagged)
- Errors: Serious issues (tagged)
- Fatal: Complete failure with chain

**TTY-aware**: TextFormatter determines styling (ANSI vs plain).

**Testable**: BufferedUserDiagnostics for unit tests.

## Data Flow

???

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

## Testing Strategy

### Unit Tests (`tests/unit/`)

Each unit test evaluates a single component in complete isolation.
All dependencies, if any, are mocked.
There are no external system dependencies, including the filesystem.

### Integration Tests (`tests/integration/`)

Each integration test evaluates one or more components together.
Dependencies may be mocked or stubbed, or they may be injected using real components.
Integration tests may make use of the external network or filesystem.

## Entry Points

### Driver Program (`tools/driver/`)

The `porytiles2` executable:
1. Parses command-line arguments
2. Initializes configuration (DefaultProvider → YamlFileProvider → defaults)
3. Creates DI container (Fruit)
4. Dispatches to appropriate use case (create, compile, import, verify)
5. Displays results through UserDiagnostics

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
