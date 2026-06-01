# Google Fruit DI Migration Plan for Porytiles

**Document Version:** 1.0
**Created:** 2025-10-21
**Target Codebase:** Porytiles

---

## Table of Contents

1. [Overview & Rationale](#overview--rationale)
2. [Phase 1: Setup & Integration](#phase-1-setup--integration)
3. [Phase 2: Component Architecture](#phase-2-component-architecture)
4. [Phase 3: Migration Strategy](#phase-3-migration-strategy)
5. [Phase 4: Advanced Patterns](#phase-4-advanced-patterns)
6. [Phase 5: Implementation Checklist](#phase-5-implementation-checklist)
7. [Code Examples](#code-examples)
8. [Testing Strategy](#testing-strategy)
9. [Troubleshooting](#troubleshooting)

---

## Overview & Rationale

### Why Google Fruit?

**Primary Requirement:** Runtime conditional injection based on CLI flags and environment (e.g., TTY detection for color output).

**Decision Matrix:**

| Library | Runtime Conditional Injection | Code Compatibility | Performance | Verdict |
|---------|------------------------------|-------------------|-------------|---------|
| Boost.DI | ⚠️ Awkward, requires workarounds | ✅ Perfect (works with gsl::not_null) | ✅ Zero overhead | ❌ Poor fit for runtime conditions |
| Google Fruit | ✅ Natural and clean | ✅ Good (works with raw pointers) | ✅ Minimal overhead | ✅ **Best choice** |
| Hypodermic | ✅ Decent | ❌ Requires shared_ptr everywhere | ⚠️ Reference counting overhead | ❌ Breaking changes required |

**Key Benefits for Porytiles:**
1. ✅ **Runtime conditional injection** - swap implementations based on CLI flags/TTY detection
2. ✅ **Component-based architecture** - modular, scalable design
3. ✅ **Works with existing gsl::not_null<T*>** - minimal refactoring
4. ✅ **Excellent for CLI tools** - designed for runtime configuration
5. ✅ **Scales well** - as project complexity grows, DI handles the wiring

---

## Phase 1: Setup & Integration

### Step 1.1: Add Fruit Dependency to CMake

**File:** `CMakeLists.txt` (root or porytiles/CMakeLists.txt)

```cmake
# Option 1: Using FetchContent (recommended for simplicity)
include(FetchContent)

FetchContent_Declare(
    fruit
    GIT_REPOSITORY https://github.com/google/fruit.git
    GIT_TAG v3.7.1  # Use latest stable version
)

# Fruit configuration options
set(FRUIT_USES_BOOST FALSE CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(fruit)

# Link Fruit to your targets
target_link_libraries(PorytilesLib PUBLIC fruit)
target_link_libraries(porytiles PRIVATE fruit)
```

```cmake
# Option 2: Using find_package (if Fruit is system-installed)
find_package(Fruit 3.7 REQUIRED)

target_link_libraries(PorytilesLib PUBLIC Fruit::fruit)
target_link_libraries(porytiles PRIVATE Fruit::fruit)
```

**Build verification:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j7
```

### Step 1.2: Create DI Directory Structure

```
porytiles/
├── include/porytiles/
│   └── di/
│       ├── components.hpp          # Main component definitions
│       ├── cli_config.hpp          # Runtime configuration struct
│       └── injector_factory.hpp    # Injector creation utilities
└── lib/di/
    ├── components.cpp              # Component implementations
    └── injector_factory.cpp        # Injector factory implementations
```

**Rationale:** Separate DI configuration from domain logic, keeping services focused on business logic.

---

## Phase 2: Component Architecture

### Component Hierarchy

```
Application Component
├── Formatter Component (CONDITIONAL)
│   ├── AnsiStyledTextFormatter (if TTY && !--no-color)
│   └── PlainTextFormatter (otherwise)
├── Foundation Component
│   ├── UserDiagnostics (depends on TextFormatter)
│   └── TilePrinter (depends on TextFormatter)
├── Config Component
│   └── DomainConfig (LazyLayeredConfig)
├── Loader Services Component
│   ├── PngRgbaImageLoader
│   ├── PngIndexedImageLoader
│   └── JascPalLoader
├── Domain Services Component
│   ├── PrimaryTilesetCompiler
│   ├── TileValidator
│   ├── ColorSetBuilder
│   └── MetatileDecompiler (requires factory)
└── Repository Component
    ├── ArtifactChecksumProvider
    ├── TilesetRepo
    └── Artifact readers/writers
```

### Step 2.1: CLI Configuration Structure

**File:** `porytiles/include/porytiles/di/cli_config.hpp`

```C++
#pragma once

#include <string>
#include <unistd.h>  // For isatty()

namespace porytiles::di {

/**
 * @brief Runtime configuration derived from CLI arguments and environment.
 *
 * @details
 * This structure captures all runtime parameters that affect dependency injection decisions.
 * It's used to conditionally install different component implementations based on user preferences
 * and environment detection (e.g., TTY detection for color output).
 */
struct CliConfig {
    bool no_color{false};           ///< User explicitly disabled color output
    bool verbose{false};            ///< Enable verbose logging
    std::string project_root{"."};  ///< Root directory of the project
    std::string tileset_name;       ///< Name of the tileset being processed

    /**
     * @brief Determines if styled (ANSI) output should be used.
     *
     * @details
     * Color output is enabled when:
     * - User has not set --no-color flag
     * - AND stderr is connected to a TTY
     *
     * @return True if ANSI styled output should be used, false for plain text
     */
    [[nodiscard]] bool should_use_color() const {
        return !no_color && isatty(STDERR_FILENO);
    }
};

} // namespace porytiles::di
```

### Step 2.2: Foundation Components

**File:** `porytiles/include/porytiles/di/components.hpp`

```C++
#pragma once

#include <fruit/fruit.h>

#include "porytiles/di/cli_config.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/services/primary_tileset_compiler.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tile_validator.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/services/stderr_ascii_tile_printer.hpp"
#include "porytiles/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles::di {

// ============================================================================
// CONDITIONAL FORMATTER COMPONENT
// ============================================================================

/**
 * @brief Component that provides TextFormatter based on runtime configuration.
 *
 * @details
 * This component conditionally binds either AnsiStyledTextFormatter or PlainTextFormatter
 * based on the CLI configuration. This is the primary example of runtime conditional injection.
 *
 * @param config Runtime configuration containing color preferences and TTY detection
 * @return Component providing TextFormatter interface
 */
fruit::Component<TextFormatter> getFormatterComponent(CliConfig config);

// ============================================================================
// FOUNDATION COMPONENTS
// ============================================================================

/**
 * @brief Component providing foundation diagnostic and printing services.
 *
 * @details
 * This component provides UserDiagnostics and TilePrinter implementations.
 * Both depend on TextFormatter, which must be provided by another component.
 *
 * @return Component providing UserDiagnostics and TilePrinter, requiring TextFormatter
 */
fruit::Component<UserDiagnostics, TilePrinter, fruit::Required<TextFormatter>>
getFoundationComponent();

// ============================================================================
// CONFIG COMPONENT
// ============================================================================

/**
 * @brief Component providing DomainConfig implementation.
 *
 * @details
 * Provides the LazyLayeredConfig implementation of DomainConfig.
 *
 * @return Component providing DomainConfig
 */
fruit::Component<DomainConfig> getConfigComponent();

// ============================================================================
// LOADER SERVICES COMPONENT
// ============================================================================

/**
 * @brief Component providing stateless image and palette loader services.
 *
 * @details
 * These loaders are stateless and can be created as needed.
 *
 * @return Component providing all loader services
 */
fruit::Component<PngRgbaImageLoader, PngIndexedImageLoader, JascPalLoader>
getLoaderServicesComponent();

// ============================================================================
// DOMAIN SERVICES COMPONENT
// ============================================================================

/**
 * @brief Component providing primary domain services.
 *
 * @details
 * Provides compiler and validator services. These depend on foundation services
 * and configuration, which must be provided by other components.
 *
 * @return Component providing domain services with their requirements
 */
fruit::Component<
    PrimaryTilesetCompiler,
    TileValidator,
    ColorSetBuilder,
    fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>>
getDomainServicesComponent();

// ============================================================================
// REPOSITORY COMPONENT
// ============================================================================

/**
 * @brief Component providing repository infrastructure.
 *
 * @details
 * Provides TilesetRepo and all its dependencies including artifact readers,
 * writers, and checksum providers.
 *
 * @return Component providing repository infrastructure
 */
fruit::Component<TilesetRepo, fruit::Required<DomainConfig>>
getRepositoryComponent();

// ============================================================================
// APPLICATION COMPONENT (COMPOSITION ROOT)
// ============================================================================

/**
 * @brief Main application component that composes all sub-components.
 *
 * @details
 * This is the composition root that assembles all components based on runtime
 * configuration. It handles conditional injection of TextFormatter and wires
 * all dependencies together.
 *
 * Usage:
 * ```C++
 * CliConfig config{.no_color = false, .tileset_name = "my_tileset"};
 * fruit::Injector<PrimaryTilesetCompiler, TilesetRepo> injector(
 *     getApplicationComponent, config);
 * auto* compiler = injector.get<PrimaryTilesetCompiler*>();
 * ```
 *
 * @param config Runtime configuration determining component bindings
 * @return Component providing all application services
 */
fruit::Component<
    PrimaryTilesetCompiler,
    TilesetRepo,
    TileValidator,
    UserDiagnostics,
    TextFormatter>
getApplicationComponent(CliConfig config);

} // namespace porytiles::di
```

### Step 2.3: Component Implementations

**File:** `porytiles/lib/di/components.cpp`

```C++
#include "porytiles/di/components.hpp"

#include "porytiles/domain/services/color_set_builder.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles/infra/services/jasc_pal_loader.hpp"
#include "porytiles/infra/services/jasc_pal_saver.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/infra/services/project_artifact_checksum_provider.hpp"

namespace porytiles::di {

// ============================================================================
// CONDITIONAL FORMATTER COMPONENT
// ============================================================================

fruit::Component<TextFormatter> getFormatterComponent(CliConfig config)
{
    if (config.should_use_color()) {
        return fruit::createComponent()
            .bind<TextFormatter, AnsiStyledTextFormatter>();
    } else {
        return fruit::createComponent()
            .bind<TextFormatter, PlainTextFormatter>();
    }
}

// ============================================================================
// FOUNDATION COMPONENTS
// ============================================================================

fruit::Component<UserDiagnostics, TilePrinter, fruit::Required<TextFormatter>>
getFoundationComponent()
{
    return fruit::createComponent()
        .bind<UserDiagnostics, StderrStyledUserDiagnostics>()
        .bind<TilePrinter, StderrAsciiTilePrinter>();
}

// ============================================================================
// CONFIG COMPONENT
// ============================================================================

fruit::Component<DomainConfig> getConfigComponent()
{
    return fruit::createComponent()
        .bind<DomainConfig, LazyLayeredConfig>();
}

// ============================================================================
// LOADER SERVICES COMPONENT
// ============================================================================

fruit::Component<PngRgbaImageLoader, PngIndexedImageLoader, JascPalLoader>
getLoaderServicesComponent()
{
    return fruit::createComponent()
        .registerConstructor<PngRgbaImageLoader()>()
        .registerConstructor<PngIndexedImageLoader()>()
        .registerConstructor<JascPalLoader()>();
}

// ============================================================================
// DOMAIN SERVICES COMPONENT
// ============================================================================

fruit::Component<
    PrimaryTilesetCompiler,
    TileValidator,
    ColorSetBuilder,
    fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>>
getDomainServicesComponent()
{
    return fruit::createComponent()
        .registerConstructor<PrimaryTilesetCompiler(
            DomainConfig*,
            TextFormatter*,
            UserDiagnostics*,
            TilePrinter*)>()
        .registerConstructor<TileValidator(
            TextFormatter*,
            UserDiagnostics*,
            TilePrinter*)>()
        .registerConstructor<ColorSetBuilder(TextFormatter*)>();
}

// ============================================================================
// REPOSITORY COMPONENT
// ============================================================================

fruit::Component<TilesetRepo, fruit::Required<DomainConfig>>
getRepositoryComponent()
{
    return fruit::createComponent()
        .registerConstructor<ProjectTilesetArtifactKeyProvider()>()
        .bind<ArtifactChecksumProvider, ProjectArtifactChecksumProvider>()
        .registerConstructor<ProjectArtifactChecksumProvider(
            ProjectTilesetArtifactKeyProvider*)>()
        .registerConstructor<ProjectTilesetArtifactReader(
            PngRgbaImageLoader*,
            PngIndexedImageLoader*,
            JascPalLoader*)>()
        .registerConstructor<ProjectTilesetArtifactWriter(
            DomainConfig*,
            const std::string&,  // project_root
            PngRgbaImageSaver*,
            PngIndexedImageSaver*,
            JascPalSaver*)>()
        .registerConstructor<TilesetRepo(
            ArtifactChecksumProvider*,
            ProjectTilesetArtifactKeyProvider*,
            ProjectTilesetArtifactReader*,
            ProjectTilesetArtifactWriter*)>()
        .install(getLoaderServicesComponent);
}

// ============================================================================
// APPLICATION COMPONENT (COMPOSITION ROOT)
// ============================================================================

fruit::Component<
    PrimaryTilesetCompiler,
    TilesetRepo,
    TileValidator,
    UserDiagnostics,
    TextFormatter>
getApplicationComponent(CliConfig config)
{
    return fruit::createComponent()
        .install(getFormatterComponent, config)  // Conditional based on config
        .install(getFoundationComponent)
        .install(getConfigComponent)
        .install(getDomainServicesComponent)
        .install(getRepositoryComponent);
}

} // namespace porytiles::di
```

---

## Phase 3: Migration Strategy

### Migration Approach: Incremental & Safe

**Goal:** Migrate without breaking existing functionality. Test at each step.

### Step 3.1: Set Up Fruit (No Code Changes)

1. Add Fruit to CMakeLists.txt
2. Verify project builds successfully
3. Run all tests to ensure no regressions

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j7
./build/porytiles/tests/PorytilesAllTests
```

### Step 3.2: Create DI Infrastructure (Parallel to Existing Code)

1. Create `porytiles/include/porytiles/di/` directory
2. Add `cli_config.hpp`, `components.hpp`
3. Create `porytiles/lib/di/` directory
4. Add `components.cpp`
5. Update CMakeLists.txt to include new source files

**At this point:** Existing code unchanged, DI components exist but unused.

### Step 3.3: Migrate One Command at a Time

Start with `DebugPrimaryCompileCommand` since it's a good example.

**Before (debug_commands.hpp:42-104):**
```C++
void Run() override {
    // Manual DI - verbose and error-prone
    std::unique_ptr<TextFormatter> text_formatter = std::make_unique<AnsiStyledTextFormatter>();
    std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter.get());
    std::unique_ptr<TilePrinter> tile_printer = std::make_unique<StderrAsciiTilePrinter>(text_formatter.get());

    std::vector<std::unique_ptr<ConfigProvider>> providers{};
    providers.push_back(std::make_unique<DefaultProvider>());
    LazyLayeredConfig config{text_formatter.get(), std::move(providers)};

    PngRgbaImageLoader png_rgba_loader{};
    PngIndexedImageLoader png_indexed_loader{};
    // ... many more manual initializations ...

    PrimaryTilesetCompiler compiler{&config, text_formatter.get(), diag.get(), tile_printer.get()};
    // ... rest of code
}
```

**After (using Fruit DI):**
```C++
#include "porytiles/di/components.hpp"

void Run() override {
    using namespace porytiles::di;

    // Build runtime configuration
    CliConfig config{
        .no_color = no_color_flag_,        // From CLI11 parsing
        .verbose = verbose_flag_,
        .project_root = ".",
        .tileset_name = tileset_name_
    };

    // Create injector with all dependencies wired automatically
    fruit::Injector<PrimaryTilesetCompiler, TilesetRepo, UserDiagnostics> injector(
        getApplicationComponent, config);

    // Retrieve services - all dependencies resolved automatically
    auto* compiler = injector.get<PrimaryTilesetCompiler*>();
    auto* repo = injector.get<TilesetRepo*>();
    auto* diag = injector.get<UserDiagnostics*>();

    // Use services normally - rest of logic unchanged
    auto maybe_tileset = repo->load(tileset_name_);
    if (!maybe_tileset.has_value()) {
        diag->fatal(maybe_tileset);
        return;
    }

    auto compile_result = compiler->compile(*maybe_tileset.value());
    // ... rest unchanged
}
```

**Benefits Achieved:**
- ✅ 40+ lines of manual DI → 10 lines
- ✅ Runtime conditional injection (color based on TTY)
- ✅ All dependencies wired automatically
- ✅ Easy to add new dependencies without touching command code

### Step 3.4: Add CLI Flags for Color Control

**File:** `porytiles/tools/driver/debug_commands.hpp`

Update the command to accept `--no-color` flag:

```C++
class DebugPrimaryCompileCommand final : public Command {
public:
    explicit DebugPrimaryCompileCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
        cmd.add_flag("--no-color", no_color_flag_, "Disable colored output");
        cmd.add_flag("-v,--verbose", verbose_flag_, "Enable verbose output");
    }

    void Run() override {
        // ... use no_color_flag_ in CliConfig
    }

private:
    static constexpr auto kCommandName = "debug-compile-primary";
    static constexpr auto kCommandDesc = "...";
    static constexpr auto kCommandGroup = "COMMANDS";

    std::string tileset_name_;
    bool no_color_flag_{false};
    bool verbose_flag_{false};
};
```

### Step 3.5: Testing Strategy per Step

After each migration step:

1. **Build verification:**
   ```bash
   cmake --build build -j7 > /tmp/build.log 2>&1
   echo $?  # Should be 0
   ```

2. **Run tests:**
   ```bash
   ./build/porytiles/tests/PorytilesAllTests
   ```

3. **Manual functional test:**
   ```bash
   # Test with color (TTY)
   ./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset

   # Test without color (flag)
   ./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset --no-color

   # Test without color (redirect)
   ./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset 2> /tmp/out.txt
   cat /tmp/out.txt  # Should not contain ANSI codes
   ```

4. **Verify output:**
   - Check that TTY output has colors
   - Check that `--no-color` disables colors
   - Check that redirected stderr has no ANSI codes

---

## Phase 4: Advanced Patterns

### Pattern 1: Factory for Services with Runtime Parameters

**Problem:** `MetatileDecompiler` needs `tileset_name` at construction, which is only known at runtime.

**Solution:** Use assisted injection with factory pattern.

**File:** `porytiles/include/porytiles/di/components.hpp` (addition)

```C++
// Add to components.hpp:

/**
 * @brief Factory for creating MetatileDecompiler with runtime tileset name.
 *
 * @details
 * Since MetatileDecompiler requires a runtime parameter (tileset_name) that cannot be
 * known at injector creation time, we use a factory pattern with assisted injection.
 */
class MetatileDecompilerFactory {
public:
    // Fruit will inject these dependencies
    MetatileDecompilerFactory(
        DomainConfig* config,
        TextFormatter* format,
        UserDiagnostics* diag,
        TilePrinter* tile_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}
    {
    }

    /**
     * @brief Creates a MetatileDecompiler with the given runtime tileset name.
     *
     * @param tileset_name The name of the tileset to decompile
     * @return A unique_ptr to the created MetatileDecompiler
     */
    [[nodiscard]] std::unique_ptr<MetatileDecompiler> create(std::string tileset_name) const {
        return std::make_unique<MetatileDecompiler>(
            std::move(tileset_name),
            config_,
            format_,
            diag_,
            tile_printer_);
    }

private:
    DomainConfig* config_;
    TextFormatter* format_;
    UserDiagnostics* diag_;
    TilePrinter* tile_printer_;
};

// Add component for factory
fruit::Component<MetatileDecompilerFactory,
                 fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>>
getMetatileDecompilerFactoryComponent();
```

**File:** `porytiles/lib/di/components.cpp` (addition)

```C++
fruit::Component<MetatileDecompilerFactory,
                 fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>>
getMetatileDecompilerFactoryComponent()
{
    return fruit::createComponent()
        .registerConstructor<MetatileDecompilerFactory(
            DomainConfig*,
            TextFormatter*,
            UserDiagnostics*,
            TilePrinter*)>();
}
```

**Usage:**
```C++
// In your command
fruit::Injector<MetatileDecompilerFactory> injector(
    getApplicationComponent, config,
    getMetatileDecompilerFactoryComponent);

auto* factory = injector.get<MetatileDecompilerFactory*>();

// Create decompiler with runtime tileset name
auto decompiler = factory->create("my_tileset");
decompiler->decompile_metatiles(/* ... */);
```

### Pattern 2: Multi-Binding for Plugin Architecture

**Use Case:** If you want multiple `AssetGenerator` implementations to be collected automatically.

```C++
// In components.hpp
fruit::Component<std::vector<std::unique_ptr<AssetGenerator>>>
getAssetGeneratorsComponent();

// In components.cpp
fruit::Component<std::vector<std::unique_ptr<AssetGenerator>>>
getAssetGeneratorsComponent()
{
    return fruit::createComponent()
        .addMultibinding<AssetGenerator, TileAssetGenerator>()
        .addMultibinding<AssetGenerator, PaletteAssetGenerator>()
        .addMultibinding<AssetGenerator, MetatileAssetGenerator>();
}

// Usage
fruit::Injector<std::vector<std::unique_ptr<AssetGenerator>>> injector(...);
std::vector<std::unique_ptr<AssetGenerator>> generators =
    injector.get<std::vector<std::unique_ptr<AssetGenerator>>>();

// Process all generators
for (const auto& generator : generators) {
    auto result = generator->generate();
    // ... handle result
}
```

### Pattern 3: Conditional Multi-Component Selection

**Use Case:** Select different sets of components based on compilation mode.

```C++
fruit::Component<CompilerServices> getCompilerServicesComponent(CompilationMode mode)
{
    if (mode == CompilationMode::Debug) {
        return fruit::createComponent()
            .bind<Optimizer, DebugOptimizer>()           // No optimizations
            .bind<Validator, StrictValidator>()          // Strict validation
            .install(getLoggingComponent, LogLevel::Verbose);
    } else {
        return fruit::createComponent()
            .bind<Optimizer, ReleaseOptimizer>()         // Full optimizations
            .bind<Validator, StandardValidator>()        // Standard validation
            .install(getLoggingComponent, LogLevel::Error);
    }
}
```

---

## Phase 5: Implementation Checklist

### Checklist Format
- [ ] Task description
  - **Files affected:** List of files
  - **Estimated complexity:** Low/Medium/High
  - **Testing requirements:** What to test

---

### Phase 5.1: Setup Tasks

- [ ] **Add Fruit dependency to CMake**
  - **Files:** `CMakeLists.txt` or `porytiles/CMakeLists.txt`
  - **Complexity:** Low
  - **Testing:** Build succeeds, no regressions

- [ ] **Create DI directory structure**
  - **Files:** New directories: `include/porytiles/di/`, `lib/di/`
  - **Complexity:** Low
  - **Testing:** Directories exist

- [ ] **Create cli_config.hpp**
  - **Files:** `porytiles/include/porytiles/di/cli_config.hpp`
  - **Complexity:** Low
  - **Testing:** Compiles, TTY detection works

- [ ] **Create components.hpp**
  - **Files:** `porytiles/include/porytiles/di/components.hpp`
  - **Complexity:** Medium
  - **Testing:** Compiles with Fruit includes

- [ ] **Implement components.cpp**
  - **Files:** `porytiles/lib/di/components.cpp`
  - **Complexity:** High
  - **Testing:** All components compile and link

- [ ] **Update CMakeLists.txt to include DI sources**
  - **Files:** `porytiles/CMakeLists.txt`
  - **Complexity:** Low
  - **Testing:** Build succeeds

---

### Phase 5.2: Service Migration Tasks

- [ ] **Verify all service constructors are Fruit-compatible**
  - **Files:** All service headers in `domain/services/`
  - **Complexity:** Low
  - **Testing:** Check that constructors use raw pointers, not unique_ptr

- [ ] **Add necessary #includes for Fruit**
  - **Files:** `components.cpp`, potentially service headers
  - **Complexity:** Low
  - **Testing:** Compiles without errors

- [ ] **Test component registration for each service**
  - **Files:** `components.cpp`
  - **Complexity:** Medium
  - **Testing:** Can create injector and resolve each service

---

### Phase 5.3: Command Migration Tasks

- [ ] **Migrate DebugPrimaryCompileCommand**
  - **Files:** `tools/driver/debug_commands.hpp`
  - **Complexity:** Medium
  - **Testing:**
    - Command runs successfully
    - Color output works on TTY
    - `--no-color` flag disables colors
    - Redirected output has no ANSI codes

- [ ] **Add --no-color flag to all commands**
  - **Files:** `tools/driver/command.hpp` (base class), or individual commands
  - **Complexity:** Low
  - **Testing:** Flag accepted and respected by all commands

- [ ] **Migrate CreateTilesetCommand**
  - **Files:** `tools/driver/create_tileset_command.hpp`
  - **Complexity:** Medium
  - **Testing:** Full tileset creation flow works

- [ ] **Migrate VerifyTilesetCommand**
  - **Files:** `tools/driver/verify_tileset_command.hpp`
  - **Complexity:** Medium
  - **Testing:** Verification logic works correctly

---

### Phase 5.4: Advanced Feature Tasks

- [ ] **Implement MetatileDecompilerFactory**
  - **Files:** `di/components.hpp`, `di/components.cpp`
  - **Complexity:** Medium
  - **Testing:** Factory creates decompilers with correct dependencies

- [ ] **Add component for factory to application component**
  - **Files:** `di/components.cpp`
  - **Complexity:** Low
  - **Testing:** Factory can be injected and used

- [ ] **Document factory pattern usage**
  - **Files:** This file, code comments
  - **Complexity:** Low
  - **Testing:** Documentation is clear

---

### Phase 5.5: Testing & Verification Tasks

- [ ] **Create DI integration test**
  - **Files:** New test file `tests/integration/di_integration_test.cpp`
  - **Complexity:** Medium
  - **Testing:** Verify injector creates correct services

- [ ] **Test conditional injection with TTY**
  - **Files:** Integration test
  - **Complexity:** Medium
  - **Testing:** Mock TTY detection, verify correct formatter

- [ ] **Test all CLI commands with DI**
  - **Files:** Existing command tests
  - **Complexity:** Medium
  - **Testing:** All commands work with DI wiring

- [ ] **Verify no performance regressions**
  - **Files:** Performance benchmarks
  - **Complexity:** High
  - **Testing:** Compare runtime before/after DI

---

## Code Examples

### Example 1: Complete Command Migration

**Before:**
```C++
// porytiles/tools/driver/debug_commands.hpp (old)
class DebugPrimaryCompileCommand final : public Command {
public:
    explicit DebugPrimaryCompileCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
    }

    void Run() override {
        using namespace porytiles;

        // 40+ lines of manual dependency wiring
        std::unique_ptr<TextFormatter> text_formatter = std::make_unique<AnsiStyledTextFormatter>();
        std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter.get());
        std::unique_ptr<TilePrinter> tile_printer = std::make_unique<StderrAsciiTilePrinter>(text_formatter.get());

        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter.get(), std::move(providers)};

        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        PngRgbaImageSaver png_rgba_saver{};
        PngIndexedImageSaver png_indexed_saver{};
        JascPalLoader jasc_loader{};
        JascPalSaver jasc_saver{};

        PrimaryTilesetCompiler compiler{&config, text_formatter.get(), diag.get(), tile_printer.get()};

        ProjectTilesetArtifactReader artifact_reader{&png_rgba_loader, &png_indexed_loader, &jasc_loader};
        ProjectTilesetArtifactWriter artifact_writer{&config, ".", &png_rgba_saver, &png_indexed_saver, &jasc_saver};
        ProjectTilesetArtifactKeyProvider key_provider{"."};
        ProjectArtifactChecksumProvider checksum_provider{&key_provider};
        TilesetRepo repo{&checksum_provider, &key_provider, &artifact_reader, &artifact_writer};

        // Business logic
        auto maybe_tileset = repo.load(tileset_name_);
        if (!maybe_tileset.has_value()) {
            diag->fatal(maybe_tileset);
            return;
        }

        const auto tileset = std::move(maybe_tileset.value());
        auto compile_result = compiler.compile(*tileset);
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to compile tileset '{}'", FormatParam{tileset_name_, Style::bold}},
                compile_result};
            diag->fatal(fail_result);
            return;
        }

        const auto new_tileset = std::move(compile_result.value());
        const auto new_tileset_save_result = repo.save(*new_tileset);
        if (!new_tileset_save_result.has_value()) {
            diag->fatal(new_tileset_save_result);
            return;
        }
    }

private:
    static constexpr auto kCommandName = "debug-compile-primary";
    static constexpr auto kCommandDesc = "...";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
```

**After:**
```C++
// porytiles/tools/driver/debug_commands.hpp (new)
#include "porytiles/di/components.hpp"

class DebugPrimaryCompileCommand final : public Command {
public:
    explicit DebugPrimaryCompileCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
        cmd.add_flag("--no-color", no_color_flag_, "Disable colored output");
        cmd.add_flag("-v,--verbose", verbose_flag_, "Enable verbose output");
    }

    void Run() override {
        using namespace porytiles;
        using namespace porytiles::di;

        // 10 lines of DI setup - all dependencies wired automatically
        CliConfig config{
            .no_color = no_color_flag_,
            .verbose = verbose_flag_,
            .project_root = ".",
            .tileset_name = tileset_name_
        };

        fruit::Injector<PrimaryTilesetCompiler, TilesetRepo, UserDiagnostics> injector(
            getApplicationComponent, config);

        auto* compiler = injector.get<PrimaryTilesetCompiler*>();
        auto* repo = injector.get<TilesetRepo*>();
        auto* diag = injector.get<UserDiagnostics*>();

        // Business logic - UNCHANGED
        auto maybe_tileset = repo->load(tileset_name_);
        if (!maybe_tileset.has_value()) {
            diag->fatal(maybe_tileset);
            return;
        }

        const auto tileset = std::move(maybe_tileset.value());
        auto compile_result = compiler->compile(*tileset);
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to compile tileset '{}'", FormatParam{tileset_name_, Style::bold}},
                compile_result};
            diag->fatal(fail_result);
            return;
        }

        const auto new_tileset = std::move(compile_result.value());
        const auto new_tileset_save_result = repo->save(*new_tileset);
        if (!new_tileset_save_result.has_value()) {
            diag->fatal(new_tileset_save_result);
            return;
        }
    }

private:
    static constexpr auto kCommandName = "debug-compile-primary";
    static constexpr auto kCommandDesc = "...";
    static constexpr auto kCommandGroup = "COMMANDS";

    std::string tileset_name_;
    bool no_color_flag_{false};
    bool verbose_flag_{false};
};
```

**Key Improvements:**
- ✅ Reduced from ~60 lines to ~30 lines
- ✅ Runtime conditional injection (AnsiStyledTextFormatter vs PlainTextFormatter)
- ✅ All dependencies resolved automatically
- ✅ Easy to add new services without touching command code
- ✅ Business logic completely unchanged

---

### Example 2: Testing DI Configuration

**File:** `porytiles/tests/integration/di_integration_test.cpp`

```C++
#include <gtest/gtest.h>

#include "porytiles/di/components.hpp"
#include "porytiles/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;
using namespace porytiles::di;

class DiIntegrationTest : public ::testing::Test {
protected:
    // Common test configuration
    CliConfig color_config{
        .no_color = false,  // Color enabled
        .verbose = false,
        .project_root = ".",
        .tileset_name = "test_tileset"
    };

    CliConfig no_color_config{
        .no_color = true,   // Color disabled
        .verbose = false,
        .project_root = ".",
        .tileset_name = "test_tileset"
    };
};

TEST_F(DiIntegrationTest, ShouldInjectAnsiFormatterWhenColorEnabled)
{
    // Arrange
    fruit::Injector<TextFormatter> injector(getFormatterComponent, color_config);

    // Act
    TextFormatter* formatter = injector.get<TextFormatter*>();

    // Assert
    ASSERT_NE(formatter, nullptr);

    // Verify it's actually AnsiStyledTextFormatter by checking behavior
    std::string styled = formatter->style("test", Style::bold);
    EXPECT_TRUE(styled.find("\033[") != std::string::npos)  // Contains ANSI codes
        << "Expected ANSI codes in styled text";
}

TEST_F(DiIntegrationTest, ShouldInjectPlainFormatterWhenColorDisabled)
{
    // Arrange
    fruit::Injector<TextFormatter> injector(getFormatterComponent, no_color_config);

    // Act
    TextFormatter* formatter = injector.get<TextFormatter*>();

    // Assert
    ASSERT_NE(formatter, nullptr);

    // Verify it's PlainTextFormatter by checking behavior
    std::string styled = formatter->style("test", Style::bold);
    EXPECT_EQ(styled, "test")  // No ANSI codes, just plain text
        << "Expected plain text without ANSI codes";
}

TEST_F(DiIntegrationTest, ShouldWireCompilerWithAllDependencies)
{
    // Arrange
    fruit::Injector<PrimaryTilesetCompiler, UserDiagnostics> injector(
        getApplicationComponent, color_config);

    // Act
    auto* compiler = injector.get<PrimaryTilesetCompiler*>();
    auto* diag = injector.get<UserDiagnostics*>();

    // Assert
    ASSERT_NE(compiler, nullptr) << "Compiler should be injected";
    ASSERT_NE(diag, nullptr) << "Diagnostics should be injected";

    // Verify dependencies are wired correctly (compiler should not crash when used)
    // This is more of a smoke test - actual functionality tested elsewhere
}

TEST_F(DiIntegrationTest, ShouldCreateMultipleInjectorsIndependently)
{
    // Arrange & Act
    fruit::Injector<TextFormatter> injector1(getFormatterComponent, color_config);
    fruit::Injector<TextFormatter> injector2(getFormatterComponent, no_color_config);

    TextFormatter* formatter1 = injector1.get<TextFormatter*>();
    TextFormatter* formatter2 = injector2.get<TextFormatter*>();

    // Assert - different configurations produce different behaviors
    std::string styled1 = formatter1->style("test", Style::bold);
    std::string styled2 = formatter2->style("test", Style::bold);

    EXPECT_NE(styled1, styled2)
        << "Different configurations should produce different formatters";
}

TEST_F(DiIntegrationTest, ShouldHandleDependencyChain)
{
    // Arrange - UserDiagnostics depends on TextFormatter, both should be wired
    fruit::Injector<UserDiagnostics, TextFormatter> injector(
        getApplicationComponent, color_config);

    // Act
    auto* diag = injector.get<UserDiagnostics*>();
    auto* formatter = injector.get<TextFormatter*>();

    // Assert
    ASSERT_NE(diag, nullptr);
    ASSERT_NE(formatter, nullptr);

    // Verify that diag uses the injected formatter (would crash if not wired correctly)
    // This is tested by actually using the diagnostics
    diag->note("Test note");  // Should not crash
}
```

---

## Testing Strategy

### Unit Tests for Components

**Test each component independently:**

1. **Formatter Component:**
   - Verify AnsiStyledTextFormatter is injected when color enabled
   - Verify PlainTextFormatter is injected when color disabled
   - Verify TTY detection logic in `should_use_color()`

2. **Foundation Component:**
   - Verify UserDiagnostics and TilePrinter are created
   - Verify they receive TextFormatter dependency

3. **Application Component:**
   - Verify full dependency graph can be resolved
   - Verify all required services can be injected

### Integration Tests

**Test command execution with DI:**

```bash
# Test script: test_di_integration.sh

#!/bin/bash
set -e

# Build the project
cmake --build build -j7

# Test 1: Color output on TTY
echo "Test 1: Color output on TTY"
./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset 2>&1 | grep -q "\033\[" && echo "PASS: ANSI codes present" || echo "FAIL: No ANSI codes"

# Test 2: No color with flag
echo "Test 2: No color with --no-color flag"
./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset --no-color 2>&1 | grep -q "\033\[" && echo "FAIL: ANSI codes present" || echo "PASS: No ANSI codes"

# Test 3: No color when redirected
echo "Test 3: No color when redirected to file"
./build/porytiles/tools/driver/porytiles debug-compile-primary test_tileset 2> /tmp/porytiles_output.txt
grep -q "\033\[" /tmp/porytiles_output.txt && echo "FAIL: ANSI codes in file" || echo "PASS: No ANSI codes in file"

echo "All DI integration tests completed"
```

### Performance Tests

**Verify DI doesn't add significant overhead:**

```C++
// Benchmark: DI creation overhead
#include <benchmark/benchmark.h>
#include "porytiles/di/components.hpp"

static void BM_InjectorCreation(benchmark::State& state)
{
    using namespace porytiles::di;

    CliConfig config{.tileset_name = "test"};

    for (auto _ : state) {
        fruit::Injector<PrimaryTilesetCompiler> injector(
            getApplicationComponent, config);
        auto* compiler = injector.get<PrimaryTilesetCompiler*>();
        benchmark::DoNotOptimize(compiler);
    }
}
BENCHMARK(BM_InjectorCreation);

static void BM_ServiceResolution(benchmark::State& state)
{
    using namespace porytiles::di;

    CliConfig config{.tileset_name = "test"};
    fruit::Injector<PrimaryTilesetCompiler> injector(
        getApplicationComponent, config);

    for (auto _ : state) {
        auto* compiler = injector.get<PrimaryTilesetCompiler*>();
        benchmark::DoNotOptimize(compiler);
    }
}
BENCHMARK(BM_ServiceResolution);
```

---

## Troubleshooting

### Common Issues & Solutions

#### Issue 1: "No matching constructor found"

**Error:**
```
error: No constructor was specified for type porytiles::PrimaryTilesetCompiler, ...
```

**Solution:**
You need to register the constructor explicitly in your component:

```C++
// In components.cpp
fruit::Component<PrimaryTilesetCompiler, fruit::Required<...>>
getDomainServicesComponent()
{
    return fruit::createComponent()
        .registerConstructor<PrimaryTilesetCompiler(
            DomainConfig*,
            TextFormatter*,
            UserDiagnostics*,
            TilePrinter*)>();  // Must match actual constructor signature
}
```

#### Issue 2: "Required type not provided"

**Error:**
```
error: The required type TextFormatter is not provided by this component
```

**Solution:**
The component has a `fruit::Required<TextFormatter>` but you didn't install a component that provides it:

```C++
// Make sure to install the formatter component
fruit::Component<...> getApplicationComponent(CliConfig config)
{
    return fruit::createComponent()
        .install(getFormatterComponent, config)  // Provides TextFormatter
        .install(getFoundationComponent);         // Requires TextFormatter
}
```

#### Issue 3: Circular Dependency

**Error:**
```
error: Circular dependency detected: A -> B -> A
```

**Solution:**
Break the circular dependency by:
1. Introducing an interface that both depend on
2. Restructuring the dependency graph
3. Using lazy injection if one dependency is optional

#### Issue 4: Multiple Definitions for Same Type

**Error:**
```
error: Type TextFormatter is provided by multiple components
```

**Solution:**
Ensure each type is bound only once in the component hierarchy. Use `fruit::Required<>` to declare dependencies instead of binding them again:

```C++
// WRONG - binds TextFormatter twice
fruit::Component<UserDiagnostics> getBadComponent() {
    return fruit::createComponent()
        .bind<TextFormatter, AnsiStyledTextFormatter>()  // Binds here
        .install(getOtherComponent);  // Also binds TextFormatter - ERROR!
}

// CORRECT - declares requirement
fruit::Component<UserDiagnostics, fruit::Required<TextFormatter>> getGoodComponent() {
    return fruit::createComponent()
        .bind<UserDiagnostics, StderrStyledUserDiagnostics>();
        // TextFormatter provided by parent component
}
```

#### Issue 5: gsl::not_null Compatibility

**Issue:**
Service constructors use `gsl::not_null<T*>` but Fruit provides `T*`.

**Solution:**
Fruit works with raw pointers. The `gsl::not_null` is just a wrapper, so you can either:

1. **Keep gsl::not_null (recommended):** Fruit will inject `T*`, constructor converts to `gsl::not_null<T*>`
   ```C++
   // Constructor with gsl::not_null - works fine
   PrimaryTilesetCompiler(
       gsl::not_null<DomainConfig*> config,
       gsl::not_null<TextFormatter*> format)
       : config_{config}, format_{format} {}

   // Register with raw pointers - Fruit handles conversion
   .registerConstructor<PrimaryTilesetCompiler(DomainConfig*, TextFormatter*)>()
   ```

2. **Use raw pointers in registration:** No changes needed to service code

---

## Summary

### What We've Achieved

1. ✅ **Runtime conditional injection** - Choose implementations based on CLI flags and environment
2. ✅ **Reduced boilerplate** - ~40 lines of manual DI → ~10 lines
3. ✅ **Scalable architecture** - Easy to add new services without touching command code
4. ✅ **Testable components** - Each component can be tested independently
5. ✅ **Modular design** - Components compose cleanly for different configurations

### Next Steps

1. **Implement Phase 1** - Set up Fruit and create DI infrastructure
2. **Migrate one command** - Start with `DebugPrimaryCompileCommand`
3. **Verify functionality** - Test color output, CLI flags, and core features
4. **Iterate** - Migrate remaining commands one at a time
5. **Add advanced patterns** - Factories, multi-bindings as needed

### Key Takeaways

- **Components are composable** - Small, focused components combine to form the application
- **Runtime flexibility** - Fruit excels at runtime conditional injection
- **Type safety maintained** - Fruit provides compile-time verification of dependency graphs
- **Minimal code changes** - Services remain unchanged, only DI wiring moves to components
- **Testability improved** - Each component and service can be tested in isolation

---

## References

- [Google Fruit Documentation](https://github.com/google/fruit/wiki)
- [Fruit Tutorial](https://github.com/google/fruit/wiki/tutorial)
- [Dependency Injection in C++](https://martinfowler.com/articles/injection.html)
- [Modern C++ Dependency Injection](https://www.boost.org/doc/libs/1_84_0/libs/di/doc/html/index.html)

---

**End of Document**
