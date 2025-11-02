# Code Generation Solutions for Layered Config System

## Problem Statement

Adding a new configuration value currently requires editing multiple files:
1. Layer config interface (DomainConfig/AppConfig/InfraConfig) - add pure virtual method
2. LazyLayeredConfig header - add override declaration
3. LazyLayeredConfig implementation - implement the method
4. ConfigProvider header - add virtual method with default implementation
5. Each concrete provider (DefaultProvider, YamlFileProvider, etc.) - add override

This is tedious and error-prone. We want to define each config value **once** and generate all the boilerplate automatically.

## Solution Options

### Option 1: X-Macros (Preprocessor-Based)

**Overview:** Define all config values in a single macro list, then include that list multiple times with different macro definitions to generate the various pieces of code.

**Zero Dependencies:** Works with any C++ compiler, no external tools required.

**Example Implementation:**

```C++
// config_definitions.hpp
// Define all config values in one place
#define CONFIG_VALUE_LIST \
    /* name, type, layer, takes_tileset */ \
    CONFIG_VALUE(num_tiles_primary, std::size_t, domain, true) \
    CONFIG_VALUE(num_tiles_total, std::size_t, domain, true) \
    CONFIG_VALUE(num_metatiles_primary, std::size_t, domain, true) \
    CONFIG_VALUE(num_pals_total, std::size_t, domain, true) \
    CONFIG_VALUE(tiles_pal_mode, TilesPalMode, infra, true) \
    CONFIG_VALUE(extrinsic_transparency, Rgba32, domain, true) \
    CONFIG_VALUE(patch_build_enabled, bool, domain, true)
```

```C++
// domain_config.hpp
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

    // Generate pure virtual methods for domain config values
    #define CONFIG_VALUE(name, type, layer, takes_tileset) \
        BOOST_PP_IIF(BOOST_PP_EQUAL(layer, domain), \
            [[nodiscard]] virtual ChainableResult<ConfigValue<type>> \
            name(BOOST_PP_IIF(takes_tileset, const std::string &tileset, BOOST_PP_EMPTY)()) const = 0;, \
            BOOST_PP_EMPTY())

    CONFIG_VALUE_LIST

    #undef CONFIG_VALUE

    // Derived/computed methods stay hand-written
    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_secondary(const std::string &tileset) const { /* ... */ }
};
```

```C++
// config_provider.hpp
class ConfigProvider {
  public:
    virtual ~ConfigProvider() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    // Generate virtual methods with default implementations
    #define CONFIG_VALUE(name, type, layer, takes_tileset) \
        [[nodiscard]] virtual LayerValue<type> \
        name(BOOST_PP_IIF(takes_tileset, const std::string &tileset, BOOST_PP_EMPTY)()) const;

    CONFIG_VALUE_LIST

    #undef CONFIG_VALUE
};
```

```C++
// config_provider.cpp
#define CONFIG_VALUE(name, type, layer, takes_tileset) \
    LayerValue<type> ConfigProvider::name(\
        BOOST_PP_IIF(takes_tileset, const std::string &tileset, BOOST_PP_EMPTY)()) const \
    { \
        return LayerValue<type>::not_provided(); \
    }

CONFIG_VALUE_LIST

#undef CONFIG_VALUE
```

**Pros:**
- No external dependencies or build tools
- Everything stays in C++
- Works with any compiler (GCC, Clang, MSVC)
- Very fast - just preprocessor expansion
- Easy to integrate with existing build system
- Type-safe

**Cons:**
- Preprocessor-heavy code can be harder to debug
- IDE support may be limited (code completion, navigation)
- Error messages can be cryptic
- Limited expressiveness compared to full templating language
- Requires learning X-Macro pattern

**Integration:**
- No CMake changes needed
- Just create `config_definitions.hpp` and include where needed

**Recommendation for this project:** ⭐⭐⭐⭐ Good fit - simple, no dependencies, works with your existing setup.

---

### Option 2: Python Script + Jinja2 Templates

**Overview:** Define configuration schema in YAML/JSON, write Jinja2 templates for each generated file, run Python script at build time to generate C++ code.

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

```cmake
# CMakeLists.txt addition
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Generate config files before building
add_custom_command(
    OUTPUT
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/domain/config/domain_config.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/config_provider.hpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
    DEPENDS
        ${CMAKE_SOURCE_DIR}/config_schema.yaml
        ${CMAKE_SOURCE_DIR}/templates/domain_config.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/templates/config_provider.hpp.jinja2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating configuration code..."
)

add_custom_target(generate_config_code
    DEPENDS
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/domain/config/domain_config.hpp
)

add_dependencies(porytiles2_lib generate_config_code)
```

