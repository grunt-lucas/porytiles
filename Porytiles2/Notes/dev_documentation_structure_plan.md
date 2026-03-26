# Porytiles Developer Documentation Structure Plan

## Context

Porytiles2 is approaching a state where developer/contributor documentation is needed. The existing `porytiles-dev-docs` repo has a Sphinx + Read the Docs theme + MyST parser setup with four placeholder stub pages (`getting-started.md`, `architecture.md`, `contributing.md`, `reference.md`) and a formatting `testbed.md`. This plan defines the full documentation structure: 19 pages organized into 5 sections, covering everything from environment setup to CI/CD. The structure follows the [Diataxis framework](https://diataxis.fr/): tutorials (Getting Started), explanation (Architecture + Core Systems), how-to guides (How-To Guides), and reference (Reference).

The target audience is a **C++ developer who may be new to the Porytiles codebase** but is not new to programming. The goal is "what you need to know to contribute effectively" -- not an exhaustive API reference for every class.

The in-repo `Porytiles2/ARCHITECTURE.md` (~665 lines) provides a detailed codemap and layer descriptions. The dev docs site **complements** that document with deeper dives, worked examples, data flow walkthroughs, and contributor recipes. Some conceptual overlap is expected -- the in-repo doc serves as a quick reference for repo browsers, while the dev docs site provides a proper tutorial/guide reading experience.

The docs use **Sphinx + Read the Docs theme + MyST parser** (markdown files). The tone should be direct and technical with code snippets and ASCII diagrams where they add clarity.

---

## Documentation Structure

### Toctree (in `index.rst`)

```rst
.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   dev-environment-setup

.. toctree::
   :maxdepth: 2
   :caption: Architecture

   project-layout
   layered-architecture
   data-flow-and-pipelines
   dependency-injection
   cpp-features-and-patterns

.. toctree::
   :maxdepth: 2
   :caption: Core Systems

   config-generation-system
   error-handling-and-diagnostics
   c-parser

.. toctree::
   :maxdepth: 2
   :caption: How-To Guides

   adding-a-config-value
   adding-a-command
   adding-a-packing-strategy
   adding-a-di-managed-service
   writing-tests

.. toctree::
   :maxdepth: 2
   :caption: Reference

   build-and-test
   external-dependencies
   scripts-and-tooling
   ci-cd
   glossary

.. toctree::
   :hidden:

   testbed
```

### Files to delete
- `docsrc/getting-started.md` (stub, replaced by `dev-environment-setup.md`)
- `docsrc/architecture.md` (stub, replaced by `layered-architecture.md`)
- `docsrc/contributing.md` (stub, replaced by how-to guides section)
- `docsrc/reference.md` (stub, replaced by reference section)

### Files to preserve
- `docsrc/testbed.md` -- formatting reference, placed in hidden toctree so it remains accessible but does not clutter the main navigation.

---

## Page-by-Page Breakdown

### Section 1: Getting Started (Diataxis: Tutorials)

#### 1. `dev-environment-setup.md` -- Setting Up Your Development Environment

**Diataxis type: Tutorial.** Learning-oriented, takes the contributor from zero to a working build.

The first page any new contributor reads. Purely procedural, gets them from zero to a working build.

- Prerequisites: C++23 compiler (Clang 17+ recommended, GCC 14+), CMake 3.20+, `uv` (Python package manager for code generation and scripts)
- Platform-specific notes: macOS (Homebrew Clang), Linux (system Clang or GCC)
- Cloning the repo
- First build walkthrough: `cmake -S . -B porytiles-build-debug -DCMAKE_BUILD_TYPE=Debug` then `cmake --build porytiles-build-debug -j7`
- Emphasis on the build directory name convention (`porytiles-build-debug`, **never** `build`)
- Running the full test suite to verify the build: `./porytiles-build-debug/Porytiles2/tests/Porytiles2AllTests`
- Installing the built executable: `cmake --install porytiles-build-debug --prefix ~/.local`
- Editor/IDE setup tips: clangd configuration, `compile_commands.json` symlink
- Troubleshooting common first-build issues (missing `uv`, wrong compiler version, FetchContent download failures)

**Cross-references:** forward to `project-layout.md` for orientation, `build-and-test.md` for full build details

---

### Section 2: Architecture (Diataxis: Explanation)

#### 2. `project-layout.md` -- Project Layout and Directory Structure

**Diataxis type: Explanation.** Understanding-oriented — gives the contributor a mental map, not a procedural task.

Orientation page. After building, the contributor needs a mental map of what lives where.

- Top-level directory layout: `Porytiles2/` (main source), `Scripts/`, `Resources/`, `.github/workflows/`, `porytiles-dev-docs/`, `porytiles-user-docs/`
- Inside `Porytiles2/`: `include/porytiles2/`, `lib/`, `tools/driver/`, `tests/`, `config_templates/`, `Notes/`
- The include/lib split: headers in `include/porytiles2/<layer>/`, implementations in `lib/<layer>/`
- Layer directories at a glance: `domain/`, `app/`, `infra/`, `xcut/`, `utilities/` -- one-sentence description each (full details on the architecture page)
- `tools/driver/`: the CLI executable entry point
- `config_templates/`: Jinja2 templates and `config_schema.yaml` for code generation
- `tests/unit/` vs `tests/integration/` vs `tests/support/`
- `Notes/`: internal design decision documents (not user-facing, but valuable for understanding rationale)
- `Porytiles2/ARCHITECTURE.md`: the in-repo architecture overview (the dev docs site expands on it with tutorials and recipes)
- `STYLE.md`: code style guide (the authoritative reference for naming, formatting, and idioms -- dev docs reference it, not duplicate it)
- `Resources/`: test assets and example files used by integration tests

**Cross-references:** forward to `layered-architecture.md` for DDD layer details, `config-generation-system.md` for config_templates, `scripts-and-tooling.md` for Scripts/

---

#### 3. `layered-architecture.md` -- Layered Architecture and DDD

The core architectural reference. Complements `Porytiles2/ARCHITECTURE.md` with richer diagrams, decision frameworks, and worked examples of "where does this code go?"

- The five layers and their responsibilities (brief -- `ARCHITECTURE.md` has the detailed codemap):
  - **utilities**: zero-dependency helpers (result types, text formatting, C parser, string utils)
  - **xcut**: cross-cutting concerns (config value wrapper, diagnostics interface, DI wiring, validators)
  - **domain**: pure business logic (models, algorithms, services, repositories as abstract interfaces, packing subsystem)
  - **app**: use case orchestration (compile, decompile, create, import tileset)
  - **infra**: I/O adapters and concrete implementations (file readers/writers, YAML parsing, CLI, header parsing)
- The strict dependency rule with ASCII diagram
- **"Where does my code go?" decision framework** -- the main value-add over `ARCHITECTURE.md`:
  - Pure data transformation with 1-2 params? -> `domain/algorithms/` free function
  - Multi-step operation with 3+ deps? -> `domain/services/` class
  - Orchestrates domain services for a user goal? -> `app/use_cases/`
  - Reads/writes files or external systems? -> `infra/`
  - Shared across layers? -> `xcut/` or `utilities/`
- The service vs free function decision framework (formalize `Notes/service_vs_free_function_architecture.md`):
  - Free functions: pure transformations, clean signatures (<=2 dependency params), avoids circular deps
  - Services: multi-step orchestration, 3+ dependencies, side effects, stateful processing
  - The "clean signature test" with examples
- The repository pattern: abstract interfaces in `domain/repos/`, concrete implementations in `infra/repos/`
  - The `TilesetRepo` -> `ArtifactReader`/`ArtifactWriter`/`ArtifactKeyProvider` delegation pattern
- Design principles summary (reference `ARCHITECTURE.md` Design Principles section for full details)

**Cross-references:** `data-flow-and-pipelines.md` for concrete data flow, `dependency-injection.md` for DI, `Porytiles2/ARCHITECTURE.md` in-repo (readers can cross-reference for the detailed codemap)

---

#### 4. `data-flow-and-pipelines.md` -- Data Flow and Compilation Pipelines

Concrete walkthrough of how data moves through the system during the major operations. This is the page that makes the architecture *real* -- showing the actual classes and methods involved at each step.

- **The compile pipeline** (detailed):
  - Input: RGBA layer PNGs (bottom, middle, top) + `attributes.csv`
  - `LayerImageMetatileizer` -> metatiles from layer images
  - `ImageTileizer` -> extract 8x8 tiles from images
  - Tile canonicalization -> `ShapeTile` deduplication
  - `PalettePacker` -> bin-pack tiles into hardware palettes (delegates to strategy)
  - `PrimaryTilesetCompiler` -> validation + final assembly
  - Output: `tiles.png`, `palettes/*.pal`, `metatiles.bin`, `metatile_attributes.bin`
- **Edit mode branching**: how optimize/patch/locked modes affect the pipeline
- **The decompile pipeline** (reverse direction):
  - Input: Porymap binary artifacts
  - `MetatileDecompiler` -> reconstruct RGBA layers
  - Output: layer PNGs + `attributes.csv`
- **The import pipeline**:
  - C parser reads `headers.h`, `graphics.h`, `metatiles.h`, `tileset_anims.c`
  - Discovers INCBIN paths -> reads binary artifacts
  - Constructs both `PorytilesTilesetComponent` and `PorymapTilesetComponent`
- Key domain models that flow through: `Tileset`, `PorytilesTilesetComponent`, `PorymapTilesetComponent`, `PixelTile<T>`, `Metatile`, `PackedPalette`
- The use case layer as orchestrator: how `CompilePrimaryTileset` wires together domain services
- Animation data flow (brief overview): frame PNGs + `key.png` + `anim.json` -> animation tile compilation

**Cross-references:** `layered-architecture.md` for layer context, `adding-a-packing-strategy.md` for extending the packing step, `c-parser.md` for the import parser

---

#### 5. `dependency-injection.md` -- Dependency Injection with Google Fruit

**Diataxis type: Explanation.** Understanding-oriented — explains the DI system, why it exists, and how it works.

Focused page on the DI system. Fruit is not widely used in C++ projects, so contributors need explicit guidance.

- Why DI in Porytiles: testability, runtime conditional binding (TTY detection for formatter selection), decoupling
- Google Fruit basics for contributors who have not used it: components, injectors, binding
- The composition root: where the injector is created (in `tools/driver/` command handlers)
- Current DI components in `xcut/di/components.hpp`:
  - `get_formatter_component()` -- conditionally binds `AnsiStyledTextFormatter` vs `PlainTextFormatter`
- Runtime conditional binding walkthrough: how `--no-color` flag and TTY detection flow through
- DI migration status: most services still manually instantiated in command handlers (migration planned)
- Testing with DI: using mock implementations, `BufferedUserDiagnostics`, `NullUserDiagnostics`

Note: The step-by-step recipe for adding a new DI-managed service has been extracted to `adding-a-di-managed-service.md` in the How-To Guides section, keeping this page focused on explanation.

**Cross-references:** `layered-architecture.md` for why interfaces live in domain, `external-dependencies.md` for Fruit library details, `adding-a-di-managed-service.md` for the step-by-step recipe

---

#### 6. `cpp-features-and-patterns.md` -- C++ Features and Patterns Used

**Diataxis type: Explanation.** Understanding-oriented — discusses WHY specific C++ features and patterns are used in the codebase, not just what they are.

Background for contributors who may not be familiar with specific C++20/23 features and patterns used in the codebase.

- **C++20 features in use**:
  - Concepts: `SupportsTransparency` and others for compile-time constraints
  - `std::source_location`: used in panic diagnostics for automatic file/line capture
  - Ranges and views: used in `utilities/functional/transform.hpp` and throughout for container transformations
- **C++23 features in use**:
  - `std::format` and `std::formatter` specializations (with the critical `auto &ctx` note from `STYLE.md`)
  - `std::expected`: used in `parse_int.hpp` and newer code for simple error returns
  - Deducing `this`: where and why it is used
- **Key patterns**:
  - Move-only types: `ChainableResult` is move-only, use case return values use `std::move`
  - Template aggregates: `PixelTile<T>`, `Image<T>`, `Metatile<T>` -- parameterized on pixel type
  - Strong types: `ArtifactKey`, `ConfigValue<T>` -- wrapping primitives for type safety
  - Bitmask enums: `Style` flags for text formatting
  - The `PT_TRY_ASSIGN_CHAIN_ERR` macro: what it expands to
- **fmtlib notes**: mostly migrated to `std::format`, but `fmt::dynamic_format_arg_store` still needed for runtime variable argument counts (reference `Notes/fmtlib_usage_analysis.md`)
- Reference to `STYLE.md` for naming conventions, include ordering, and idioms (do not duplicate)

**Cross-references:** `error-handling-and-diagnostics.md` for the Result/Error types in detail, `STYLE.md` (external reference)

---

### Section 3: Core Systems (Diataxis: Explanation)

#### 7. `config-generation-system.md` -- The Configuration Code Generation System

**Diataxis type: Explanation** (with reference-like listings that serve the explanatory narrative).

The most complex and unique system in the project. A single YAML file drives the generation of ~38 C++ files across three architectural layers. Deserves its own deep-dive.

- Overview: `config_schema.yaml` (780+ lines) -> `Scripts/generate_config.py` -> 24 Jinja2 templates -> ~38 generated C++ files
- **The schema format** -- fields per config value:
  - `canonical_name`, `symbol` (C++ method name), `yaml_path`, `cli_option`, `cli_desc`
  - `layer` (domain/app/infra), `type`, `parser`, `default_value`
  - `validators`, `cross_field_validators`
  - `header_define` (optional), `yaml_only`, `yaml_is_map`
- **Enum type definitions** in the schema: what they generate (enum class, `*_from_str()`, `to_string()`, fuzzy matching)
- **What gets generated and where**:
  - Layer config interfaces: `DomainConfig`, `AppConfig`, `InfraConfig` (abstract classes with pure virtual getters)
  - `LazyLayeredConfig`: single class implementing all three interfaces, lazy evaluation + caching + provenance tracking
  - Config providers: `DefaultProvider`, `YamlFileProvider`, `HeaderDefineProvider`, `CliOptionProvider`
  - CLI integration: `CliOptionStorage`, `CliOptionRegistration`, `CliCompletionData`
  - Valid YAML paths for validation
  - Mock configs for testing
- **The provider priority chain**: CLI > per-tileset local YAML > per-tileset YAML > project local YAML > project YAML > header defines > defaults
- **`ConfigValue<T>`**: the wrapper that tracks provenance (source location, source key, source details)
- **Three-tier validation**: raw fetch -> single-value validators -> cross-field validators
- **The template directory structure**: `config_templates/{domain,app,infra,testing}/` plus `_macros.jinja2`
- **How CMake triggers regeneration**: custom command with dependencies on schema + templates + script
- **The `porytiles.example.yaml`** reference file

**Cross-references:** `adding-a-config-value.md` for the step-by-step recipe, `layered-architecture.md` for why config interfaces are split by layer

---

#### 8. `error-handling-and-diagnostics.md` -- Error Handling and User Diagnostics

**Diataxis type: Explanation** (with reference-like listings that serve the explanatory narrative).

Covers two tightly related systems: the error propagation model and the diagnostic output system.

- **Error handling philosophy**: no C++ exceptions; two mechanisms for two purposes:
  - `ChainableResult<T, E>` for recoverable errors (most of the codebase)
  - `panic()` / `assert_or_panic()` for programmer errors / invariant violations (immediate abort with source location)
- **`ChainableResult<T, E>`**: monadic result type, move-only, error chain accumulation
  - How chaining works: `ChainableResult(new_error, cause_result)` preserves the full path from root cause to proximate error
  - The `PT_TRY_ASSIGN_CHAIN_ERR` macro for ergonomic propagation (similar to Rust's `?` operator)
  - What the macro expands to and how it short-circuits on error
- **`Error` interface and `FormattableError`**:
  - Format strings with styled `FormatParam` parameters (`Style::bold`, etc.)
  - `join()` for multi-error aggregation
  - The message style rules (capital first letter, ends with period, single quotes around highlighted items)
- **`UserDiagnostics` system**:
  - Abstract interface: `remark()`, `warning()`, `error()`, `fatal()` at four severity levels
  - Note variants (`remark_note()`, `warning_note()`, `error_note()`) for additional context lines
  - Tag-based filtering: regex include/exclude patterns for warnings and remarks
  - Concrete implementations:
    - `StderrStyledUserDiagnostics` (production)
    - `BufferedUserDiagnostics` (tests)
    - `NullUserDiagnostics` (silent)
    - `FilteredUserDiagnostics` (tag-based filtering wrapper)
- **`TextFormatter` integration**: `AnsiStyledTextFormatter` vs `PlainTextFormatter`, selected via DI
- **How error chains render**: the multi-line tree format with `caused by:` / `root cause:` and source location context (include the example from `ARCHITECTURE.md`)
- **When to use which**: Result for domain/app errors, panic for precondition violations, diagnostics for user-facing messages

**Cross-references:** `dependency-injection.md` for how formatter/diagnostics are injected, `writing-tests.md` for testing with `BufferedUserDiagnostics`, `cpp-features-and-patterns.md` for the macro details, reference `Notes/diagnostics_formatting_cookbook.md` for formatting recipes

---

#### 9. `c-parser.md` -- The C Parser System

A standalone subsystem that deserves dedicated documentation since it is non-trivial and critical for the import pipeline.

- **Purpose**: parse pokeemerald C/C++ header files to discover enum declarations, `#define` constants, `INCBIN` paths, function definitions, struct declarations, designated initializer lists
- **Location**: `utilities/c_parser/` -- lives in utilities because it has zero project-specific dependencies
- **Architecture**: lexer (`lexer.hpp`) -> token stream (`token.hpp`) -> parser (`parser.hpp`) -> AST node types
- **AST node types** and what each represents:
  - `EnumDeclaration` / `EnumMember`: C enum parsing (for metatile behaviors, terrain types, encounter types)
  - `DefineStatement`: `#define` macro parsing (for fieldmap constants like `NUM_TILES_IN_PRIMARY`)
  - `IncbinDeclaration`: `INCBIN_*` macro parsing (for discovering tileset image paths in `graphics.h`)
  - `FunctionDefinition` / `FunctionCallInfo`: function parsing (for `tileset_anims.c` callback discovery)
  - `StructVariableDeclaration` / `DesignatedInitializerField`: struct parsing (for metatile data)
  - `ArrayDeclaration`: array parsing
- **`CParserFacade`**: high-level entry point that wraps the lexer+parser and provides convenient query methods
- **`CParserContext`**: shared state during parsing
- **How the import pipeline uses it**: `HeaderBehaviorMapProvider` reads `metatile_behaviors.h`, `HeaderDefineProvider` reads `fieldmap.h`, import use case reads `graphics.h` and `tileset_anims.c`
- **Testing the parser**: unit tests with string inputs, no file I/O needed

**Cross-references:** `data-flow-and-pipelines.md` for how import uses the parser, `layered-architecture.md` for why it lives in utilities, reference `Notes/c_parser_ast_analysis.md` for detailed AST analysis

---

### Section 4: How-To Guides (Diataxis: How-to Guides)

#### 10. `adding-a-config-value.md` -- How to Add a New Configuration Value

Step-by-step recipe. The most common contributor task involving the config system.

- Step 1: Add the entry to `config_schema.yaml` -- all required fields, choosing the correct layer (domain/app/infra)
- Step 2: If defining a new enum type, add it to the enum types section of the schema
- Step 3: If needed, add a parser function (for new types) or validator function
- Step 4: Run the code generator: `uv run Scripts/generate_config.py` (or just build -- CMake triggers it automatically)
- Step 5: Verify generated files -- what to check in the generated layer interface, provider implementations, CLI registration
- Step 6: Use the new config value in code via the appropriate layer config interface
- Step 7: Add tests -- unit test the validator, integration test the full config resolution via mock configs
- Step 8: Update user docs `configuration.md` page with the new value
- Worked example: adding a hypothetical config value from schema entry to usage in code

**Cross-references:** `config-generation-system.md` for the full system explanation, `build-and-test.md` for running the generator

---

#### 11. `adding-a-command.md` -- How to Add a New CLI Command

Step-by-step recipe for adding a new subcommand to the `porytiles2` CLI.

- The Command pattern in `tools/driver/`: `command.hpp` base class, one `command_*.hpp/.cpp` per subcommand
- Step 1: Create a new `command_<name>.hpp` and `command_<name>.cpp` in `tools/driver/`
- Step 2: Implement the `Command` subclass: register CLI11 options, implement `Run()`
- Step 3: If needed, create a use case in `app/use_cases/` to hold the orchestration logic
- Step 4: Wire domain services in the command handler (or via DI when available)
- Step 5: Register the command in the driver's main setup
- Step 6: Add option groups if the command shares options with other commands
- Step 7: Add integration tests for the new command
- Reference: existing commands as templates (`command_compile_tileset` is the most complete example)

**Cross-references:** `layered-architecture.md` for where use cases vs commands live, `dependency-injection.md` for wiring services

---

#### 12. `adding-a-packing-strategy.md` -- How to Add a New Packing Strategy

Recipe for the Strategy pattern in the packing subsystem. Also serves as a concrete example of extending the domain layer.

- The `PackingStrategy` abstract interface in `domain/packing/services/`
- Existing strategies as reference:
  - `BestFusionStrategy` (greedy fusion)
  - `OverloadAndRemoveStrategy` (heuristic retries)
  - `BacktrackingStrategy` (exhaustive search)
- Step 1: Create a new strategy class implementing `PackingStrategy`
- Step 2: Add a new enum value to `PackingStrategyType` in `config_schema.yaml`
- Step 3: Run code generation to update the enum and CLI/YAML parsing
- Step 4: Wire the new strategy into `PalettePacker` (the orchestrating service)
- Step 5: Write unit tests with known tile/palette inputs and expected outputs
- Key packing domain models to understand: `PackableTile`, `PackedPalette`, `PaletteHint`, `PrefilledPalette`, `PalettePool`, `ColorSet`
- Brief overview of packing metrics (from `packing_metrics.hpp`): multiplicity, cost functions, efficiency calculations

**Cross-references:** `data-flow-and-pipelines.md` for where packing fits in compilation, `config-generation-system.md` and `adding-a-config-value.md` for adding the enum

---

#### 13. `adding-a-di-managed-service.md` -- How to Add a DI-Managed Service

Step-by-step recipe extracted from the DI explanation page. Covers the practical task of wiring a new service into the Google Fruit DI system.

- Step 1: Define the abstract interface in the appropriate layer (`domain/` or `xcut/`)
- Step 2: Create the concrete implementation in `infra/` (or same layer for non-I/O services)
- Step 3: Create a Fruit component function in `xcut/di/components.hpp`
- Step 4: Bind the component in the relevant command handler(s) in `tools/driver/`
- Step 5: Write tests using mock implementations (e.g., `BufferedUserDiagnostics`, `NullUserDiagnostics`)
- Reference: the existing `get_formatter_component()` as a template

**Cross-references:** `dependency-injection.md` for conceptual understanding of the DI system, `layered-architecture.md` for interface placement decisions

---

#### 14. `writing-tests.md` -- Writing Tests

Patterns and conventions for the test suite.

- **Unit tests vs integration tests**: when to use each
  - Unit: no I/O, no filesystem, all dependencies mocked, tests a single component in isolation
  - Integration: real file I/O, real assets from `Resources/`, tests multiple components together
- Test directory structure: `tests/unit/` mirrors the source layout, `tests/integration/` organized by feature
- GoogleTest basics for contributors who have not used it: `TEST`, `TEST_F`, `EXPECT_*`, `ASSERT_*`
- Custom test main (`test_main.cpp`): stacktrace control with `--enable-stacktrace`
- Testing with `BufferedUserDiagnostics`: inject it, run code, assert on buffered messages
- Testing with generated mock configs: what `MockDomainConfig`, `MockAppConfig`, `MockInfraConfig` provide and how to use them
- Testing `ChainableResult` error paths: checking error chains, verifying error messages
- Testing domain algorithms: pure functions are trivial to test -- call with inputs and check outputs
- Integration test resources: where they live (`Resources/`), how to add new test fixtures
- Running specific tests: GoogleTest filter flags (`--gtest_filter`)
- Code coverage: using `Scripts/coverage.py` to verify new code paths are actually exercised

**Cross-references:** `error-handling-and-diagnostics.md` for testing error paths, `build-and-test.md` for running the suite and coverage

---

### Section 5: Reference (Diataxis: Reference)

#### 15. `build-and-test.md` -- Building, Testing, and Development Workflows

**Diataxis type: Reference.** Information-oriented — a lookup resource for build commands, test flags, and development workflows.

Detailed build and test reference beyond the initial setup.

- CMake configuration options and build types (Debug, Release)
- The code generation step: what `generate_config.py` does, when CMake re-runs it (custom command with dependencies on schema + templates)
- Building specific targets: `porytiles2` (executable), `Porytiles2UnitTests`, `Porytiles2IntegrationTests`, `Porytiles2AllTests`
- Running tests: GoogleTest filter flags (`--gtest_filter`), running specific test suites or individual tests
- The custom test main (`test_main.cpp`): stacktrace control via `--enable-stacktrace`
- Test resource files in `Resources/` and how integration tests reference them
- Running the formatter: `uv run Scripts/format.py` (reference `STYLE.md` and `.clang-format` for what it enforces)
- Running the linter: `uv run Scripts/tidy.py`
- Running code coverage: `uv run Scripts/coverage.py build`, `report`, `show`, `clean`
- The common development loop: edit code -> build -> run tests -> format -> commit
- Testing against a real decomp project: the `../pokeemerald-expansion` testbed

**Cross-references:** `dev-environment-setup.md` for initial setup, `scripts-and-tooling.md` for full script reference, `writing-tests.md` for test authoring patterns

---

#### 16. `external-dependencies.md` -- External Dependencies

Quick reference for every third-party library: what it provides, why it was chosen, and where it is used.

| Library | Version | Purpose | Used In |
|---------|---------|---------|---------|
| **fmt** (fmtlib) | 11.1.4 | String formatting (`dynamic_format_arg_store`) | Throughout, for diagnostic messages |
| **GSL** | v4.2.0 | `gsl::not_null`, safety annotations | Throughout |
| **png++** | HEAD | Header-only libpng wrapper | `infra/services/` (PNG I/O) |
| **nlohmann/json** | v3.12.0 | JSON parsing (header-only) | `infra/services/` (`anim.json`) |
| **yaml-cpp** | master | YAML config file parsing | `infra/config/` |
| **cpptrace** | v0.7.5 | Cross-platform stacktrace capture | `utilities/panic/`, test main |
| **Google Fruit** | master | Dependency injection framework | `xcut/di/` |
| **CLI11** | v2.5.0 | Command-line argument parsing | `tools/driver/` |
| **GoogleTest** | v1.16.0 | Testing framework | `tests/` only |

- How dependencies are fetched: CMake FetchContent with pinned versions
- System dependencies: `libpng` and `zlib` (via `find_package`)
- The Fruit C++17 workaround: Fruit is built with C++17 during fetch, then restored to C++23
- Adding a new dependency: where to add in `lib/CMakeLists.txt`, FetchContent pattern to follow

**Cross-references:** `build-and-test.md` for build system details, `dependency-injection.md` for Fruit specifics

---

#### 17. `scripts-and-tooling.md` -- Scripts and Tooling

Reference for the `Scripts/` directory and developer utilities. All scripts use `uv run` for execution.

| Script | Purpose | Key Usage |
|--------|---------|-----------|
| `generate_config.py` | Config code generation from YAML schema | `uv run Scripts/generate_config.py` |
| `format.py` | Runs clang-format on all Porytiles2 sources | `uv run Scripts/format.py` |
| `coverage.py` | LLVM source-based code coverage | `uv run Scripts/coverage.py build\|report\|show\|clean` |
| `tidy.py` | Runs clang-tidy static analysis | `uv run Scripts/tidy.py` |
| `todo.py` | Scans for TODO/FIXME/HACK comments | `uv run Scripts/todo.py` |
| `new_class.py` | Scaffolds new C++ class (header + cpp + test) | `uv run Scripts/new_class.py ClassName` |
| `dump_metatiles_json.py` | Debugging: dump metatile data as JSON | `uv run Scripts/dump_metatiles_json.py` |
| `find_and_replace.py` | Project-wide find-and-replace | `uv run Scripts/find_and_replace.py` |
| `set_pixel.py` | Pixel manipulation for test assets | `uv run Scripts/set_pixel.py` |

- Python environment management: `pyproject.toml` + `uv.lock`, requires Python >= 3.13
- The `.porytiles-marker-file`: used by scripts to validate they are running from the repo root

**Cross-references:** `build-and-test.md` for common development workflows, `config-generation-system.md` for `generate_config.py` deep-dive

---

#### 18. `ci-cd.md` -- CI/CD and Release Process

Reference for the GitHub Actions setup.

- Workflow files in `.github/workflows/`
- Build matrix:
  - Linux Clang (amd64 + arm64) -- primary CI targets
  - macOS Clang (arm64) -- Apple Silicon
  - Linux GCC -- planned (waiting for C++23 support in GitHub Actions runners)
- `dev_build.yml`: triggered on pushes to `develop` branch and manual dispatch
- `pr_dev_build.yml`: triggered on pull requests
- `nightly_release.yml`: scheduled nightly release builds
- Reusable workflow template: `build_jobs_template.yml` with composite actions for install, build, test
- What CI checks: build success on all matrix entries, all tests pass
- Branch strategy: `develop` as the primary development branch, feature branches merged via PR
- Documentation deployment: separate workflow for GitHub Pages

**Cross-references:** `build-and-test.md` for local equivalents of CI steps

---

#### 19. `glossary.md` -- Glossary

Quick-lookup definitions for project-specific terms. Keeps other pages from repeating definitions.

- **Domain terms**: metatile, subtile, tile, palette, palette slot, extrinsic transparency, layer mode (dual/triple), primary vs secondary tileset, tile sharing, palette packing, behavior, terrain type, encounter type
- **Architecture terms**: domain layer, app layer, infra layer, xcut layer, utilities layer, config provider, config value, provenance, artifact, artifact key, use case, service, repository
- **Code terms**: `ChainableResult`, `FormattableError`, `ConfigValue`, `PackableTile`, `PackedPalette`, `PaletteHint`, `PorytilesTilesetComponent`, `PorymapTilesetComponent`, `UserDiagnostics`, `TextFormatter`
- **Tool terms**: Porymap, pokeemerald, pokeemerald-expansion, pokefirered, pokeruby, decomp project, GBA
- Each entry is a short definition (1-3 sentences) with a cross-reference to the page that covers it in depth

**Cross-references:** links out to every other page as appropriate

---

## Page Summary

| #  | File                                | Section         | Diataxis Type   | Status              |
|----|-------------------------------------|-----------------|-----------------|---------------------|
| 1  | `dev-environment-setup.md`          | Getting Started | Tutorial        | New                 |
| 2  | `project-layout.md`                | Architecture    | Explanation     | New                 |
| 3  | `layered-architecture.md`          | Architecture    | Explanation     | New                 |
| 4  | `data-flow-and-pipelines.md`       | Architecture    | Explanation     | New                 |
| 5  | `dependency-injection.md`          | Architecture    | Explanation     | New                 |
| 6  | `cpp-features-and-patterns.md`     | Architecture    | Explanation     | New                 |
| 7  | `config-generation-system.md`      | Core Systems    | Explanation     | New                 |
| 8  | `error-handling-and-diagnostics.md` | Core Systems   | Explanation     | New                 |
| 9  | `c-parser.md`                      | Core Systems    | Explanation     | New                 |
| 10 | `adding-a-config-value.md`         | How-To Guides   | How-to guide    | New                 |
| 11 | `adding-a-command.md`              | How-To Guides   | How-to guide    | New                 |
| 12 | `adding-a-packing-strategy.md`     | How-To Guides   | How-to guide    | New                 |
| 13 | `adding-a-di-managed-service.md`   | How-To Guides   | How-to guide    | New                 |
| 14 | `writing-tests.md`                 | How-To Guides   | How-to guide    | New                 |
| 15 | `build-and-test.md`                | Reference       | Reference       | New                 |
| 16 | `external-dependencies.md`         | Reference       | Reference       | New                 |
| 17 | `scripts-and-tooling.md`           | Reference       | Reference       | New                 |
| 18 | `ci-cd.md`                         | Reference       | Reference       | New                 |
| 19 | `glossary.md`                      | Reference       | Reference       | New                 |
| -- | `testbed.md`                       | (hidden)        | —               | Existing (preserve) |

## Implementation Approach

This is a **documentation structure plan only** -- actual content writing will happen page-by-page. Implementation order should follow the toctree order (getting started -> architecture -> core systems -> how-to guides -> reference) since later pages reference earlier ones. Note that the toctree order is the recommended *writing* order -- the final documentation should not require linear reading except for the tutorial.

### Step 1: Restructure the toctree
- Update `docsrc/index.rst` with the 5-section toctree above (plus hidden section for testbed)
- Delete the 4 stub files (`getting-started.md`, `architecture.md`, `contributing.md`, `reference.md`)
- Create stub files for all 19 new pages (title + brief description placeholder)
- Verify the Sphinx build succeeds with stubs: `cd porytiles-dev-docs/docsrc && uv run make html`

### Step 2: Write pages (one at a time, in toctree order)
Each page should use clear prose with code snippets where they add clarity, ASCII diagrams for architecture and data flow, and concrete examples. The tone should be direct and technical -- this audience knows C++ and software engineering, they just do not know this specific codebase.

Key content sources for each page:
- `Porytiles2/ARCHITECTURE.md` -- primary source for architecture pages (complement, do not duplicate verbatim)
- `Porytiles2/Notes/*.md` -- design decisions to formalize (19 files with analysis and rationale)
- `Porytiles2/config_templates/config_schema.yaml` -- config system details
- `STYLE.md` -- reference only, do not duplicate

### Step 3: Add cross-reference links
After all pages have content, do a pass to add MyST cross-reference links between pages as specified in each page's breakdown.

### Verification
- `cd porytiles-dev-docs/docsrc && uv run make html` -- build must succeed with no warnings
- Visual inspection of built HTML at `docsrc/_build/html/index.html`
- Cross-reference links resolve correctly
- Reading order test: can a new contributor read pages 1-6 sequentially and understand the codebase well enough to attempt a how-to guide?

---

## Design Decisions

### Why 5 sections?

1. **Getting Started** (1 page) -- the tutorial: procedural onboarding, read once, produces a working build
2. **Architecture** (5 pages) -- conceptual understanding: explains the codebase structure, patterns, and design decisions
3. **Core Systems** (3 pages) -- deep dives into the most complex/unique subsystems (also explanation, separated for navigability)
4. **How-To Guides** (5 pages) -- step-by-step recipes for common contributor tasks
5. **Reference** (5 pages) -- lookup tables and quick references

This follows the [Diataxis framework](https://diataxis.fr/): tutorials (Getting Started), explanation (Architecture + Core Systems), how-to guides (How-To Guides), reference (Reference). Each page has a primary Diataxis type annotated in its breakdown above.

### Why no dedicated palette packing deep-dive?

The packing system is covered from two angles: the architecture side in `data-flow-and-pipelines.md` (where packing fits in compilation) and the practical side in `adding-a-packing-strategy.md` (how to extend it). A standalone deep-dive would overlap significantly with both. If packing docs need to grow, a dedicated page can be split out later.

### Why reference STYLE.md instead of incorporating it?

`STYLE.md` lives in the repo root and is the authoritative source for code style. Duplicating it in the dev docs creates a maintenance burden. The `cpp-features-and-patterns.md` page covers C++ features and patterns used in the codebase, which is complementary to (not a duplicate of) style rules.

### Why a glossary?

Many terms (metatile, subtile, extrinsic transparency, artifact key, etc.) appear across multiple pages. A glossary provides a single authoritative source and serves as a quick reference for contributors jumping into the middle of the docs.

### Why testbed in a hidden toctree?

It is a formatting reference for documentation authors, not part of the logical reading flow. Hiding it from the main navigation keeps the structure clean while preserving the page for internal use.

### Relationship to ARCHITECTURE.md

The in-repo `ARCHITECTURE.md` is a detailed codemap optimized for quick reference by repo browsers. The dev docs site provides a different reading experience: richer diagrams, decision frameworks ("where does my code go?"), concrete data flow walkthroughs, and step-by-step recipes. Some conceptual overlap is expected and acceptable -- the two documents serve different use cases.
