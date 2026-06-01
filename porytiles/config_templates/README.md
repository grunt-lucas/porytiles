# Configuration Code Generation System

This directory contains the Porytiles configuration code generation system. Configuration values are defined in YAML and C++ code is auto-generated using Jinja2 templates.

## Overview

The configuration system provides:
- **Layered configuration** - Config values scoped to `domain`, `app`, or `infra` layers
- **Multiple providers** - Load config from defaults, YAML files, C header defines, or CLI options
- **Validation** - Single-value and cross-field validators ensure config integrity
- **CLI integration** - Auto-generated CLI option registration and shell completion data

## Directory Structure

```
config_templates/
├── README.md                      # This documentation
├── config_schema.yaml             # Source of truth for all config values
├── porytiles.example.yaml         # Example user config file
├── _macros.jinja2                 # Shared Jinja2 macros (used by all templates)
│
├── domain/config/                 # Domain layer templates
│   ├── domain_config.hpp.jinja2   # DomainConfig interface
│   └── enum_type.hpp.jinja2       # Generic template for enum generation
│
├── app/config/                    # App layer templates
│   └── app_config.hpp.jinja2      # AppConfig interface
│
└── infra/                         # Infrastructure layer templates
    ├── config/                    # Config infrastructure
    │   ├── infra_config.hpp.jinja2
    │   ├── lazy_layered_config.hpp.jinja2
    │   ├── lazy_layered_config.cpp.jinja2
    │   ├── config_provider.hpp.jinja2
    │   ├── config_provider.cpp.jinja2
    │   ├── default_provider.hpp.jinja2
    │   ├── default_provider.cpp.jinja2
    │   ├── yaml_file_provider.hpp.jinja2
    │   ├── yaml_file_provider.cpp.jinja2
    │   ├── header_define_provider.hpp.jinja2
    │   ├── header_define_provider.cpp.jinja2
    │   ├── valid_yaml_paths.hpp.jinja2
    │   ├── cli_option_provider.hpp.jinja2
    │   └── cli_option_provider.cpp.jinja2
    │
    └── cli/                       # CLI infrastructure
        ├── cli_option_storage.hpp.jinja2
        ├── cli_option_registration.hpp.jinja2
        ├── cli_option_registration.cpp.jinja2
        └── cli_completion_data.hpp.jinja2
```

The template directory structure mirrors the output location in the codebase (`domain/`, `app/`, `infra/`), making it easy to find and modify templates.

## How to Add a Config Value

### 1. Edit `config_schema.yaml`

Add a new entry to the `config_values` list:

```yaml
config_values:
  # ... existing values ...

  - canonical_name: My New Setting       # Human-readable name
    symbol: my_new_setting               # C++ method name (snake_case)
    yaml_path: some_section.my_setting   # YAML path for config files
    cli_option: my-setting               # CLI option (kebab-case)
    cli_desc: Custom description for help  # (optional) CLI help text
    layer: domain                        # Layer: domain, app, or infra
    type: std::size_t                    # C++ type
    parser: parse_size_t                 # Parser function name
    default_value: "100"                 # Default value (as C++ expression)
    validators: []                       # Single-value validators
    cross_field_validators: []           # Cross-field validators
```

### 2. Required Fields

| Field | Description |
|-------|-------------|
| `canonical_name` | Human-readable display name |
| `symbol` | C++ method name / programmatic identifier |
| `yaml_path` | Dot-separated YAML path (e.g., `fieldmap.num_tiles_in_primary`) |
| `cli_option` | CLI option name in kebab-case |
| `layer` | Configuration layer: `domain`, `app`, or `infra` |
| `type` | C++ type (`std::size_t`, `std::string`, or enum type) |
| `parser` | Parser function name (e.g., `parse_size_t`, `parse_string`) |
| `default_value` | Default value as a C++ expression string |
| `validators` | List of validator function names (use `[]` if none) |
| `cross_field_validators` | List of cross-field validators (use `[]` if none) |

