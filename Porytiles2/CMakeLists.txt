
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles2/include)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fstandalone-debug -fno-limit-debug-info -fno-omit-frame-pointer")

# ============================================================================
# Configuration Code Generation
# ============================================================================

# Find Python interpreter - prefer venv if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3.13")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3.13")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Define generated output file(s)
set(GENERATED_DOMAIN_CONFIG
    ${CMAKE_SOURCE_DIR}/Porytiles2/include/porytiles2/domain/config/domain_config.hpp
)

# Define template file
set(DOMAIN_CONFIG_TEMPLATE
    ${CMAKE_SOURCE_DIR}/templates/domain_config.hpp.jinja2
)

# Generate config files before building
add_custom_command(
    OUTPUT ${GENERATED_DOMAIN_CONFIG}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
    DEPENDS
        ${CMAKE_SOURCE_DIR}/config_schema.yaml
        ${CMAKE_SOURCE_DIR}/Scripts/generate_config.py
        ${DOMAIN_CONFIG_TEMPLATE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating configuration code from templates..."
    VERBATIM
)

# Create target that depends on generated files
add_custom_target(generate_config_code
    DEPENDS ${GENERATED_DOMAIN_CONFIG}
)

# ============================================================================

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
