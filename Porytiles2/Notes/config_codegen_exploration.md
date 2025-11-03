# Code Generation for Layered Config System

## Problem Statement

Adding a new configuration value currently requires editing multiple files:
1. Layer config interface (DomainConfig/AppConfig/InfraConfig) - add pure virtual method
2. LazyLayeredConfig header - add override declaration
3. LazyLayeredConfig implementation - implement the method
4. ConfigProvider header - add virtual method with default implementation
5. Each concrete provider (DefaultProvider, YamlFileProvider, etc.) - add override

This is tedious and error-prone. We want to define each config value **once** and generate all the boilerplate automatically.

## Solution: Python Script + Jinja2 Templates

### Overview

Define the configuration schema in YAML, create Jinja2 templates for each generated C++ file, and run a Python script at build time to generate all the boilerplate code. This approach provides clean separation of concerns: the schema defines *what* to generate, templates define *how* to generate it, and the script orchestrates the generation process.

### Why Jinja2?

**Advantages:**
- **Very flexible and powerful** - can handle complex generation logic
- **Clean separation** - schema, templates, and generated code are completely separate
- **Easy to understand** - templates look very similar to the C++ code they generate
- **Great for complex scenarios** - conditionals, loops, filters, and macros built-in
- **Multi-format generation** - can generate C++ code, documentation, JSON schemas, etc.
- **Widely used and well-documented** - industry standard for code generation
- **Type-safe** - generated C++ code is validated by the compiler
- **Good IDE support** - generated files are normal C++ files

**Trade-offs:**
- Adds Python dependency (though many C++ projects already use Python)
- Requires maintaining template files alongside code
- Slightly more complex build process (but integrates cleanly with CMake)
- Generated files must be either committed to version control or regenerated on every build

### Python Environment Setup

To avoid polluting the global Python environment, we'll use a virtual environment to manage dependencies.

**Step 1: Create virtual environment**
```bash
# From project root
python3 -m venv .venv
```

**Step 2: Activate virtual environment**
```bash
# On macOS/Linux
source .venv/bin/activate

# On Windows
.venv\Scripts\activate
```

**Step 3: Install dependencies**
```bash
pip install jinja2 pyyaml
```

**Step 4: Create requirements.txt** (for reproducibility)
```bash
pip freeze > Scripts/requirements.txt
```

This creates a `Scripts/requirements.txt` file that looks like:
```
Jinja2==3.1.2
MarkupSafe==2.1.1
PyYAML==6.0
```

**Future setup** (for other developers or CI/CD):
```bash
source .venv/bin/activate
pip install -r Scripts/requirements.txt
```

**CMake Integration:**
The CMake configuration will automatically use Python from the virtual environment if activated, or fall back to system Python. See the CMake section below for details.

### Schema Definition

**Example Implementation:**

```yaml
# config_schema.yaml
config_values:
  - name: num_tiles_primary
    type: std::size_t
    layer: domain
    takes_tileset: true

  - name: num_tiles_total
    type: std::size_t
    layer: domain
    takes_tileset: true

  - name: tiles_pal_mode
    type: TilesPalMode
    layer: infra
    takes_tileset: true

  - name: extrinsic_transparency
    type: Rgba32
    layer: domain
    takes_tileset: true
```

```jinja2
{# templates/domain_config.hpp.jinja2 #}
#pragma once

#include <string>
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

{% for value in config_values if value.layer == 'domain' %}
    [[nodiscard]] virtual ChainableResult<ConfigValue<{{ value.type }}>>
    {{ value.name }}({% if value.takes_tileset %}const std::string &tileset{% endif %}) const = 0;

{% endfor %}
    // Hand-written derived methods...
};

} // namespace porytiles2
```

