# Architecture

This document describes the high-level architecture of Porytiles2.
If you want to familiarize yourself with the code base, you are just in the right place!

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
    xcut/         -- Cross-cutting concerns (errors, diagnostics, config)
      │
    utilities/    -- Low-level helpers, zero dependencies
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

#### `domain/models/` - Core Data Types

**Pixels and Colors:**
- `Rgba32`: 32-bit RGBA color (8 bits per channel)
  - Intrinsic transparency: alpha channel indicates opacity
  - Extrinsic transparency: comparison with a reference color
  - Used as input pixel format and palette colors

- `IndexPixel`: Palette index (integer)
  - Intrinsic transparency: index 0 is conventionally transparent
  - Used as output pixel format in indexed color mode

**Tiles - Basic Unit:**
- `PixelTile<T>`: 8x8 grid of pixels
  - Template parameter `T` must satisfy `SupportsTransparency` concept
  - Direct per-pixel access: `at(row, col)`, `at(index)`
  - Transparency checking: all pixels transparent?
  - Flipping operations: horizontal and vertical
  - Color extraction: unique non-transparent colors
  - Invariant: default-constructed tile is fully transparent

- `Metatile<T>`: 2x2 grid of PixelTile objects across 3 layers
  - Layer::bottom (index 0) - background
  - Layer::middle (index 1) - middleground
  - Layer::top (index 2) - foreground
  - Each layer contains 2x2 PixelTile grid (4 tiles per layer, 12 total)
  - Access by layer then subtile position: `bottom(i)`, `middle(i)`, `top(i)`
  - Decomposition: extract all 12 tiles as flat array

**Images - Large Containers:**
- `Image<T>`: 2D pixel grid of arbitrary size
  - Template support for any pixel type
  - Both 2D and linear indexing
  - Optional palette (for indexed color images)
  - Width/height tracking

**Tileset Components:**
- `PorytilesTilesetComponent`: Input asset
  - Three Image<Rgba32> layers: bottom, middle, top
  - Raw RGBA data from user assets

- `PorymapTilesetComponent`: Output asset (Porymap format)
  - `metatiles_bin_`: Vector<TilemapEntry> - metatile references
  - `metatile_attributes_`: Vector<MetatileAttribute> - behavior data
  - `tiles_png_`: Image<IndexPixel> - compiled tile image
  - `pals_`: Array<Palette<Rgba32>, 16> - color palettes
  - Layer mode detection: dual (2 layers) vs triple (3 layers)

- `Tileset`: Aggregate root
  - Combines PorytilesTilesetComponent and PorymapTilesetComponent
  - Name for identification
  - Ensures both components are always valid

**Output References:**
- `TilemapEntry`: Metatile entry in output
  - `tile_index`: which tile to use
  - `pal_index`: which palette (0-15)
  - `hflip`: horizontal flip flag
  - `vflip`: vertical flip flag
  - Invariant: default entry (tile_index=0) refers to transparent tile

- `MetatileAttribute`: Metatile behavior data
  - `layer_type`: normal, covered, or split
  - `behavior`: behavior flags

**Palette and Color Management:**
- `Palette<T>`: Vector of colors (typically 16)
  - Add, set, access colors
  - Size tracking

- `ColorSet`: Bitset of active color indices
  - Test, set, reset individual color indices
  - Hashable for use in maps/sets

- `ColorIndex`: Strong type wrapper for color indices
  - Prevents accidental integer confusion

#### `domain/algorithms/` - Pure Functions

Domain algorithms are **pure transformations** implemented as free functions:

**Key Characteristics:**
- No external dependencies (beyond standard library)
- Pure data transformations with no side effects
- Template-based for type flexibility
- Clean signatures (typically ≤2 parameters)
- Avoid circular dependencies between types

**Examples:**

- `tile_converters.hpp`: Type conversions between tile representations
  - `from_pixel_tile(pixel_tile, color_index_map)`: Convert PixelTile → ShapeTile
  - `from_shape_tile(shape_tile, color_index_map)`: Convert ShapeTile → PixelTile
  - `shape_tile_to_pixel_colors(shape_tile, color_index_map)`: Extract colors
  - These avoid coupling PixelTile and ShapeTile directly

**Why Free Functions?**
- Keep domain types decoupled (no PixelTile → ShapeTile dependency)
- Enable generic programming through templates
- Pure functions are trivial to test and reason about
- Clean call sites: `auto shape = from_pixel_tile(pixel, map);`

See `Porytiles2/Notes/service_vs_free_function_architecture.md` for the complete decision framework.

#### `domain/services/` - Orchestrated Operations

