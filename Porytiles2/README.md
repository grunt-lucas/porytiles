# Porytiles 2
**Major Version 2** (MV2) is the next-generation Porytiles offering,
featuring significant enhancements and a very different UX.
Users can access MV2 functionality via the new driver, `porytiles2`.
MV2's code lives in this `Porytiles2` directory,
and the driver code lives in the `tools/driver` subtree.
MV2 is tested via GoogleTest: the tests live in the `tests` subtree.

Unlike MV1, MV2 uses a library-based architecture informed by domain-driven design principles.
This should make it much easier for other developers
to integrate with their own tooling.
Our long-term goal is to integrate the core Porytiles functionality
directly into Porymap.

## Configuration System Setup

Porytiles2 uses a code generation system for configuration classes based on Python and Jinja2.
Configuration values are defined in `config_templates/config_schema.yaml` and C++ code is auto-generated.

### Prerequisites

Install [uv](https://docs.astral.sh/uv/getting-started/installation/) - a fast Python package manager:

```bash
# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Or via Homebrew
brew install uv
```

### Regenerating configuration code

The CMake build system automatically regenerates configuration code when needed.
If you modify `config_templates/config_schema.yaml` or any template files,
the next build will regenerate the affected C++ headers and implementation files.

To manually regenerate configuration code:

```bash
# From the project root directory
uv run Scripts/generate_config.py
```