```python
# Scripts/generate_config.py
import yaml
from jinja2 import Environment, FileSystemLoader
from pathlib import Path

def generate_config_files():
    # Load schema
    with open('config_schema.yaml') as f:
        schema = yaml.safe_load(f)

    # Setup Jinja2
    env = Environment(loader=FileSystemLoader('templates'))

    # Generate each file
    templates = [
        ('domain_config.hpp.jinja2', 'include/porytiles2/domain/config/domain_config.hpp'),
        ('config_provider.hpp.jinja2', 'include/porytiles2/infra/config/config_provider.hpp'),
        ('config_provider.cpp.jinja2', 'lib/infra/config/config_provider.cpp'),
    ]

    for template_name, output_path in templates:
        template = env.get_template(template_name)
        output = template.render(**schema)
        Path(output_path).write_text(output)

if __name__ == '__main__':
    generate_config_files()
```

### CMake Integration

The build system needs to run the generation script before compiling the library. Here's how to integrate it with CMake:

```cmake
# Find Python interpreter (works with venv if activated)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Define all generated output files
set(GENERATED_CONFIG_FILES
    ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/domain/config/domain_config.hpp
    ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/app/config/app_config.hpp
    ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/infra_config.hpp
    ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/config_provider.hpp
    ${CMAKE_SOURCE_DIR}/Porytiles2/lib/infra/config/config_provider.cpp
)

# Define all template files
set(CONFIG_TEMPLATES
    ${CMAKE_SOURCE_DIR}/templates/domain_config.hpp.jinja2
    ${CMAKE_SOURCE_DIR}/templates/app_config.hpp.jinja2
    ${CMAKE_SOURCE_DIR}/templates/infra_config.hpp.jinja2
    ${CMAKE_SOURCE_DIR}/templates/config_provider.hpp.jinja2
    ${CMAKE_SOURCE_DIR}/templates/config_provider.cpp.jinja2
)

# Generate config files before building
add_custom_command(
    OUTPUT ${GENERATED_CONFIG_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
    DEPENDS
        ${CMAKE_SOURCE_DIR}/config_schema.yaml
        ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        ${CONFIG_TEMPLATES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating configuration code from templates..."
    VERBATIM
)

# Create a target that depends on generated files
add_custom_target(generate_config_code
    DEPENDS ${GENERATED_CONFIG_FILES}
)

# Make the library depend on generated code
add_dependencies(porytiles2_lib generate_config_code)
```

**How it works:**
- `find_package(Python3)` locates Python (respects activated venv)
- `add_custom_command` defines the generation process
- Lists all OUTPUT files so CMake knows what gets generated
- Lists all DEPENDS so CMake knows when to regenerate
- `VERBATIM` ensures proper command-line escaping
- `add_dependencies` ensures generation runs before compilation

**Regeneration triggers:**
The code will be regenerated automatically when:
- `config_schema.yaml` changes
- Any template file changes
- `Scripts/generate_config.py` changes
- Generated files are deleted

### Template Examples

Templates use Jinja2 syntax to generate C++ code. Here are some more detailed examples:

**domain_config.hpp.jinja2** (expanded):
```jinja2
{# templates/domain_config.hpp.jinja2 #}
#pragma once

#include <string>

#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Domain layer configuration interface.
 *
 * @details
 * Generated from config_schema.yaml - DO NOT EDIT THIS FILE DIRECTLY.
 * Edit config_schema.yaml and regenerate using Scripts/generate_config.py
 */
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

{% for value in config_values if value.layer == 'domain' %}
    /**
     * @brief Get {{ value.name }} configuration value.
     */
    [[nodiscard]] virtual ChainableResult<ConfigValue<{{ value.type }}>
    {{ value.name }}({% if value.takes_tileset %}const std::string &tileset{% endif %}) const = 0;

{% endfor %}
    // Hand-written derived/computed methods below
    // These are NOT auto-generated and should be maintained manually

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_secondary(const std::string &tileset) const;
};

} // namespace porytiles2
```

