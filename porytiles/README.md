# Porytiles 2
**Major Version 2** (MV2) is the next-generation Porytiles offering,
featuring significant enhancements and a very different UX.
Users can access MV2 functionality via the new driver, `porytiles`.
MV2's code lives in this `Porytiles` directory,
and the driver code lives in the `tools/driver` subtree.
MV2 is tested via GoogleTest: the tests live in the `tests` subtree.

Unlike MV1, MV2 uses a library-based architecture informed by domain-driven design principles.
This should make it much easier for other developers
to integrate with their own tooling.
Our long-term goal is to integrate the core Porytiles functionality
directly into Porymap.

## Configuration System

Porytiles uses a code generation system for configuration classes.
Configuration values are defined in YAML and C++ code is auto-generated using Jinja2 templates.

**See [`config_templates/README.md`](config_templates/README.md) for full documentation**, including:
- Directory structure and template organization
- How to add new config values
- How to add enum-based config values
- Validator usage

To regenerate configuration code:

```bash
uv run scripts/generate_config.py
```
