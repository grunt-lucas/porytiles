project(PorytilesLib CXX)

include(FetchContent)


###########################
#### Find dependencies ####
###########################

# Find PNG and ZLIB packages using standard CMake discovery
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

# fmtlib
FetchContent_Declare(
        fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt"
        GIT_TAG "11.1.4"
)
FetchContent_MakeAvailable(fmt)

# Core Guidelines support library
FetchContent_Declare(
        GSL
        GIT_REPOSITORY "https://github.com/microsoft/GSL"
        GIT_TAG "v4.2.0"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)

# png++
FetchContent_Declare(
        pngpp
        GIT_REPOSITORY "https://github.com/usagi/pngpp"
)
FetchContent_MakeAvailable(pngpp)

# nlohmann's json
FetchContent_Declare(
        json
        GIT_REPOSITORY "https://github.com/nlohmann/json"
        GIT_TAG "v3.12.0"
)
FetchContent_MakeAvailable(json)

# jbeder - yaml-cpp
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
        GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

# cpptrace - cross-platform stacktrace library
# (std::stacktrace not available on macOS with Apple Clang)
FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY "https://github.com/jeremy-rifkin/cpptrace.git"
        GIT_TAG "v0.7.5"
)
FetchContent_MakeAvailable(cpptrace)

# Google Fruit DI
FetchContent_Declare(
        fruit
        GIT_REPOSITORY https://github.com/google/fruit.git
        GIT_TAG master  # Use master branch for CMake compatibility fix
)
set(FRUIT_USES_BOOST FALSE CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# Fruit needs C++17 or earlier, not compatible with C++23
set(CMAKE_CXX_STANDARD 17)
FetchContent_MakeAvailable(fruit)
# Restore C++23 for Porytiles
set(CMAKE_CXX_STANDARD 23)

# CLI11 - command-line parser (needed for CLI option registration in infra layer)
FetchContent_Declare(
        cli11_proj
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.5.0
)
FetchContent_MakeAvailable(cli11_proj)

##############################################
#### Set up Porytiles library compilation ####
##############################################

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_library(PorytilesLib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(PorytilesLib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")

target_compile_definitions(PorytilesLib PRIVATE PORYTILES_BUILD_VERSION_=${PORYTILES_BUILD_VERSION_})

# Ensure config code is generated before building the library
add_dependencies(PorytilesLib generate_config_code)

#######################################
#### Configure include directories ####
#######################################

target_include_directories(PorytilesLib PUBLIC ${PORYTILES_INCLUDE_DIR})
# target_include_directories(PorytilesLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# PNG::PNG imported target handles include directories automatically

# For header-only libs, we pull the sources and add headers to include path
if (pngpp_SOURCE_DIR)
    target_include_directories(PorytilesLib PRIVATE ${pngpp_SOURCE_DIR}/include)
else ()
    message(FATAL_ERROR "pngpp_SOURCE_DIR was empty")
endif ()

if (json_SOURCE_DIR)
    target_include_directories(PorytilesLib PRIVATE ${json_SOURCE_DIR}/include)
else ()
    message(FATAL_ERROR "json_SOURCE_DIR was empty")
endif ()


###########################
#### Link dependencies ####
###########################

### Link PNG and ZLIB using imported targets ###
target_link_libraries(PorytilesLib PRIVATE PNG::PNG)
target_link_libraries(PorytilesLib PRIVATE ZLIB::ZLIB)

### Public link API exposed libs ###
target_link_libraries(PorytilesLib PUBLIC Microsoft.GSL::GSL)
target_link_libraries(PorytilesLib PUBLIC fruit)
target_link_libraries(PorytilesLib PUBLIC CLI11::CLI11)

### Private link internal libs ###
target_link_libraries(PorytilesLib PRIVATE yaml-cpp::yaml-cpp)
# NOTE: fmt is still required for TextFormatter which uses fmt::dynamic_format_arg_store.
# C++23 std::format does not support runtime-variable argument counts.
# All other fmt::format usages have been migrated to std::format.
target_link_libraries(PorytilesLib PRIVATE fmt::fmt)
target_link_libraries(PorytilesLib PRIVATE cpptrace::cpptrace)


##################################
#### Export Porytiles library ####
##################################

# Export the library and its dependencies.
# Libraries with modern CMake export configuration (like fmt::fmt) handle their own exports
# automatically through namespaced targets and don't need to be listed here.
# However, libraries without proper export configuration must be explicitly exported:
# - GSL: Only exports when built as a top-level project (GSL_INSTALL defaults to false
#        when used via FetchContent as a subdirectory)
# - fruit: Uses simple CMake targets without automatic export handling
# - yaml-cpp: Only exports when built as a top-level project (YAML_CPP_INSTALL defaults to
#             false when used via FetchContent as a subdirectory)
# These must be manually included so downstream projects can find them as dependencies.
export(TARGETS PorytilesLib GSL fruit yaml-cpp CLI11 FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