**config_provider.cpp.jinja2** (with default implementations):
```jinja2
{# templates/config_provider.cpp.jinja2 #}
#include "porytiles2/infra/config/config_provider.hpp"

namespace porytiles2 {

{% for value in config_values %}
LayerValue<{{ value.type }}> ConfigProvider::{{ value.name }}(
    {%- if value.takes_tileset %}const std::string &tileset{% endif -%}
) const {
    // Default: not provided at this layer
    return LayerValue<{{ value.type }}>::not_provided();
}

{% endfor %}

} // namespace porytiles2
```

### Generation Script

The Python script reads the schema and renders each template:

```python
# Scripts/generate_config.py (expanded version)
#!/usr/bin/env python3
"""
Generate configuration code from YAML schema and Jinja2 templates.
"""

import sys
from pathlib import Path
import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape

def generate_config_files():
    """Main generation function."""
    # Determine project root (script is in Scripts/)
    project_root = Path(__file__).parent.parent

    # Load schema
    schema_path = project_root / 'config_schema.yaml'
    print(f"Loading schema from: {schema_path}")
    with open(schema_path) as f:
        schema = yaml.safe_load(f)

    # Setup Jinja2 environment
    template_dir = project_root / 'templates'
    env = Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(),
        trim_blocks=True,
        lstrip_blocks=True,
    )

    # Define template -> output mappings
    templates = [
        ('domain_config.hpp.jinja2',
         'Porytiles2/include/porytiles2/domain/config/domain_config.hpp'),
        ('app_config.hpp.jinja2',
         'Porytiles2/include/porytiles2/app/config/app_config.hpp'),
        ('infra_config.hpp.jinja2',
         'Porytiles2/include/porytiles2/infra/config/infra_config.hpp'),
        ('config_provider.hpp.jinja2',
         'Porytiles2/include/porytiles2/infra/config/config_provider.hpp'),
        ('config_provider.cpp.jinja2',
         'Porytiles2/lib/infra/config/config_provider.cpp'),
    ]

    # Generate each file
    for template_name, output_rel_path in templates:
        print(f"Generating: {output_rel_path}")

        template = env.get_template(template_name)
        output = template.render(**schema)

        output_path = project_root / output_rel_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(output)

    print("✓ Configuration code generation complete")

if __name__ == '__main__':
    try:
        generate_config_files()
    except Exception as e:
        print(f"Error generating config files: {e}", file=sys.stderr)
        sys.exit(1)
```

### Generated Files: Commit or Regenerate?

**Option A: Commit generated files**
- **Pros:** Faster builds, no Python dependency for most developers, easier to review changes
- **Cons:** Merge conflicts in generated files, must remember to regenerate before committing

**Option B: Always regenerate**
- **Pros:** Generated files always in sync, no merge conflicts, smaller repo
- **Cons:** Requires Python setup for all developers, slightly slower builds

**Recommendation:** Start with Option A (commit generated files) for simplicity. The generation script is fast, and committing generated files makes it easier for developers who just want to build without setting up Python. Add a pre-commit check to ensure generated files are up-to-date.

## Implementation Roadmap

