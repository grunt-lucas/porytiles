#!/usr/bin/env python3
"""
Generate configuration code from YAML schema and Jinja2 templates.

This script reads config_schema.yaml and generates C++ configuration code using
Jinja2 templates. It is designed to be run either manually or automatically by
CMake during the build process.
"""

import sys
import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape
from pathlib import Path


def generate_config_files():
    """Main generation function."""
    # Determine project root (script is in Scripts/)
    project_root = Path(__file__).parent.parent
    print(f"Project root: {project_root}")

    # Load schema from Porytiles2/config_templates/
    schema_path = project_root / "Porytiles2" / "config_templates" / "config_schema.yaml"
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
        name = config_value.get("name", f"<unnamed config value at index {idx}>")

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
                    if other_config.get("name") == other_field_name and other_config.get("layer") == current_layer:
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

    # Setup Jinja2 environment with new template directory
    template_dir = project_root / "Porytiles2" / "config_templates"
    env = Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(),
        trim_blocks=True,
        lstrip_blocks=True,
    )

    # Define template -> output mappings
    templates = [
        # Layer interfaces
        (
            "domain_config.hpp.jinja2",
            "Porytiles2/include/porytiles2/domain/config/domain_config.hpp",
        ),
        (
            "app_config.hpp.jinja2",
            "Porytiles2/include/porytiles2/app/config/app_config.hpp",
        ),
        (
            "infra_config.hpp.jinja2",
            "Porytiles2/include/porytiles2/infra/config/infra_config.hpp",
        ),
        # LazyLayeredConfig
        (
            "lazy_layered_config.hpp.jinja2",
            "Porytiles2/include/porytiles2/infra/config/lazy_layered_config.hpp",
        ),
        (
            "lazy_layered_config.cpp.jinja2",
            "Porytiles2/lib/infra/config/lazy_layered_config.cpp",
        ),
        # ConfigProvider base class
        (
            "config_provider.hpp.jinja2",
            "Porytiles2/include/porytiles2/infra/config/config_provider.hpp",
        ),
        (
            "config_provider.cpp.jinja2",
            "Porytiles2/lib/infra/config/config_provider.cpp",
        ),
        # DefaultProvider
        (
            "default_provider.hpp.jinja2",
            "Porytiles2/include/porytiles2/infra/config/default_provider.hpp",
        ),
        (
            "default_provider.cpp.jinja2",
            "Porytiles2/lib/infra/config/default_provider.cpp",
        ),
        # YamlFileProvider
        (
            "yaml_file_provider.hpp.jinja2",
            "Porytiles2/include/porytiles2/infra/config/yaml_file_provider.hpp",
        ),
        (
            "yaml_file_provider.cpp.jinja2",
            "Porytiles2/lib/infra/config/yaml_file_provider.cpp",
        ),
    ]

    # Generate each file
    for template_name, output_rel_path in templates:
        print(f"Generating: {output_rel_path}")

        try:
            template = env.get_template(template_name)
            output = template.render(**schema)

            output_path = project_root / output_rel_path
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(output)

            print(f"  ✓ Successfully generated {output_path}")
        except Exception as e:
            print(f"  ✗ Failed to generate {output_rel_path}: {e}", file=sys.stderr)
            sys.exit(1)

    print("✓ Configuration code generation complete")


if __name__ == "__main__":
    try:
        generate_config_files()
    except Exception as e:
        print(f"Error generating config files: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc()
        sys.exit(1)
