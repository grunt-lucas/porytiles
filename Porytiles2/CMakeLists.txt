
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles2/include)

# ============================================================================
# Configuration Code Generation
# ============================================================================

# Find uv executable for Python script execution
find_program(UV_EXECUTABLE uv REQUIRED)

# Define generated output files
set(GENERATED_CONFIG_FILES
        # Layer interfaces
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/domain/config/domain_config.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/app/config/app_config.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/infra_config.hpp
        # LazyLayeredConfig implementation
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/lazy_layered_config.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/lib/infra/config/lazy_layered_config.cpp
        # ConfigProvider
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/config_provider.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/lib/infra/config/config_provider.cpp
        # DefaultProvider
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/default_provider.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/lib/infra/config/default_provider.cpp
        # YamlFileProvider
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/yaml_file_provider.hpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/lib/infra/config/yaml_file_provider.cpp
        ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/infra/config/valid_yaml_paths.hpp
)

# Define schema and template files
set(CONFIG_SCHEMA
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/config_schema.yaml
)

# Glob all template files in the config_templates directory
file(GLOB CONFIG_TEMPLATES
        "${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/*.jinja2"
)

# Generate config files before building using uv run
add_custom_command(
        OUTPUT ${GENERATED_CONFIG_FILES}
        COMMAND ${UV_EXECUTABLE} run ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        DEPENDS
        ${CONFIG_SCHEMA}
        ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        ${CONFIG_TEMPLATES}
        ${CMAKE_SOURCE_DIR}/pyproject.toml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating configuration code from templates (via uv)..."
        VERBATIM
)

# Create target that depends on generated files
add_custom_target(generate_config_code
        DEPENDS ${GENERATED_CONFIG_FILES}
)

# ============================================================================

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