**Pros:**
- Very flexible and powerful
- Clean separation: schema, templates, and generated code are separate
- Easy to understand and modify templates
- Good for complex generation logic
- Can generate any text format (C++, docs, JSON, etc.)
- Widely used and well-documented
- Can generate documentation alongside code

**Cons:**
- Adds Python dependency (though already needed for many C++ projects)
- More complex build process
- Generated files need to be either committed or regenerated on every build
- Requires maintaining template files
- Slightly slower than preprocessor (but still fast)

**Integration:**
- Add custom CMake commands as shown above
- Add `pip install jinja2 pyyaml` to build instructions
- Could use Python virtual environment to isolate dependencies

**Recommendation for this project:** ⭐⭐⭐⭐⭐ Excellent fit - flexible, clean, plays well with CMake.

---

### Option 3: Cog (Embedded Python Code Generator)

**Overview:** Embed Python code in C++ files as comments. Cog processes these files and generates C++ code inline.

**Example Implementation:**

```C++
// domain_config.hpp
#pragma once

#include <string>
#include "porytiles2/xcut/config/config_value.hpp"

namespace porytiles2 {

class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

/*[[[cog
import cog

config_values = [
    ('num_tiles_primary', 'std::size_t'),
    ('num_tiles_total', 'std::size_t'),
    ('num_metatiles_primary', 'std::size_t'),
]

for name, type_name in config_values:
    cog.outl(f"    [[nodiscard]] virtual ChainableResult<ConfigValue<{type_name}>>")
    cog.outl(f"    {name}(const std::string &tileset) const = 0;")
    cog.outl("")
]]]*/
//[[[end]]]
};

} // namespace porytiles2
```

After running `cog -r domain_config.hpp`:

```C++
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

/*[[[cog
import cog

config_values = [
    ('num_tiles_primary', 'std::size_t'),
    ('num_tiles_total', 'std::size_t'),
]

for name, type_name in config_values:
    cog.outl(f"    [[nodiscard]] virtual ChainableResult<ConfigValue<{type_name}>>")
    cog.outl(f"    {name}(const std::string &tileset) const = 0;")
    cog.outl("")
]]]*/
    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_tiles_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_tiles_total(const std::string &tileset) const = 0;

//[[[end]]]
};
```

**Pros:**
- Generation logic lives right next to generated code
- Can see both generator and output in same file
- Simple to understand what's being generated
- Python for generation logic (very flexible)
- Can commit generated code to version control

