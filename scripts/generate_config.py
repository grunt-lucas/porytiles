#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = [
#     "Jinja2>=3.1.6",
#     "PyYAML>=6.0.3",
# ]
# ///
"""
Generate configuration code from YAML schema and Jinja2 templates.

This script reads config_schema.yaml and generates C++ configuration code using
Jinja2 templates. It is designed to be run either manually or automatically by
CMake during the build process.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape

# Reuse the clang-format resolution logic from format.py (same directory).
# Skip bytecode caching so the import doesn't create scripts/__pycache__/.
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from format import find_clang_format  # noqa: E402


def extract_all_yaml_paths(config_values):
    """
    Extract all valid YAML paths from config values, including intermediate paths.

    For path "fieldmap.num_tiles_in_primary", generates both:
    - "fieldmap" (valid as an intermediate node)
    - "fieldmap.num_tiles_in_primary" (valid as a leaf node)

    Returns a sorted list for deterministic output.
    """
    paths = set()
    for config in config_values:
        if "yaml_path" in config:
            full_path = config["yaml_path"]
            parts = full_path.split(".")
            # Add all prefixes and the full path
            for i in range(1, len(parts) + 1):
                paths.add(".".join(parts[:i]))
    return sorted(paths)


def extract_yaml_map_prefixes(config_values):
    """
    Extract YAML path prefixes for config entries with yaml_is_map: true.

    These prefixes identify map-type config values whose children are dynamic
    keys (e.g., animation names) rather than fixed config paths. The YAML path
    validator uses these to skip dynamic children instead of emitting unknown
    key warnings.

    Returns a sorted list for deterministic output.
    """
    prefixes = set()
    for config in config_values:
        if config.get("yaml_is_map", False) and "yaml_path" in config:
            prefixes.add(config["yaml_path"])
    return sorted(prefixes)


# Non-enum types the CLI templates know how to register. Enum types are handled separately, via
# enum_type_map. This list must stay in sync with the type dispatch chains in
# infra/cli/cli_option_registration.cpp.jinja2 and infra/cli/cli_completion_data.hpp.jinja2.
#
# The dispatch chains match on the literal spelling of the type, so a new spelling of an
# already-handled concept (e.g. std::optional<std::size_t> next to std::optional<std::uint32_t>)
# is a new case. Registration would emit a skip comment and completion would emit nothing at all,
# producing a flag that is missing from --help and rejected when passed. validate_cli_option_types()
# turns that into a generation error instead.
CLI_HANDLED_TYPES = [
    "bool",
    "std::vector<std::string>",
    "std::size_t",
    "std::string",
    "std::optional<std::size_t>",
    "std::optional<std::uint32_t>",
    "Rgba32",
]


def validate_cli_option_types(config_values, enum_type_map):
    """
    Fail generation if a config value would get a CLI flag that the templates cannot register.

    Every config value without yaml_only: true is expected to declare a cli_option and to have a
    type the CLI templates dispatch on. A type outside CLI_HANDLED_TYPES and outside enum_type_map
    is silently skipped by those templates, so catch it here where the error is actionable.
    """
    for config_value in config_values:
        if config_value.get("yaml_only"):
            continue

        name = config_value.get("symbol", config_value.get("canonical_name", "<unnamed config value>"))
        value_type = config_value.get("type")

        if not config_value.get("cli_option"):
            print(
                f"Error: Config value '{name}' is missing required 'cli_option' field",
                file=sys.stderr,
            )
            print(
                "  Hint: Add a 'cli_option', or mark the value 'yaml_only: true' if it has no CLI flag",
                file=sys.stderr,
            )
            sys.exit(1)

        if value_type not in CLI_HANDLED_TYPES and value_type not in enum_type_map:
            print(
                f"Error: Config value '{name}' declares cli_option "
                f"'{config_value['cli_option']}' but its type '{value_type}' is not handled "
                f"by the CLI templates",
                file=sys.stderr,
            )
            print(
                f"  Hint: Handled types are: {', '.join(CLI_HANDLED_TYPES)}, or any enum in enum_types",
                file=sys.stderr,
            )
            print(
                "  Hint: To support a new type, add a branch to the dispatch chains in "
                "infra/cli/cli_option_registration.cpp.jinja2 and infra/cli/cli_completion_data.hpp.jinja2, "
                "add the accessor to infra/config/cli_option_provider.cpp.jinja2, then list the type in "
                "CLI_HANDLED_TYPES",
                file=sys.stderr,
            )
            print(
                "  Hint: Or mark the value 'yaml_only: true' if it should not have a CLI flag",
                file=sys.stderr,
            )
            sys.exit(1)


def process_enum_types(schema):
    """
    Process enum_types from schema to compute derived fields and create lookup map.

    For each enum value, computes:
    - fuzzy_names if not provided (defaults to [name, kebab-case-of-name])

    Creates enum_type_map for template lookup with cli_type_name for each enum.

    Returns the enum_type_map dict keyed by cpp_type.
    """
    enum_types = schema.get("enum_types", [])
    enum_type_map = {}

    for enum in enum_types:
        # Compute fuzzy_names for each value if not provided
        cli_names = []
        for val in enum["enum_values"]:
            # Compute kebab-case name for CLI display
            kebab_name = val["name"].replace("_", "-")
            cli_names.append(kebab_name)

            # Compute fuzzy_names if not provided
            # Default: include name and kebab-case-of-name
            if "fuzzy_names" not in val:
                fuzzy_set = {val["name"], kebab_name}
                val["fuzzy_names"] = sorted(fuzzy_set)

        # Create CLI type name string like "{locked|patch|optimize}"
        enum["cli_type_name"] = "{" + "|".join(cli_names) + "}"

        # Add to lookup map
        enum_type_map[enum["cpp_type"]] = enum

    return enum_type_map


def format_generated_files(generated_paths):
    """
    Run clang-format over the files the generator just wrote.

    The Jinja2 templates produce nearly-formatted output, but the committed files are
    clang-formatted, so without this step every regeneration dirties the tree with
    whitespace-only diffs. Formatting is cosmetic, and CMake runs this script as a
    build step, so a missing clang-format binary or a clang-format failure (e.g. a
    binary too old for our .clang-format options) is a warning, never a build failure.
    """
    clang_format = find_clang_format()
    if clang_format is None:
        print(
            "Warning: clang-format not found on PATH; skipping formatting of generated files. "
            "Run 'uv run scripts/format.py' once clang-format is installed.",
            file=sys.stderr,
        )
        return

    print(f"Formatting {len(generated_paths)} generated files with {clang_format}")
    cmd = [clang_format, "-style=file", "-i", *[str(p) for p in generated_paths]]
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(
            f"Warning: {clang_format} failed; leaving generated files unformatted. "
            "Generation itself succeeded, so the build can proceed.",
            file=sys.stderr,
        )
        return
    print("✓ Formatted generated files")


def generate_config_files(run_formatter=True):
    """Main generation function."""
    # Determine project root (script is in scripts/)
    project_root = Path(__file__).resolve().parent.parent
    print(f"Project root: {project_root}")

    # Load schema from porytiles/config_templates/
    schema_path = project_root / "porytiles" / "config_templates" / "config_schema.yaml"
    print(f"Loading schema from: {schema_path}")

    if not schema_path.exists():
        print(f"Error: Schema file not found at {schema_path}", file=sys.stderr)
        sys.exit(1)

    with open(schema_path) as f:
        schema = yaml.safe_load(f)

    # Validate schema
    if "config_values" not in schema:
        print("Error: Schema must contain 'config_values' key", file=sys.stderr)
        sys.exit(1)

    # Validate that all config values have required fields
    for idx, config_value in enumerate(schema["config_values"]):
        name = config_value.get("symbol", config_value.get("canonical_name", f"<unnamed config value at index {idx}>"))

        # Check for validators field
        if "validators" not in config_value:
            print(
                f"Error: Config value '{name}' is missing required 'validators' field",
                file=sys.stderr,
            )
            print(
                "  Hint: Add 'validators: []' if no validators are needed",
                file=sys.stderr,
            )
            sys.exit(1)

        # Check for cross_field_validators field
        if "cross_field_validators" not in config_value:
            print(
                f"Error: Config value '{name}' is missing required 'cross_field_validators' field",
                file=sys.stderr,
            )
            print(
                "  Hint: Add 'cross_field_validators: []' if no cross-field validators are needed",
                file=sys.stderr,
            )
            sys.exit(1)

        # Validate cross-field validator references
        for cross_validator in config_value.get("cross_field_validators", []):
            # Check if it's a comparison validator
            if cross_validator.startswith("compare_") and ":" in cross_validator:
                parts = cross_validator.split(":")
                if len(parts) != 2:
                    print(
                        f"Error: Invalid cross-field validator '{cross_validator}' in config value '{name}'",
                        file=sys.stderr,
                    )
                    print(
                        "  Hint: Comparison validators must be in format 'compare_<op>:<field_name>'",
                        file=sys.stderr,
                    )
                    sys.exit(1)

                operator_name, other_field_name = parts
                valid_operators = [
                    "compare_greater_than",
                    "compare_less_than",
                    "compare_greater_equal",
                    "compare_less_equal",
                    "compare_equal",
                    "compare_not_equal",
                ]
                if operator_name not in valid_operators:
                    print(
                        f"Error: Unknown comparison operator '{operator_name}' in config value '{name}'",
                        file=sys.stderr,
                    )
                    print(
                        f"  Hint: Valid operators are: {', '.join(valid_operators)}",
                        file=sys.stderr,
                    )
                    sys.exit(1)

                # Check that the referenced field exists in the same layer
                current_layer = config_value.get("layer")
                other_field_exists = False
                for other_config in schema["config_values"]:
                    if other_config.get("symbol") == other_field_name and other_config.get("layer") == current_layer:
                        other_field_exists = True
                        break

                if not other_field_exists:
                    print(
                        f"Error: Cross-field validator in '{name}' references unknown field '{other_field_name}' in layer '{current_layer}'",
                        file=sys.stderr,
                    )
                    print(
                        "  Hint: Cross-field validators can only reference fields in the same layer",
                        file=sys.stderr,
                    )
                    sys.exit(1)

    print(f"Loaded {len(schema['config_values'])} config values from schema")

    # Extract all valid YAML paths for unknown key detection
    all_yaml_paths = extract_all_yaml_paths(schema["config_values"])
    schema["all_yaml_paths"] = all_yaml_paths
    print(f"Extracted {len(all_yaml_paths)} valid YAML paths")

    # Extract YAML map prefixes for map-type config values
    yaml_map_prefixes = extract_yaml_map_prefixes(schema["config_values"])
    schema["yaml_map_prefixes"] = yaml_map_prefixes
    print(f"Extracted {len(yaml_map_prefixes)} YAML map prefixes")

    # Process enum types and create lookup map
    enum_type_map = process_enum_types(schema)
    schema["enum_type_map"] = enum_type_map
    print(f"Processed {len(enum_type_map)} enum types")

    # Runs after enum processing because enum types are valid CLI option types
    validate_cli_option_types(schema["config_values"], enum_type_map)
    print("Validated CLI option types")

    # Setup Jinja2 environment with new template directory
    template_dir = project_root / "porytiles" / "config_templates"
    env = Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(),
        trim_blocks=True,
        lstrip_blocks=True,
    )

    # Custom filter to convert PascalCase to snake_case
    def pascal_to_snake(value):
        """Convert PascalCase to snake_case (e.g., ArtifactEditMode -> artifact_edit_mode)."""
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', value)
        return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

    env.filters['snake_case'] = pascal_to_snake

    # Define template -> output mappings
    # Templates are organized by layer: domain/, app/, infra/
    templates = [
        # Layer interfaces
        (
            "domain/config/domain_config.hpp.jinja2",
            "porytiles/include/porytiles/domain/config/domain_config.hpp",
        ),
        (
            "app/config/app_config.hpp.jinja2",
            "porytiles/include/porytiles/app/config/app_config.hpp",
        ),
        (
            "infra/config/infra_config.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/infra_config.hpp",
        ),
        # LazyLayeredConfig
        (
            "infra/config/lazy_layered_config.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/lazy_layered_config.hpp",
        ),
        (
            "infra/config/lazy_layered_config.cpp.jinja2",
            "porytiles/lib/infra/config/lazy_layered_config.cpp",
        ),
        # ConfigProvider base class
        (
            "infra/config/config_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/config_provider.hpp",
        ),
        (
            "infra/config/config_provider.cpp.jinja2",
            "porytiles/lib/infra/config/config_provider.cpp",
        ),
        # DefaultProvider
        (
            "infra/config/default_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/default_provider.hpp",
        ),
        (
            "infra/config/default_provider.cpp.jinja2",
            "porytiles/lib/infra/config/default_provider.cpp",
        ),
        # YamlFileProvider
        (
            "infra/config/yaml_file_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/yaml_file_provider.hpp",
        ),
        (
            "infra/config/yaml_file_provider.cpp.jinja2",
            "porytiles/lib/infra/config/yaml_file_provider.cpp",
        ),
        # HeaderDefineProvider
        (
            "infra/config/header_define_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/header_define_provider.hpp",
        ),
        (
            "infra/config/header_define_provider.cpp.jinja2",
            "porytiles/lib/infra/config/header_define_provider.cpp",
        ),
        # OverrideConfigProvider
        (
            "infra/config/override_config_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/override_config_provider.hpp",
        ),
        (
            "infra/config/override_config_provider.cpp.jinja2",
            "porytiles/lib/infra/config/override_config_provider.cpp",
        ),
        # Valid YAML paths for unknown key detection
        (
            "infra/config/valid_yaml_paths.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/valid_yaml_paths.hpp",
        ),
        # CLI Option System
        (
            "infra/cli/cli_option_storage.hpp.jinja2",
            "porytiles/include/porytiles/infra/cli/cli_option_storage.hpp",
        ),
        (
            "infra/config/cli_option_provider.hpp.jinja2",
            "porytiles/include/porytiles/infra/config/cli_option_provider.hpp",
        ),
        (
            "infra/config/cli_option_provider.cpp.jinja2",
            "porytiles/lib/infra/config/cli_option_provider.cpp",
        ),
        (
            "infra/cli/cli_option_registration.hpp.jinja2",
            "porytiles/include/porytiles/infra/cli/cli_option_registration.hpp",
        ),
        (
            "infra/cli/cli_option_registration.cpp.jinja2",
            "porytiles/lib/infra/cli/cli_option_registration.cpp",
        ),
        (
            "infra/cli/cli_completion_data.hpp.jinja2",
            "porytiles/include/porytiles/infra/cli/cli_completion_data.hpp",
        ),
        # Test mock configs
        (
            "testing/mock_domain_config.hpp.jinja2",
            "porytiles/tests/support/mock_domain_config.hpp",
        ),
        (
            "testing/mock_infra_config.hpp.jinja2",
            "porytiles/tests/support/mock_infra_config.hpp",
        ),
        (
            "testing/mock_app_config.hpp.jinja2",
            "porytiles/tests/support/mock_app_config.hpp",
        ),
    ]

    # Track everything we write so we can format it all at the end
    generated_paths = []

    # Generate each file
    for template_name, output_rel_path in templates:
        print(f"Generating: {output_rel_path}")

        try:
            template = env.get_template(template_name)
            output = template.render(**schema)

            output_path = project_root / output_rel_path
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(output)
            generated_paths.append(output_path)

            print(f"  ✓ Successfully generated {output_path}")
        except Exception as e:
            print(f"  ✗ Failed to generate {output_rel_path}: {e}", file=sys.stderr)
            sys.exit(1)

    # Generate enum header files
    enum_template = env.get_template("domain/config/enum_type.hpp.jinja2")
    for enum in schema.get("enum_types", []):
        output_rel_path = "porytiles/include/" + enum["header_path"]
        print(f"Generating: {output_rel_path}")

        try:
            output = enum_template.render(enum=enum)

            output_path = project_root / output_rel_path
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(output)
            generated_paths.append(output_path)

            print(f"  ✓ Successfully generated {output_path}")
        except Exception as e:
            print(f"  ✗ Failed to generate {output_rel_path}: {e}", file=sys.stderr)
            sys.exit(1)

    if run_formatter:
        format_generated_files(generated_paths)

    print("✓ Configuration code generation complete")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate configuration code from YAML schema and Jinja2 templates."
    )
    parser.add_argument(
        "--no-format", action="store_true",
        help="Skip running clang-format on the generated files."
    )
    args = parser.parse_args()

    try:
        generate_config_files(run_formatter=not args.no_format)
    except Exception as e:
        print(f"Error generating config files: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc()
        sys.exit(1)