Domain services implement **complex multi-step processes** requiring dependencies:

- `ImageTileizer<T>`: Decompose Image<T> into PixelTile<T> vector
  - Validates image dimensions are multiples of 8
  - Processes row-major order
  - Generic pixel type support

- `PrimaryTilesetCompiler`: Main orchestration service
  - Coordinates the compilation pipeline
  - Invokes validators, color builders, etc.

- `TileValidator`: Validates tiles meet requirements
  - Checks transparency requirements
  - Validates color counts

- `ColorSetBuilder`: Builds color sets from palettes
  - Maps colors to indices

- `LayerModeConverter`: Detects layer mode
  - Analyzes metatile structure
  - Determines if dual or triple mode

- `AssetGenerator`: Produces final output assets
  - Combines tiles, palettes, and attributes

- `MegatileDecompiler`: Decomposes metatiles into tiles

- `PackSetGenerator`: Generates optimized tile packs
  - Deduplication and packing optimization

#### `domain/repos/` - Persistence Abstractions

Repository interfaces define how domain objects persist:

- `TilesetRepo`: Repository for Tileset aggregate
  - `save(tileset)`: Persist to backing store, update checksums
  - `load(name)`: Load by name
  - `exists(name)`: Check existence
  - Dependencies: checksum_provider, key_provider, reader, writer
  - Note: Repo is responsible for checksum updates, not just saving

- `TilesetArtifactKeyProvider`: Discover available artifacts
  - Interface for finding what's in backing store
  - Implemented by infra (ProjectTilesetArtifactKeyProvider)

- `TilesetArtifactReader`: Load artifact data
  - Interface for reading individual artifacts
  - Implemented by infra (ProjectTilesetArtifactReader)

- `TilesetArtifactWriter`: Save artifact data
  - Interface for writing individual artifacts
  - Implemented by infra (ProjectTilesetArtifactWriter)

- `ArtifactChecksumProvider`: Cache artifact checksums
  - `compute(artifact)`: Get or compute checksum
  - Used for incremental builds
  - Implemented by infra (ProjectArtifactChecksumProvider)

- `ArtifactKey`: Strong type for artifact identifiers
  - Wraps string key
  - Hashable for use in maps

#### `domain/config/` - Configuration Interfaces

- `DomainConfig`: Abstract config interface for domain layer
  - Tileset-specific numeric settings (num_tiles_primary, num_metatiles_total, etc.)
  - Color configuration (extrinsic_transparency)
  - Layer configuration (patch_build_enabled, num_tiles_per_metatile)
  - Implemented by LazyLayeredConfig in infra

### `app/` - Use Cases

Application layer implements user-facing workflows.

#### `app/config/` - Application Configuration

- `AppConfig`: Abstract config interface for application layer
- `IncrementalBuildMode`: Enum for build strategies (full, incremental, etc.)

#### `app/use_cases/` - Workflows

- `CreatePrimaryTileset`: Create new tileset
  - Input: tileset name, source asset paths
  - Process: load images, compile, save
  - Output: persisted Tileset

- `CompilePrimaryTileset`: Recompile existing tileset
  - Input: tileset name
  - Process: load from store, recompile, save back
  - Used when configuration changes

- `ImportPrimaryTileset`: Import external tileset
  - Input: external asset paths
  - Process: load external format, convert, validate, save
  - Output: imported Tileset in system format

- `VerifyPrimaryTileset`: Validate tileset integrity
  - Input: tileset name
  - Process: load and validate structure
  - Output: validation results

### `infra/` - I/O and External Systems

Infrastructure layer implements concrete I/O and system integration.

#### `infra/config/` - Configuration System

- `ConfigProvider`: Abstract base for configuration sources
  - `get_value<T>(key, tileset)`: Get typed configuration value
  - Returns LayerValue<T> with metadata

- `DefaultProvider`: Hardcoded defaults
  - Provides baseline values for all settings
  - Always last in provider chain

- `YamlFileProvider`: Load from YAML files
  - Reads tileset.yaml or other YAML configs
  - Type conversion and validation

- `LazyLayeredConfig`: Multi-layer configuration resolver
  - Implements DomainConfig, AppConfig, InfraConfig interfaces
  - Consults providers in priority order (highest first)
  - Lazy evaluation: resolve on first access
  - Caching: store resolved values
  - Provenance tracking: remember which provider supplied each value
  - `dump()`: Debug output of all cached values

- `ConfigValue<T>`: Resolved configuration value
  - Wraps the actual value
  - Includes metadata (source provider, source details)

