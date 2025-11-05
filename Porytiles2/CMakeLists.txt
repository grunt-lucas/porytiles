
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles2/include)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fstandalone-debug -fno-limit-debug-info -fno-omit-frame-pointer")

# ============================================================================
# Configuration Code Generation
# ============================================================================

# Find Python interpreter - prefer venv if it exists
if (EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3.13")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3.13")
elseif (EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
elseif (EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python")
endif ()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

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
)

# Define schema and template files
set(CONFIG_SCHEMA
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/config_schema.yaml
)

set(CONFIG_TEMPLATES
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/domain_config.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/app_config.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/infra_config.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/lazy_layered_config.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/lazy_layered_config.cpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/config_provider.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/config_provider.cpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/default_provider.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/default_provider.cpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/yaml_file_provider.hpp.jinja2
        ${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/yaml_file_provider.cpp.jinja2
)

# Generate config files before building
add_custom_command(
        OUTPUT ${GENERATED_CONFIG_FILES}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        DEPENDS
        ${CONFIG_SCHEMA}
        ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        ${CONFIG_TEMPLATES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating configuration code from templates..."
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