### Phase 1: Setup Infrastructure (1-2 hours)
1. Create Python virtual environment
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install jinja2 pyyaml
   pip freeze > Scripts/requirements.txt
   ```

2. Create `config_schema.yaml` at project root
   - Start with current config values from existing code
   - Define schema structure (name, type, layer, takes_tileset)

3. Create `templates/` directory at project root
   - Will hold all `.jinja2` template files

4. Add `.venv/` to `.gitignore`

### Phase 2: First Template End-to-End (2-3 hours)
1. Create `templates/domain_config.hpp.jinja2`
   - Port existing `DomainConfig` structure
   - Add Jinja2 loops for config values
   - Keep derived/computed methods as hand-written section

2. Write `Scripts/generate_config.py` skeleton
   - Load YAML schema
   - Setup Jinja2 environment
   - Generate single file (DomainConfig)

3. Test generation manually
   ```bash
   python Scripts/generate_config.py
   ```

4. Verify generated code compiles

### Phase 3: CMake Integration (1 hour)
1. Add CMake generation commands to `Porytiles2/CMakeLists.txt`
   - `find_package(Python3)`
   - `add_custom_command` for generation
   - `add_custom_target` for dependency tracking
   - `add_dependencies` to link with library

2. Test full build process
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

3. Verify regeneration triggers work

### Phase 4: Expand to All Config Files (3-4 hours)
1. Create templates for remaining files:
   - `templates/app_config.hpp.jinja2`
   - `templates/infra_config.hpp.jinja2`
   - `templates/config_provider.hpp.jinja2`
   - `templates/config_provider.cpp.jinja2`

2. Update `Scripts/generate_config.py` with all template mappings

3. Generate all files and verify compilation

4. Run tests to ensure behavior unchanged

### Phase 5: Handle Special Cases (2-3 hours)
1. Preserve hand-written derived/computed methods
   - Add special sections in templates for manual code
   - Document which methods are auto-generated vs manual

2. Handle provider-specific overrides
   - YamlFileProvider will need custom implementations
   - Keep these in separate hand-written files

3. Add validation to generation script
   - Check for duplicate config names
   - Validate type names
   - Ensure layer values are valid

### Phase 6: Documentation & Polish (1-2 hours)
1. Add generation header comments to templates
   - "DO NOT EDIT - Generated from config_schema.yaml"
   - Link to schema file and generation script

2. Generate Doxygen comments in templates

3. Update build documentation (README/wiki)
   - Python environment setup instructions
   - How to add new config values
   - How to regenerate manually

4. Consider adding pre-commit hook to check generated files are up-to-date

### Total Estimated Time: 10-15 hours

## Directory Structure

After implementation, the project structure will look like:

```
porytiles/
├── .venv/                                  # Python virtual environment (gitignored)
├── config_schema.yaml                      # Single source of truth for config
├── templates/                              # Jinja2 templates
│   ├── domain_config.hpp.jinja2
│   ├── app_config.hpp.jinja2
│   ├── infra_config.hpp.jinja2
│   ├── config_provider.hpp.jinja2
│   └── config_provider.cpp.jinja2
├── Scripts/
│   ├── generate_config.py                  # Generation script
│   └── requirements.txt                    # Python dependencies
└── Porytiles2/
    ├── CMakeLists.txt                      # Updated with generation commands
    ├── include/porytiles2/
    │   ├── domain/config/
    │   │   └── domain_config.hpp           # GENERATED - committed to repo
    │   ├── app/config/
    │   │   └── app_config.hpp              # GENERATED - committed to repo
    │   └── infra/config/
    │       ├── infra_config.hpp            # GENERATED - committed to repo
    │       └── config_provider.hpp         # GENERATED - committed to repo
    └── lib/infra/config/
        └── config_provider.cpp             # GENERATED - committed to repo
```

## Adding New Config Values

Once the system is in place, adding a new config value is simple:

1. **Edit `config_schema.yaml`:**
   ```yaml
   config_values:
     - name: my_new_setting
       type: int
       layer: domain
       takes_tileset: false
   ```

2. **Regenerate code:**
   ```bash
   python Scripts/generate_config.py
   # Or just rebuild (CMake will regenerate automatically)
   cmake --build build
   ```

3. **Implement provider logic** (if needed):
   - Add override in YamlFileProvider to read from YAML
   - Add default value in DefaultProvider if applicable

4. **Done!** No need to manually edit 5+ files.

## References

- **Jinja2 Documentation:** https://jinja.palletsprojects.com/
- **Jinja2 Template Designer:** https://jinja.palletsprojects.com/en/3.1.x/templates/
- **PyYAML Documentation:** https://pyyaml.org/wiki/PyYAMLDocumentation
- **CMake add_custom_command:** https://cmake.org/cmake/help/latest/command/add_custom_command.html
- **Python venv Guide:** https://docs.python.org/3/library/venv.html