- `InfraConfig`: Abstract config interface for infrastructure layer
  - I/O settings (file paths, formats)
  - Output modes (TilesPalMode)

- `TilesPalMode`: PNG color mode (indexed vs RGBA)

#### `infra/repos/` - Concrete Persistence

- `ProjectTilesetArtifactKeyProvider`: Find artifacts in project
  - Scans `data/tilesets/<name>/` directory
  - Discovers .png and .pal files

- `ProjectTilesetArtifactReader`: Load from files
  - Reads PNG images using libpng
  - Reads PAL files (JASC or binary format)
  - Returns artifact data or error

- `ProjectTilesetArtifactWriter`: Save to files
  - Writes PNG images using libpng
  - Writes PAL files in appropriate format
  - Creates directories as needed

- `ProjectArtifactChecksumProvider`: Cache checksums
  - Computes MD5/SHA hash of files
  - Caches in memory during session
  - Used for incremental build detection

#### `infra/services/` - I/O Implementations

**Image Loading/Saving:**
- `PngRgbaImageLoader`: Load PNG as RGBA
- `PngRgbaImageSaver`: Save PNG as RGBA
- `PngIndexedImageLoader`: Load PNG as indexed color
- `PngIndexedImageSaver`: Save PNG as indexed color

**Palette Loading/Saving:**
- `JascPalLoader`: Load JASC palette format (text)
- `JascPalSaver`: Save JASC palette format
- `FilePalLoader`: Load binary palette files
- `FilePalSaver`: Save binary palette files

**Diagnostics:**
- `AsciiTilePrinter`: Print ASCII representation of tiles for debugging

**Checksums:**
- `NoopArtifactChecksumProvider`: No-op implementation (always recompile)
- `ProjectArtifactChecksumProvider`: File-based checksums

**Error Types:**
- `ImageLoadError`: Domain error type for image loading failures

### `xcut/` - Cross-Cutting Concerns

Cross-cutting concerns that don't fit neatly into layers.

#### `utilities/result/` - Error Handling

- `Error`: Abstract interface for all error types
  - `details(formatter)`: Get formatted error lines
  - `clone()`: Polymorphic copy
  - Implemented by FormattableError and custom types

- `FormattableError`: General-purpose error implementation
  - Stores error message text and optional format parameters
  - Constructors for various input styles (string, vector, formatted)
  - TTY-aware formatting through TextFormatter
  - Empty FormattableError for error chain passthrough

- `ChainableResult<T, E>`: Result type with error chains
  - Wraps std::expected<T, E>
  - Maintains vector<unique_ptr<Error>> error chain
  - Move-only semantics (no copies)
  - Template specialization for void type
  - Constructors:
    - `ChainableResult(T value)`: Success path
    - `ChainableResult(const E& error)`: Initial error
    - `ChainableResult(const E& error, const ChainableResult& cause)`: Chain error
  - Methods:
    - `has_value()`: Check for success or error
    - `value()`: Access success value
    - `error()`: Access immediate error
    - `chain()`: Access full error chain
    - `add_cause()`: Append another chain

- **Error Handling Macros**:
  - `PT_TRY_ASSIGN_CHAIN_ERR(var, expr, msg, return_type)`: Unwrap and chain message
  - `PT_TRY_ASSIGN_PASS_ERR(var, expr, return_type)`: Unwrap and passthrough empty error
  - `PT_TRY_ASSIGN_PASS_SAME_ERR(var, expr)`: Unwrap and passthrough same error
  - `PT_TRY_CALL_*`: Void versions of above

**Error Flow Example:**
```c++
// Low level: create initial error
if (!parse_file(path)) {
    return FormattableError{"could not parse file"};
}

// Mid level: add context
PT_TRY_ASSIGN_CHAIN_ERR(data, parse_result, "failed to load config", ConfigData);

// High level: fatal error display
if (!result.has_value()) {
    diag.fatal(result);  // Displays full chain from proximate to root
}
```

#### `xcut/diagnostics/` - User Communication

- `UserDiagnostics`: Abstract interface for diagnostics
  - Severity levels: note, warning, error, fatal
  - Methods support both single and multi-line messages
  - `note(tag, msg)`: Informational messages with category
  - `warn(tag, msg)`: Non-fatal warnings
  - `warn_note(tag, msg)`: Notes that accompany warnings
  - `err(tag, msg)`: Serious issues
  - `emit_fatal_proximate(err)`: Display immediate error
  - `emit_fatal_step(err)`: Display intermediate error
  - `emit_fatal_root(err)`: Display root cause
  - `fatal(result)`: Process ChainableResult error chain and display as tree

