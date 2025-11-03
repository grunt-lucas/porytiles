#!/usr/bin/env python3
"""
Generate configuration code from YAML schema and Jinja2 templates.

This script reads config_schema.yaml and generates C++ configuration code using
Jinja2 templates. It is designed to be run either manually or automatically by
CMake during the build process.
"""

import sys
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape


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
