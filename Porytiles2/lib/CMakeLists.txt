project(Porytiles2Lib CXX)

include(FetchContent)


###########################
#### Find dependencies ####
###########################

# Find the PNG package in the standard way. We won't use this for linking, only
# for including the headers correctly.
find_package(PNG REQUIRED)

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

# CImg
FetchContent_Declare(
        CImg
        GIT_REPOSITORY "https://github.com/GreycLab/CImg"
        GIT_TAG "v.3.5.5"
)
FetchContent_MakeAvailable(CImg)

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

# Find libz.a before cpptrace so that libdwarf's find_package(ZLIB) resolves to
# the static archive instead of libz.so (which breaks -static linking on Linux).
find_library(ZLIB_STATIC
        NAMES libz.a
        PATHS /opt/homebrew/opt/zlib/lib
        PATHS /usr/local/opt/zlib/lib
        PATHS /usr/lib/x86_64-linux-gnu
        PATHS /usr/lib/aarch64-linux-gnu)
if (ZLIB_STATIC)
    set(ZLIB_LIBRARY ${ZLIB_STATIC} CACHE FILEPATH "Path to zlib library" FORCE)
endif ()

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

# Find libpng.a and libz.a explicitly. We'll statically link these so users
# don't need to install.
find_library(PNG_STATIC
        NAMES libpng.a
        PATHS /opt/homebrew/opt/libpng/lib
        PATHS /usr/local/opt/libpng/lib
        PATHS /usr/lib/x86_64-linux-gnu
        PATHS /usr/lib/aarch64-linux-gnu)
##############################################
#### Set up Porytiles library compilation ####
##############################################

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_library(Porytiles2Lib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles2")
set_target_properties(Porytiles2Lib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")

# Ensure config code is generated before building the library
add_dependencies(Porytiles2Lib generate_config_code)

# Configure CImg
target_compile_definitions(Porytiles2Lib PRIVATE cimg_use_png)
target_compile_definitions(Porytiles2Lib PRIVATE cimg_display=0)
target_compile_definitions(Porytiles2Lib PRIVATE cimg_verbosity=0)


#######################################
#### Configure include directories ####
#######################################

target_include_directories(Porytiles2Lib PUBLIC ${PORYTILES2_INCLUDE_DIR})
# target_include_directories(Porytiles2Lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Find the libpng include dirs located via find_package
target_include_directories(Porytiles2Lib PRIVATE ${PNG_INCLUDE_DIRS})

# For header-only libs, we pull the sources and add headers to include path
if (cimg_SOURCE_DIR)
    target_include_directories(Porytiles2Lib PRIVATE ${cimg_SOURCE_DIR})
else ()
    message(FATAL_ERROR "cimg_SOURCE_DIR was empty")
endif ()

if (pngpp_SOURCE_DIR)
    target_include_directories(Porytiles2Lib PRIVATE ${pngpp_SOURCE_DIR}/include)
else ()
    message(FATAL_ERROR "pngpp_SOURCE_DIR was empty")
endif ()

if (json_SOURCE_DIR)
    target_include_directories(Porytiles2Lib PRIVATE ${json_SOURCE_DIR}/include)
else ()
    message(FATAL_ERROR "json_SOURCE_DIR was empty")
endif ()


###########################
#### Link dependencies ####
###########################

### Link the static libpng and zlib we searched for earlier ###
target_link_libraries(Porytiles2Lib PRIVATE ${PNG_STATIC})
target_link_libraries(Porytiles2Lib PRIVATE ${ZLIB_STATIC})

### Public link API exposed libs ###
target_link_libraries(Porytiles2Lib PUBLIC Microsoft.GSL::GSL)
target_link_libraries(Porytiles2Lib PUBLIC fruit)
target_link_libraries(Porytiles2Lib PUBLIC CLI11::CLI11)

### Private link internal libs ###
target_link_libraries(Porytiles2Lib PRIVATE yaml-cpp::yaml-cpp)
# NOTE: fmt is still required for TextFormatter which uses fmt::dynamic_format_arg_store.
# C++23 std::format does not support runtime-variable argument counts.
# All other fmt::format usages have been migrated to std::format.
target_link_libraries(Porytiles2Lib PRIVATE fmt::fmt)
target_link_libraries(Porytiles2Lib PRIVATE cpptrace::cpptrace)


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
export(TARGETS Porytiles2Lib GSL fruit yaml-cpp CLI11 FILE ${CMAKE_CURRENT_BINARY_DIR}/Porytiles2LibraryTargets.cmake)