- `StderrStyledUserDiagnostics`: Write to stderr with ANSI colors
  - Formats messages with source location and style
  - Pretty-prints error chains

- `BufferedUserDiagnostics`: Collect in memory (for testing)
  - Stores all diagnostics for test assertions

#### `xcut/config/` - Configuration Utilities

- `ConfigValue<T>`: Resolved configuration value wrapper
  - Holds actual value of type T
  - Metadata: source provider, source_details

- `ConfigValidator`: Validate configuration constraints
  - Domain-specific validation rules

- `UnwrapConfig`: Extract values from layered config
  - Helper for accessing config values

### `di/` - Dependency Injection

- `Components`: Fruit DI component definitions
  - `get_formatter_component(no_color)`: Conditional TextFormatter
  - Returns AnsiStyledTextFormatter (default) or PlainTextFormatter (no_color=true)
  - Demonstrates runtime conditional injection

### `utilities/` - Low-Level Helpers

Zero dependencies, reusable utilities.

#### `utilities/panic/` - Termination

- `StringViewSourceLoc`: Message + source location struct
- `panic(msg)`: Unconditional abort with location and message
- `assert_or_panic(condition, msg)`: Conditional abort

#### `utilities/text/` - Text Formatting

- `TextFormatter`: Abstract interface
  - `format(text, params)`: Format with parameters
  - TTY detection (ANSI vs plain)

- `FormatParam`: Styled text parameter
  - Value + Style bitmask
  - Used in format strings

- `Style`: ANSI style attributes
  - bold, red, green, etc.
  - Bitmask for combining

- `AnsiStyledTextFormatter`: ANSI output for TTY
  - Substitutes styles into format strings
  - Outputs ANSI escape sequences

- `PlainTextFormatter`: Plain text without styling
  - Removes all styling
  - Used when no TTY or --no-color

#### `utilities/functional/` - Functional Programming

- `transform.hpp`: Functional transformation utilities
  - Higher-order functions for common patterns

#### Other Utilities

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

### End-to-End Compilation

```
CreatePrimaryTileset use case
    ↓
LoadSourceImages (infra)
    ├── PngRgbaImageLoader.load(path) → Image<Rgba32>
    ├── Process three layers (bottom, middle, top)
    └── Create PorytilesTilesetComponent
    ↓
PrimaryTilesetCompiler service
    ├── ImageTileizer<Rgba32>.tileize() → PixelTile<Rgba32>[]
    ├── Pipeline:
    │   ├── TileValidator → check constraints
    │   ├── ColorSetBuilder → build palette
    │   ├── LayerModeConverter → detect mode
    │   └── AssetGenerator → create output
    └── → PorymapTilesetComponent
    ↓
SaveArtifacts (infra)
    ├── ProjectTilesetArtifactWriter.save()
    ├── PngIndexedImageSaver → tiles.png
    ├── JascPalSaver → palettes.pal
    └── TilesetRepo → metatiles.bin, attributes.bin
    ↓
UpdateChecksums
    └── ProjectArtifactChecksumProvider.compute()
```

### Incremental Build Detection

```
Load existing tileset
    ↓
Check source image timestamp/checksum
    ├── No change → skip compilation
    └── Changed → recompile
    ↓
Configuration change detection
    ├── No change in config → skip compilation
    └── Change → recompile
    ↓
If recompiling: run full pipeline
```

## Design Principles

### Type Safety

- Strong types prevent mixing unrelated values (ArtifactKey ≠ string)
- Generic types provide pixel type flexibility (PixelTile<Rgba32> vs PixelTile<IndexPixel>)
- Concepts (SupportsTransparency) enforce requirements at compile time
- No unsafe code except where necessary for I/O

### Separation of Concerns

- **Domain**: Business logic, zero I/O
- **Infra**: All I/O, all external dependencies
- **App**: Coordination only, delegates to domain and infra
- **XCut**: Shared concerns (errors, diagnostics)
- **Utilities**: Pure helpers

### Composability

- Small, focused operations that do one thing well
- Services with single responsibility
- Dependency injection for flexibility

### Extensibility

- Add ConfigProvider for new config sources
- Implement TilesetArtifactReader/Writer for new formats
- Implement UserDiagnostics for new output targets
- Implement TileValidator subclasses for new constraints

### Testability

- Domain has no dependencies, trivial to test
- Services can be tested with mock implementations
- BufferedUserDiagnostics for capturing output
- No global state, pure functions where possible

## Testing Strategy

### Unit Tests (`tests/unit/`)

Each unit test evaluates a single component in complete isolation. All dependencies, if any, are mocked.
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
