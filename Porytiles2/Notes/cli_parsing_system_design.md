# CLI Parsing System Design for Porytiles2

**Author:** Claude (AI Assistant)
**Date:** 2026-02-02
**Status:** Design Document (Not Yet Implemented)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Requirements](#requirements)
3. [Current State Analysis](#current-state-analysis)
4. [CLI Library Research](#cli-library-research)
5. [Proposed Architecture](#proposed-architecture)
6. [Detailed Design](#detailed-design)
7. [Implementation Plan](#implementation-plan)
8. [Design Decisions & Rationale](#design-decisions--rationale)
9. [Alternatives Considered](#alternatives-considered)
10. [Verification Plan](#verification-plan)
11. [References](#references)

---

## Executive Summary

This document proposes a CLI parsing system for Porytiles2 that:

1. **Auto-generates CLI flags** from `config_schema.yaml` via a new `CliOptionProvider`
2. **Supports manual CLI flags** not defined in the config schema
3. **Provides shell TAB completion** for bash, zsh, and fish
4. **Stays with CLI11** (the current library works well; we just need to add completion manually)

The design leverages the existing code generation infrastructure (Jinja2 templates) to minimize maintenance burden and ensure CLI options stay synchronized with the config system.

---

## Requirements

### Core Requirements

1. **Config-driven CLI options:** Users should be able to set any config value via the CLI. The `cli_option` field in `config_schema.yaml` defines the flag name (e.g., `cli_option: num-tiles-in-primary` becomes `--num-tiles-in-primary`).

2. **Non-config CLI options:** Support CLI flags that aren't part of the config schema (e.g., `--output`, `--verbose`, `--help`, `--version`). Whatever system we adopt should allow defining flags/options that aren't explicitly in `config_schema.yaml`.

3. **TAB completion:** Must support TAB completion, or at least have the plumbing in place. Users should be able to type `--tiles-pal-<TAB>` and have it complete to `--tiles-pal-mode`.

4. **Library flexibility:** Currently using CLI11. Open to alternatives if something else works better for accomplishing the above requirements.

### Derived Requirements

- CLI options should have **highest priority** in the config provider chain (CLI > YAML > Header > Default)
- CLI should use **hyphenated** string values for enums (e.g., `--tiles-pal-mode=true-color`)
- Complex types like `std::vector<PaletteHint>` should be **excluded from CLI** (too complex; use YAML instead)

---

## Current State Analysis

### Config System Architecture

The Porytiles2 config system is a sophisticated, layered architecture:

#### Schema Definition (`config_schema.yaml`)

The schema defines 24 config values with these key fields:

```yaml
- canonical_name: Number Of Tiles In Primary    # Human-readable display name
  symbol: num_tiles_in_primary                  # C++ method name
  yaml_path: fieldmap.num_tiles_in_primary      # Dot-separated YAML path
  header_define: NUM_TILES_IN_PRIMARY           # #define macro name (optional)
  cli_option: num-tiles-in-primary              # CLI flag name (NOT YET USED)
  layer: domain                                 # domain/app/infra
  type: std::size_t                             # C++ type
  parser: parse_size_t                          # Parser function name
  default_value: "512"                          # Default value
  validators: [ size_t_val_greater_than_zero ]  # Single-value validators
  cross_field_validators: [ compare_less_equal:num_tiles_total ]  # Cross-field validators
```

**Key insight:** The `cli_option` field is already defined for all 24 config values but is marked "not yet used in templates" (line 15 of the schema file).

#### Three-Tier Validation System

The config system uses a three-tier validation approach:

1. **Tier 1 (Raw):** `*_raw()` - Protected virtual methods that fetch from ConfigProvider, returning `ChainableResult<ConfigValue<T>>`

2. **Tier 2 (Validated):** `*_validated()` - Protected methods that apply single-value validators in sequence

3. **Tier 3 (Public):** Public methods that apply cross-field validators with access to other config values

Example generated code:
```cpp
[[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
num_tiles_in_primary(ConfigScopeType type, const std::string &scope) const
{
    auto validated_val = num_tiles_in_primary_validated(type, scope);
    // Apply cross-field validators
    if (validated_val.has_value()) {
        validated_val = compare_less_equal<std::size_t>(
            validated_val.value(),
            *this, type, scope, "num_tiles_total",
            [](const DomainConfig &cfg, ConfigScopeType t, const std::string &s) {
                return cfg.num_tiles_total_validated(t, s);
            });
    }
    return validated_val;
}
```

#### ConfigProvider Interface

Base class with virtual methods for each config value:
```cpp
[[nodiscard]] virtual LayerValue<T> symbol_name(ConfigScopeType type, const std::string &scope) const;
```

Returns `LayerValue<T>` with three states:
- **not_provided:** Provider doesn't handle this config (try next provider)
- **valid:** Provider supplies valid config (use this value)
- **invalid:** Provider found invalid config (stop and report error)

#### Existing ConfigProvider Implementations

1. **DefaultProvider:** Returns hardcoded default values; always at end of provider chain

2. **YamlFileProvider:** Reads from YAML files with priority ordering:
   - `porytiles/tilesets/{tileset_name}/config.local.yaml` (highest)
   - `porytiles/tilesets/{tileset_name}/config.yaml`
   - `porytiles/config.local.yaml`
   - `porytiles/config.yaml` (lowest)

3. **HeaderDefineProvider:** Reads `#define` values from C/C++ header files using CParserFacade

**No CLI Provider Currently Exists** - this is what we're designing.

#### LazyLayeredConfig

The main config container that implements all three layer config interfaces:
```cpp
class LazyLayeredConfig final : public DomainConfig, public AppConfig, public InfraConfig
```

Features:
- Takes prioritized list of ConfigProviders
- Lazy resolution with caching
- Panics if no provider supplies a value
- Provenance chain methods for debugging

### Current CLI System (CLI11 v2.5.0)

#### Main App Structure (`main.cpp`)

```cpp
CLI::App porytiles_app{"Porytiles"};
porytiles_app.add_flag("-V,--version", /* callback */, "Print version info");
porytiles_app.require_subcommand();
CLI11_PARSE(porytiles_app, argc, argv);
```

#### Four Core Commands

Each command is a subclass of `Command`:
1. **CreateTilesetCommand** - `create-tileset <tileset-name>`
2. **ImportTilesetCommand** - `import-tileset <tileset-name>`
3. **CompileTilesetCommand** - `compile-tileset <tileset-name>`
4. **DecompileTilesetCommand** - `decompile-tileset <tileset-name>`

#### Command Base Class Pattern

```cpp
class Command {
    Command(CLI::App &parent_app, const std::string &name, const std::string &desc, const std::string &group) {
        app_ = parent_app.add_subcommand(name, desc);
        app_->callback([this] { this->Run(); });
    }
    virtual void Run() = 0;
};
```

#### Option Architecture (Two-tier)

**Tier 1: Individual Options** (`option.hpp`)
```cpp
class Opt {
    virtual std::string NameShort() const = 0;
    virtual std::string NameLong() const = 0;
    virtual void RegisterOpt(CLI::App &app) = 0;
};
```

Implemented options: `OptOutput`, `OptTilesPalMode`, `OptDisableMetatileGeneration`, etc.

**Tier 2: Option Groups** (`option_group.hpp`)
```cpp
class OptGroup {
    virtual std::string GroupName() = 0;
    virtual void RegisterGroup(CLI::App &app) = 0;
};
```

Implemented groups: `OptGroupFieldmap`, `OptGroupDiagnostics`, `OptGroupArtifacts`

#### Custom Validators

```cpp
class TilesPalModeValidator : public CLI::Validator { /* ... */ };
class RgbStringValidator : public CLI::Validator { /* ... */ };
```

#### Current Limitations

1. **No shell completion support** - CLI11 doesn't provide built-in completion
2. **No CLI-to-config integration** - CLI options are hardcoded, not generated from schema
3. **Manual option definitions** - Each option is defined separately in code

### All 24 Config Values

**Domain Layer (16 values):**
- Fieldmap: `num_tiles_in_primary`, `num_tiles_total`, `num_metatiles_in_primary`, `num_metatiles_total`, `num_pals_in_primary`, `num_pals_total`, `max_map_data_size`, `num_tiles_per_metatile`
- Tileset: `extrinsic_transparency`, `tiles_edit_mode`, `pals_edit_mode`, `pal_hints_enabled`, `pal_hints`, `tiles_pal_mode`, `anim_pal_resolution_strategy`, `anim_key_frame_resolution_strategy`

**App Layer (1 value):**
- `verify_checksums`

**Infra Layer (5 values):**
- `tileset_paths_primary_src`, `tileset_paths_primary_bin`, `tileset_paths_secondary_src`, `tileset_paths_secondary_bin`, `tileset_animations_wire_anim_code`

---

## CLI Library Research

### CLI11 Completion Capabilities

**Current Status:** CLI11 does **NOT** have built-in shell completion support.

According to [GitHub issue #343](https://github.com/CLIUtils/CLI11/issues/343), shell completion has been a long-standing feature request since November 2019. As of August 2022, it remained unimplemented in the core library.

The issue discussion explored multiple approaches:
- Running the program with a special flag to generate completions dynamically
- Generating static bash/zsh/fish completion scripts at build time
- Using environment variables similar to Python's argcomplete tool

A contributor mentioned implementing "a very basic way" allowing users to call `cli11_install_completion_file(<target name>)` from CMakeLists.txt, but this was not merged.

**Implication:** If we want shell completion with CLI11, we must implement it as a custom solution.

### Alternative C++ Libraries

#### Taywee/args (Header-only, C++11+)

**Completion Support:** Yes, but Bash only

- [GitHub - Taywee/args](https://github.com/Taywee/args)
- Single-header library compatible with Python's argparse
- Includes built-in `CompletionFlag` class for bash completion
- Works by raising a `Completion` exception during parsing with suggestions

Example:
```cpp
args::CompletionFlag c(p, {"complete"});
try {
    p.ParseCLI(argc, argv);
} catch (args::Completion &e) {
    std::cout << e.what();
}
```

**Limitation:** Only supports Bash, not Zsh or Fish.

#### spevnev/args (Header-only, C99/C++11+)

**Completion Support:** Yes - Bash, Zsh, Fish

- [GitHub - spevnev/args](https://github.com/spevnev/args)
- Single-header library supporting multiple shells
- Users can generate completions via: `program completion <bash|zsh|fish>`
- Cross-platform: Linux, macOS, Windows

#### cxxopts (Header-only, C++11+)

**Completion Support:** No

- Lightweight option parser for standard GNU style syntax
- Minimal, easy-to-include library

#### Boost.Program_options

**Completion Support:** No

- Part of Boost library
- More heavyweight dependency

### Comparison Table

| Library | Type | Completion | Shells | C++ Version |
|---------|------|-----------|--------|-------------|
| **CLI11** (current) | Header-only | None | N/A | C++11+ |
| **Taywee/args** | Header-only | Yes | Bash only | C++11+ |
| **spevnev/args** | Header-only | Yes | Bash, Zsh, Fish | C++11+ |
| **cxxopts** | Header-only | No | N/A | C++11+ |
| **Boost.Program_options** | Library | No | N/A | C++11+ |

### Best Practices from Established Tools

**GitHub CLI (Go/Cobra):**
- Uses `GenBashCompletion()` and `GenZshCompletion()` built into Cobra
- Supports dynamic completions that query APIs
- Completions generated at build time or via `gh completion <shell>`

**Rust clap Ecosystem:**
- Two approaches: compile-time via `build.rs` or runtime flag
- Supports Bash, Fish, Zsh, PowerShell, Elvish
- Uses `clap_generate` crate

**Python Click:**
- Supports Bash, Zsh, Fish via `shell_complete` framework
- Custom `ParamType` completion via `shell_complete()` method

### Shell Completion Technical Details

When shells invoke completion:
- **COMP_WORDS** (Bash): Array of all words on the command line
- **COMP_CWORD** (Bash): Index of current word being completed
- **COMP_LINE** (Bash): Entire command line as string

**Key insight from Mill Build Tool:**
- For single-completion descriptions: Add duplicate completion entries to create artificial ambiguity, triggering description display
- Prefix matching: Complete `foo a<TAB>` filters candidates starting with "a"

### Recommendation

**Stay with CLI11** and implement completion manually because:
1. CLI11 is mature, well-documented, and actively maintained
2. Migration cost would be significant
3. Completion can be implemented as a `completion` subcommand (standard pattern)
4. The existing CLI11 infrastructure is working well

---

## Proposed Architecture

### High-Level Flow

```
User: porytiles compile-tileset MyTileset --num-pals-total=16
                              │
                              ▼
┌─────────────────────────────────────────────────────┐
│              CLI11 Parsing Layer                     │
│  - Parses args into CliOptionStorage (generated)    │
│  - Manual options (--output, etc) handled separately │
└─────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────┐
│              CliOptionProvider                       │
│  - Implements ConfigProvider interface              │
│  - Wraps CliOptionStorage                           │
│  - Returns LayerValue<T> for each config value      │
└─────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────┐
│              LazyLayeredConfig                       │
│  Priority: CLI > YAML > Header > Default            │
└─────────────────────────────────────────────────────┘
```

### Component Overview

1. **CliOptionStorage:** Generated struct holding `std::optional<T>` for each config value
2. **CliOptionProvider:** ConfigProvider implementation that wraps CliOptionStorage
3. **register_config_options():** Generated function to register options with CLI11
4. **cli_completion_data.hpp:** Generated metadata for shell completion scripts
5. **CompletionCommand:** Manual command that generates shell completion scripts

---

## Detailed Design

### CliOptionStorage

A generated struct that holds parsed CLI values:

```cpp
// Generated file: include/porytiles2/infra/cli/cli_option_storage.hpp
namespace porytiles2 {

struct CliOptionStorage {
    // Each config value with a cli_option gets an optional<T> field
    std::optional<std::size_t> num_tiles_in_primary;
    std::optional<std::size_t> num_tiles_total;
    std::optional<std::size_t> num_metatiles_in_primary;
    std::optional<std::size_t> num_metatiles_total;
    std::optional<std::size_t> num_pals_in_primary;
    std::optional<std::size_t> num_pals_total;
    std::optional<std::size_t> max_map_data_size;
    std::optional<std::size_t> num_tiles_per_metatile;
    std::optional<Rgba32> extrinsic_transparency;
    std::optional<ArtifactEditMode> tiles_edit_mode;
    std::optional<ArtifactEditMode> pals_edit_mode;
    std::optional<bool> pal_hints_enabled;
    // Note: pal_hints SKIPPED - too complex for CLI
    std::optional<TilesPalMode> tiles_pal_mode;
    std::optional<AnimPalResolutionStrategy> anim_pal_resolution_strategy;
    std::optional<AnimKeyFrameResolutionStrategy> anim_key_frame_resolution_strategy;
    std::optional<bool> verify_checksums;
    std::optional<std::string> tileset_paths_primary_src;
    std::optional<std::string> tileset_paths_primary_bin;
    std::optional<std::string> tileset_paths_secondary_src;
    std::optional<std::string> tileset_paths_secondary_bin;
    std::optional<bool> tileset_animations_wire_anim_code;
};

} // namespace porytiles2
```

### CliOptionProvider

Implements ConfigProvider interface, wraps CliOptionStorage:

```cpp
// Generated file: include/porytiles2/infra/config/cli_option_provider.hpp
namespace porytiles2 {

class CliOptionProvider final : public ConfigProvider {
  public:
    explicit CliOptionProvider(const CliOptionStorage &storage);

    [[nodiscard]] std::string name() const override;

    // Generated methods for each config value:
    [[nodiscard]] LayerValue<std::size_t>
    num_tiles_in_primary(ConfigScopeType type, const std::string &scope) const override;

    [[nodiscard]] LayerValue<TilesPalMode>
    tiles_pal_mode(ConfigScopeType type, const std::string &scope) const override;

    // ... one method per config value (except cli_skip: true)

  private:
    const CliOptionStorage &storage_;
};

} // namespace porytiles2
```

Implementation pattern:

```cpp
// Generated file: lib/infra/config/cli_option_provider.cpp
LayerValue<std::size_t> CliOptionProvider::num_tiles_in_primary(
    [[maybe_unused]] ConfigScopeType type,
    [[maybe_unused]] const std::string &scope) const
{
    if (!storage_.num_tiles_in_primary.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }
    return LayerValue<std::size_t>::valid(
        storage_.num_tiles_in_primary.value(),
        "Number Of Tiles In Primary",  // canonical_name from schema
        "CLI option --num-tiles-in-primary");
}
```

**Note:** CLI options are **scope-agnostic** - they apply globally regardless of `ConfigScopeType`. This is why the `type` and `scope` parameters are marked `[[maybe_unused]]`.

### CLI11 Option Registration

Generated function that registers all config options with CLI11:

```cpp
// Generated file: include/porytiles2/infra/cli/cli_option_registration.hpp
namespace porytiles2 {

void register_config_options(CLI::App &app, CliOptionStorage &storage);

} // namespace porytiles2
```

```cpp
// Generated file: lib/infra/cli/cli_option_registration.cpp
void register_config_options(CLI::App &app, CliOptionStorage &storage)
{
    // std::size_t options
    app.add_option("--num-tiles-in-primary", storage.num_tiles_in_primary,
        "Number Of Tiles In Primary")
        ->group("Config Options");

    app.add_option("--num-tiles-total", storage.num_tiles_total,
        "Number Of Tiles Total")
        ->group("Config Options");

    // bool options with negation pattern
    app.add_flag("--verify-checksums,!--no-verify-checksums", storage.verify_checksums,
        "Verify Checksums")
        ->group("Config Options");

    app.add_flag("--pal-hints-enabled,!--no-pal-hints-enabled", storage.pal_hints_enabled,
        "Palette Hints Enabled")
        ->group("Config Options");

    // enum options with CheckedTransformer
    app.add_option("--tiles-pal-mode", storage.tiles_pal_mode,
        "Tiles Palette Mode")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, TilesPalMode>{
                {"true-color", TilesPalMode::true_color},
                {"greyscale", TilesPalMode::greyscale}
            }, CLI::ignore_case))
        ->group("Config Options");

    app.add_option("--tiles-edit-mode", storage.tiles_edit_mode,
        "Tiles Edit Mode")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, ArtifactEditMode>{
                {"optimize", ArtifactEditMode::optimize},
                {"preserve", ArtifactEditMode::preserve}
            }, CLI::ignore_case))
        ->group("Config Options");

    // Rgba32 option with custom transform
    app.add_option("--extrinsic-transparency", storage.extrinsic_transparency,
        "Extrinsic Transparency (R,G,B or R,G,B,A)")
        ->transform(/* Rgba32 parser transform */)
        ->group("Config Options");

    // ... repeat for all config values
}
```

### Type Handling Matrix

| Type | CLI11 Method | Transform/Notes |
|------|--------------|-----------------|
| `std::size_t` | `add_option` | Direct parsing |
| `bool` | `add_flag` | `--flag,!--no-flag` pattern |
| `std::string` | `add_option` | Direct parsing |
| `Rgba32` | `add_option` | Custom transform: "R,G,B" or "R,G,B,A" |
| `TilesPalMode` | `add_option` | `CheckedTransformer` with hyphenated strings |
| `ArtifactEditMode` | `add_option` | `CheckedTransformer` with hyphenated strings |
| `AnimPalResolutionStrategy` | `add_option` | `CheckedTransformer` with hyphenated strings |
| `AnimKeyFrameResolutionStrategy` | `add_option` | `CheckedTransformer` with hyphenated strings |
| `std::vector<PaletteHint>` | **SKIP** | Too complex for CLI, use YAML |

### Completion Data

Generated metadata for shell completion:

```cpp
// Generated file: include/porytiles2/infra/cli/cli_completion_data.hpp
namespace porytiles2 {

struct CliOptionMeta {
    std::string long_name;
    std::string description;
    std::vector<std::string> choices;  // For enums
};

inline const std::vector<CliOptionMeta> config_option_metadata = {
    {"num-tiles-in-primary", "Number Of Tiles In Primary", {}},
    {"num-tiles-total", "Number Of Tiles Total", {}},
    {"tiles-pal-mode", "Tiles Palette Mode", {"true-color", "greyscale"}},
    {"tiles-edit-mode", "Tiles Edit Mode", {"optimize", "preserve"}},
    {"pals-edit-mode", "Palettes Edit Mode", {"optimize", "preserve"}},
    {"anim-pal-resolution-strategy", "Animation Palette Resolution Strategy",
        {"error", "warn-use-primary", "warn-use-first-frame", "use-primary", "use-first-frame"}},
    // ... etc
};

inline const std::vector<std::string> subcommand_names = {
    "compile-tileset",
    "create-tileset",
    "import-tileset",
    "decompile-tileset",
    "completion"
};

} // namespace porytiles2
```

### CompletionCommand

Manual command that generates shell completion scripts:

```cpp
// tools/driver/command_completion.hpp
class CompletionCommand final : public Command {
  public:
    explicit CompletionCommand(CLI::App &parent_app)
        : Command{parent_app, "completion", "Generate shell completion scripts", "UTILITY"}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<shell>", shell_, "Shell type: bash, zsh, fish")->required();
    }

    void Run() override {
        if (shell_ == "bash") {
            generate_bash_completion();
        } else if (shell_ == "zsh") {
            generate_zsh_completion();
        } else if (shell_ == "fish") {
            generate_fish_completion();
        } else {
            std::cerr << "Unknown shell: " << shell_ << std::endl;
            exit(1);
        }
    }

  private:
    std::string shell_;

    void generate_bash_completion();
    void generate_zsh_completion();
    void generate_fish_completion();
};
```

Example bash completion output:

```bash
_porytiles_completions() {
    local cur prev opts subcommands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    subcommands="compile-tileset create-tileset import-tileset decompile-tileset completion"

    # Config options (from config_option_metadata)
    config_opts="--num-tiles-in-primary --num-tiles-total --num-metatiles-in-primary ..."

    # Handle enum value completion
    case "${prev}" in
        --tiles-pal-mode)
            COMPREPLY=( $(compgen -W "true-color greyscale" -- ${cur}) )
            return 0
            ;;
        --tiles-edit-mode|--pals-edit-mode)
            COMPREPLY=( $(compgen -W "optimize preserve" -- ${cur}) )
            return 0
            ;;
        --anim-pal-resolution-strategy)
            COMPREPLY=( $(compgen -W "error warn-use-primary warn-use-first-frame use-primary use-first-frame" -- ${cur}) )
            return 0
            ;;
    esac

    # Main completion logic
    if [[ ${cur} == -* ]]; then
        COMPREPLY=( $(compgen -W "${config_opts} --help --version" -- ${cur}) )
    else
        COMPREPLY=( $(compgen -W "${subcommands}" -- ${cur}) )
    fi
}
complete -F _porytiles_completions porytiles
```

Usage:
```bash
eval "$(porytiles completion bash)"   # Add to .bashrc
eval "$(porytiles completion zsh)"    # Add to .zshrc
porytiles completion fish > ~/.config/fish/completions/porytiles.fish
```

### Command Integration

Updated `CompileTilesetCommand`:

```cpp
class CompileTilesetCommand final : public Command {
  public:
    explicit CompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();

        // Register all config options (generated)
        register_config_options(cmd, cli_storage_);

        // Manual options (not in config system)
        // cmd.add_option("-o,--output", output_path_, "Output directory");
    }

    void Run() override
    {
        using namespace porytiles2;

        const bool no_color = !isatty(STDERR_FILENO);
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

        std::filesystem::path project_root{"."};
        std::filesystem::path fieldmap_header_root_relative{"include/fieldmap.h"};

        // Build provider chain with CLI at highest priority
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage_));  // NEW - highest priority
        providers.push_back(std::make_unique<YamlFileProvider>(text_formatter, diag.get(), project_root));
        providers.push_back(std::make_unique<HeaderDefineProvider>(project_root, fieldmap_header_root_relative, text_formatter));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter, std::move(providers)};

        // ... rest unchanged
    }

  private:
    CliOptionStorage cli_storage_;  // NEW
    std::string tileset_name_;
};
```

---

## Implementation Plan

### Phase 1: Schema Extension

**File:** `Porytiles2/config_templates/config_schema.yaml`

Add CLI-specific metadata fields to each config value:

```yaml
cli_skip: false  # Set true to exclude from CLI
```

**Values to mark `cli_skip: true`:**
- `pal_hints` (`std::vector<PaletteHint>`) - too complex for CLI

### Phase 2: Code Generation Templates

**New Jinja2 templates to create:**

| Template | Output | Purpose |
|----------|--------|---------|
| `cli_option_storage.hpp.jinja2` | `include/porytiles2/infra/cli/cli_option_storage.hpp` | Storage struct |
| `cli_option_provider.hpp.jinja2` | `include/porytiles2/infra/config/cli_option_provider.hpp` | Provider header |
| `cli_option_provider.cpp.jinja2` | `lib/infra/config/cli_option_provider.cpp` | Provider implementation |
| `cli_option_registration.hpp.jinja2` | `include/porytiles2/infra/cli/cli_option_registration.hpp` | Registration function header |
| `cli_option_registration.cpp.jinja2` | `lib/infra/cli/cli_option_registration.cpp` | Registration implementation |
| `cli_completion_data.hpp.jinja2` | `include/porytiles2/infra/cli/cli_completion_data.hpp` | Completion metadata |

### Phase 3: Update generate_config.py

Add new template mappings:

```python
TEMPLATES = [
    # ... existing templates ...
    ('cli_option_storage.hpp.jinja2', 'include/porytiles2/infra/cli/cli_option_storage.hpp'),
    ('cli_option_provider.hpp.jinja2', 'include/porytiles2/infra/config/cli_option_provider.hpp'),
    ('cli_option_provider.cpp.jinja2', 'lib/infra/config/cli_option_provider.cpp'),
    ('cli_option_registration.hpp.jinja2', 'include/porytiles2/infra/cli/cli_option_registration.hpp'),
    ('cli_option_registration.cpp.jinja2', 'lib/infra/cli/cli_option_registration.cpp'),
    ('cli_completion_data.hpp.jinja2', 'include/porytiles2/infra/cli/cli_completion_data.hpp'),
]
```

### Phase 4: Integrate into Commands

Update each command class to:
1. Add `CliOptionStorage` member
2. Call `register_config_options()` in constructor
3. Add `CliOptionProvider` as first provider in `Run()`

### Phase 5: Shell Completion

Create manual files:
- `tools/driver/command_completion.hpp`
- `tools/driver/command_completion.cpp`
- `tools/driver/manual_cli_options.hpp` (non-config options metadata)

Add `CompletionCommand` to `main.cpp`.

### Directory Structure After Implementation

```
Porytiles2/
├── config_templates/
│   ├── _macros.jinja2                          # Existing
│   ├── config_schema.yaml                      # Modified (add cli_skip)
│   ├── cli_option_storage.hpp.jinja2           # NEW
│   ├── cli_option_provider.hpp.jinja2          # NEW
│   ├── cli_option_provider.cpp.jinja2          # NEW
│   ├── cli_option_registration.hpp.jinja2      # NEW
│   ├── cli_option_registration.cpp.jinja2      # NEW
│   └── cli_completion_data.hpp.jinja2          # NEW
├── include/porytiles2/infra/cli/               # NEW directory
│   ├── cli_option_storage.hpp                  # Generated
│   ├── cli_option_registration.hpp             # Generated
│   └── cli_completion_data.hpp                 # Generated
├── include/porytiles2/infra/config/
│   ├── config_provider.hpp                     # Existing
│   ├── cli_option_provider.hpp                 # Generated (NEW)
│   └── ...                                     # Other existing providers
├── lib/infra/cli/                              # NEW directory
│   └── cli_option_registration.cpp             # Generated
├── lib/infra/config/
│   ├── cli_option_provider.cpp                 # Generated (NEW)
│   └── ...                                     # Other existing providers
└── tools/driver/
    ├── main.cpp                                # Modified (add CompletionCommand)
    ├── command.hpp                             # Existing
    ├── command_compile_tileset.hpp             # Modified
    ├── command_completion.hpp                  # NEW (manual)
    ├── command_completion.cpp                  # NEW (manual)
    └── manual_cli_options.hpp                  # NEW (manual)
```

---

## Design Decisions & Rationale

### Decision 1: Stay with CLI11

**Decision:** Keep CLI11 as the CLI parsing library.

**Rationale:**
- CLI11 is mature, well-documented, and actively maintained
- Migration to an alternative library would require significant effort
- Shell completion can be implemented manually as a standard `completion` subcommand
- The existing CLI11 infrastructure is working well for other needs

**Alternatives rejected:**
- **spevnev/args:** Has multi-shell completion but less mature; would require learning new API
- **Taywee/args:** Only supports Bash completion

### Decision 2: Provider Priority Order

**Decision:** CLI > YAML > Header > Default

**Rationale:**
- Explicit command-line input should override everything else
- This matches user expectations from other CLI tools
- Allows users to temporarily override any setting without editing files

### Decision 3: Scope-Agnostic CLI Options

**Decision:** CLI options apply globally regardless of `ConfigScopeType`.

**Rationale:**
- CLI is a global override mechanism
- Per-scope CLI options would be confusing (e.g., `--tileset=MyTileset --num-pals-total=8 --tileset=OtherTileset --num-pals-total=6`)
- YAML files already handle per-scope configuration well

### Decision 4: Hyphenated Enum Strings

**Decision:** Use hyphenated strings for enum CLI values (e.g., `--tiles-pal-mode=true-color`).

**Rationale:**
- Matches common CLI conventions
- Consistent with flag naming style (`--tiles-pal-mode` not `--tiles_pal_mode`)
- More readable in shell commands

### Decision 5: Skip Complex Types

**Decision:** Skip `std::vector<PaletteHint>` (set `cli_skip: true`).

**Rationale:**
- Too complex to express as a CLI option
- Would require complex quoting/escaping
- YAML is the right tool for complex structured data
- Users can still configure this via config files

### Decision 6: Code Generation Over Manual Definition

**Decision:** Generate CLI option registration code from `config_schema.yaml`.

**Rationale:**
- Ensures CLI options stay synchronized with config values
- Adding new config values automatically adds CLI support
- Reduces duplication and potential for drift
- Follows established pattern in the codebase

### Decision 7: Runtime Completion Generation

**Decision:** Generate shell completion scripts at runtime via `completion` subcommand.

**Rationale:**
- Standard practice (kubectl, docker, gh, etc.)
- Completions are always in sync with actual CLI options
- No build-time complexity
- Easy for users to regenerate after updates

---

## Alternatives Considered

### Alternative 1: Use spevnev/args Library

**Description:** Replace CLI11 with spevnev/args which has built-in multi-shell completion.

**Pros:**
- Built-in completion support for Bash, Zsh, Fish
- Simpler completion setup

**Cons:**
- Migration cost
- Less mature library
- Would require rewriting all existing CLI code
- CLI11 is working well for everything except completion

**Why rejected:** The migration cost outweighs the benefit. Manual completion implementation is straightforward.

### Alternative 2: Build-Time Completion Generation

**Description:** Generate completion scripts during CMake build phase.

**Pros:**
- Completions available immediately after build
- Can be distributed with packages

**Cons:**
- More complex build system integration
- Less flexible for users
- Scripts may get out of sync if binary is updated separately

**Why rejected:** Runtime generation is the standard pattern and simpler to implement.

### Alternative 3: Per-Command Option Registration

**Description:** Each command explicitly lists which config options it supports.

**Pros:**
- Fine-grained control over which options appear on which commands
- Potentially cleaner `--help` output per command

**Cons:**
- More boilerplate code
- Risk of forgetting to add options to commands
- Most commands need most config options anyway

**Why rejected:** All config options should be available on commands that use the config system. Selective registration adds maintenance burden.

### Alternative 4: Separate CLI Config File

**Description:** Have the CLI generate a separate CLI-specific config file.

**Pros:**
- Clean separation of CLI and config system

**Cons:**
- Adds unnecessary complexity
- Duplicates information already in schema
- Goes against the unified config system design

**Why rejected:** The ConfigProvider abstraction already handles this cleanly.

---

## Verification Plan

### Build Verification

```bash
cmake --build clion-build-debug -j7 > /tmp/build.log 2>&1
echo $?  # Should be 0
```

### Unit Tests

Add GoogleTest cases for `CliOptionProvider`:

```cpp
TEST(CliOptionProviderTest, ReturnsNotProvidedWhenOptionalEmpty) {
    CliOptionStorage storage{};
    CliOptionProvider provider{storage};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST(CliOptionProviderTest, ReturnsValidWhenOptionalHasValue) {
    CliOptionStorage storage{};
    storage.num_tiles_in_primary = 256;
    CliOptionProvider provider{storage};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 256);
}
```

### Integration Test

```bash
# Test CLI override
./porytiles compile-tileset TestTileset --num-pals-total=8
# Should use CLI value (8) instead of header/yaml/default

# Test enum option
./porytiles compile-tileset TestTileset --tiles-pal-mode=true-color
# Should accept hyphenated enum value
```

### Completion Test

```bash
# Generate and source completions
eval "$(./porytiles completion bash)"

# Test option completion
./porytiles compile-tileset --tiles-pal-<TAB>
# Should complete to --tiles-pal-mode

# Test enum value completion
./porytiles compile-tileset --tiles-pal-mode=<TAB>
# Should show: true-color  greyscale

# Test subcommand completion
./porytiles <TAB>
# Should show: compile-tileset  create-tileset  import-tileset  decompile-tileset  completion
```

---

## References

### Codebase Files

- Config schema: `Porytiles2/config_templates/config_schema.yaml`
- Template macros: `Porytiles2/config_templates/_macros.jinja2`
- Config generator: `Scripts/generate_config.py`
- Current CLI: `Porytiles2/tools/driver/`
- ConfigProvider base: `Porytiles2/include/porytiles2/infra/config/config_provider.hpp`
- YamlFileProvider: `Porytiles2/include/porytiles2/infra/config/yaml_file_provider.hpp`
- LazyLayeredConfig: `Porytiles2/include/porytiles2/infra/config/lazy_layered_config.hpp`

### External Resources

- [CLI11 GitHub Repository](https://github.com/CLIUtils/CLI11)
- [CLI11 Autocomplete Issue #343](https://github.com/CLIUtils/CLI11/issues/343)
- [Taywee/args](https://github.com/Taywee/args)
- [spevnev/args](https://github.com/spevnev/args)
- [Mill Build Tool - Tab Completions](https://mill-build.org/blog/14-bash-zsh-completion.html)
- [Bash Programmable Completion](https://www.gnu.org/software/bash/manual/html_node/A-Programmable-Completion-Example.html)
- [Cobra Shell Completion Guide](https://cobra.dev/docs/how-to-guides/shell-completion/)
- [Click Shell Completion](https://click.palletsprojects.com/en/stable/shell-completion/)
