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

### Setting up the Python environment

On a new workstation, you'll need to set up the Python virtual environment:

```bash
# From the project root directory
python3.13 -m venv .venv
source .venv/bin/activate
pip install -r Scripts/requirements.txt
```

### Regenerating configuration code

The CMake build system automatically regenerates configuration code when needed.
If you modify `config_templates/config_schema.yaml` or any template files,
the next build will regenerate the affected C++ headers and implementation files.

To manually regenerate configuration code:

```bash
# From the project root directory
source .venv/bin/activate
python Scripts/generate_config.py
```