### 3. Optional Fields

| Field | Description |
|-------|-------------|
| `header_define` | C header `#define` name for `HeaderDefineProvider` (e.g., `NUM_TILES_TOTAL`) |
| `cli_desc` | Custom CLI help description (falls back to `canonical_name`) |
| `yaml_only` | Mark config as YAML-only, excluding from CLI and other non-YAML providers (e.g., `std::vector<T>`) |

### 4. Regenerate Code

```bash
uv run scripts/generate_config.py
```

### 5. Build and Test

```bash
cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1
./porytiles-build-debug/porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1
```

## How to Add an Enum-Based Config Value

Enum types require two additions to the schema.

### 1. Define the Enum Type

Add to the `enum_types` section:

```yaml
enum_types:
  # ... existing enums ...

  - cpp_type: MyEnumType                                        # C++ class name
    header_path: porytiles/domain/config/my_enum_type.hpp      # Output path
    brief: Short description of what this enum controls.
    details: |                                                  # (optional)
      Longer description with details about usage.
    enum_values:
      - name: first_value           # C++ constant name (snake_case)
        cli_completion_name: first-value  # Name shown in CLI tab completion
        fuzzy_names: [ first-value, first_value ]  # (optional) Names accepted by parser; auto-computed if not provided
        brief: Description of this value.
      - name: second_value
        cli_completion_name: second-value  # (required) Name shown in CLI tab completion
        brief: Description of second value.  # fuzzy_names auto-computed as [second-value, second_value]
```

### 2. Add the Config Value

Add to the `config_values` section:

```yaml
config_values:
  # ... existing values ...

  - canonical_name: My Enum Setting
    symbol: my_enum_setting
    yaml_path: settings.my_enum
    cli_option: my-enum
    layer: domain
    type: MyEnumType                      # Reference the enum type
    parser: parse_my_enum_type            # Parser naming: parse_<snake_case_type>
    default_value: "MyEnumType::first_value"
    validators: []
    cross_field_validators: []
```

### 3. Regenerate and Build

```bash
uv run scripts/generate_config.py
cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1
```

The generator will create:
- `include/porytiles/domain/config/my_enum_type.hpp` with:
  - `enum class MyEnumType { ... }`
  - `MyEnumType_from_str()` parser function
  - `to_string()` function
  - `operator<<` for ostream

## Validators

### Single-Value Validators

Defined in `xcut/config/config_validators.hpp` or `<layer>/config/<layer>_config_validators.hpp`.

Example:
```yaml
validators: [ size_t_val_greater_than_zero ]
```

### Cross-Field Validators

Reference other config values in the same layer:

```yaml
cross_field_validators: [ compare_less_equal:num_tiles_total ]
```

Available comparison operators:
- `compare_greater_than:<field>`
- `compare_less_than:<field>`
- `compare_greater_equal:<field>`
- `compare_less_equal:<field>`
- `compare_equal:<field>`
- `compare_not_equal:<field>`

## Files Generated

The generator creates files in these locations:

| Template | Output |
|----------|--------|
| `domain/config/domain_config.hpp.jinja2` | `include/porytiles/domain/config/domain_config.hpp` |
| `app/config/app_config.hpp.jinja2` | `include/porytiles/app/config/app_config.hpp` |
| `infra/config/infra_config.hpp.jinja2` | `include/porytiles/infra/config/infra_config.hpp` |
| `infra/config/lazy_layered_config.*` | `include/` and `lib/` |
| `infra/config/*_provider.*` | Provider implementations |
| `infra/cli/cli_option_*` | CLI option infrastructure |
| `domain/config/enum_type.hpp.jinja2` | One header per enum in `enum_types` |

## Prerequisites

Install [uv](https://docs.astral.sh/uv/getting-started/installation/) - a fast Python package manager:

```bash
# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Or via Homebrew
brew install uv
```

The `uv run` command automatically handles Python dependencies from `pyproject.toml`.