**Cons:**
- Mixes concerns (generator + generated code)
- Files become longer
- Generated sections must be committed (or regenerated)
- Less clean separation than external templates
- Another tool dependency (`pip install cogapp`)
- Generated code can't be easily grep'd (it's in comments too)

**Integration:**
```cmake
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_target(cog_generate
    COMMAND ${Python3_EXECUTABLE} -m cogapp -r
        include/porytiles2/domain/config/domain_config.hpp
        include/porytiles2/infra/config/config_provider.hpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cog code generation..."
)
```

**Recommendation for this project:** ⭐⭐⭐ Decent option, but mixing generation and code is less clean.

---

### Option 4: Boost.Hana / Magic Enum / Reflection

**Overview:** Use C++ metaprogramming libraries to reduce boilerplate through compile-time reflection and type manipulation.

**Example (conceptual):**

```C++
// Define config schema using constexpr
struct ConfigSchema {
    static constexpr auto values = hana::make_tuple(
        hana::make_pair("num_tiles_primary"_s, hana::type_c<std::size_t>),
        hana::make_pair("num_tiles_total"_s, hana::type_c<std::size_t>)
    );
};

// Generate methods via template metaprogramming
template<typename Schema>
class GeneratedDomainConfig {
    // ... template magic to generate methods ...
};
```

**Pros:**
- Pure C++ solution
- Type-safe at compile time
- No external build steps

**Cons:**
- Very complex to implement correctly
- Steep learning curve
- Significantly increased compile times
- C++ doesn't have native reflection yet (proposed for C++26)
- May not work well with virtual interfaces
- Hard to debug template errors
- Not all compilers support all features equally

**Recommendation for this project:** ⭐ Not recommended - too complex for the benefit.

---

### Option 5: Custom DSL with Parser

**Overview:** Create a custom domain-specific language for config definitions, write a parser, generate C++ code.

**Example:**

```
// config.porytiles
@domain config {
    num_tiles_primary: size_t with tileset;
    num_tiles_total: size_t with tileset;
    extrinsic_transparency: Rgba32 with tileset;
}

@infra config {
    tiles_pal_mode: TilesPalMode with tileset;
}
```

**Pros:**
- Highly customized to your exact needs
- Clean, readable syntax
- Can add validation rules
- Educational/interesting to build

**Cons:**
- Significant upfront development cost
- Parser maintenance burden
- Overkill for this use case
- Another language to learn

**Recommendation for this project:** ⭐ Not recommended - way too much work.

---

## Comparison Matrix

| Solution | Complexity | Dependencies | Flexibility | IDE Support | Compiler Portability |
|----------|-----------|--------------|-------------|-------------|---------------------|
| X-Macros | Low | None | Medium | Poor | Excellent |
| Python + Jinja2 | Medium | Python, Jinja2 | Very High | Good | Excellent |
| Cog | Medium | Python, Cog | High | Medium | Excellent |
| Boost.Hana | Very High | Boost | Medium | Poor | Good |
| Custom DSL | Very High | Parser lib | Very High | Poor | Excellent |

## Recommendations

### Best Overall: Python Script + Jinja2 ⭐⭐⭐⭐⭐

**Why:**
- Clean separation of concerns (schema separate from templates)
- Very flexible - can easily add new features
- Integrates well with CMake
- Can generate documentation alongside code
- Easy to understand and maintain
- Widely used in industry

**How to implement:**
1. Create `config_schema.yaml` defining all config values
2. Create Jinja2 templates for each generated file
3. Write `Scripts/generate_config.py`
4. Add CMake custom commands to run generation before build
5. Optionally commit generated files (for faster builds) or generate on-demand

### Simplest: X-Macros ⭐⭐⭐⭐

**Why:**
- Zero dependencies
- No build system changes
- Just works with existing setup
- Fast

**Trade-offs:**
- Less flexible than Jinja2
- Harder to debug
- Limited IDE support

**When to choose:** If you want the absolute simplest solution with no dependencies and don't mind preprocessor-heavy code.

### Middle Ground: Cog ⭐⭐⭐

**Why:**
- Keeps generation close to code
- Python for generation logic
- Can commit generated code

**Trade-offs:**
- Mixes generator and generated code
- Less clean than separate templates

## Proposed Implementation Path

If choosing **Python + Jinja2** (recommended):

1. **Phase 1:** Setup infrastructure
   - Create `config_schema.yaml` with current config values
   - Create template directory structure
   - Write first template (e.g., `domain_config.hpp`)
   - Write generation script skeleton

2. **Phase 2:** Generate one file end-to-end
   - Template for `DomainConfig`
   - Test generation works
   - Integrate with CMake
   - Verify builds correctly

3. **Phase 3:** Expand to all files
   - Add templates for all layer configs
   - Add templates for ConfigProvider
   - Add templates for LazyLayeredConfig declarations

4. **Phase 4:** Handle special cases
   - Derived/computed methods (keep hand-written)
   - Complex validation logic
   - Provider-specific logic

5. **Phase 5:** Documentation
   - Generate Doxygen comments
   - Generate config documentation
   - Update build instructions

## Example Directory Structure

```
Porytiles2/
├── config_schema.yaml           # Single source of truth
├── templates/
│   ├── domain_config.hpp.jinja2
│   ├── app_config.hpp.jinja2
│   ├── infra_config.hpp.jinja2
│   ├── config_provider.hpp.jinja2
│   ├── config_provider.cpp.jinja2
│   └── lazy_layered_config.hpp.jinja2
├── Scripts/
│   └── generate_config.py
└── include/porytiles2/
    ├── domain/config/
    │   └── domain_config.hpp        # GENERATED
    ├── app/config/
    │   └── app_config.hpp           # GENERATED
    └── infra/config/
        ├── infra_config.hpp         # GENERATED
        └── config_provider.hpp      # GENERATED
```

## Next Steps

1. Decide which approach fits your workflow best
2. Create a proof-of-concept with 2-3 config values
3. Validate it generates correct code
4. Expand to full config system
5. Update documentation

## References

- **X-Macros:** https://en.wikipedia.org/wiki/X_Macro
- **Jinja2:** https://jinja.palletsprojects.com/
- **Cog:** https://nedbatchelder.com/code/cog/
- **CMake Code Generation:** https://cmake.org/cmake/help/latest/command/add_custom_command.html
