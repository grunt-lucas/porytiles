project(PorytilesLib CXX)

include(FetchContent)


###########################
#### Find dependencies ####
###########################

# Find the PNG package in the standard way. We won't use this for linking, only
# for including the headers correctly.
find_package(PNG REQUIRED)

# Pull in fmtlib directly from GitHub
FetchContent_Declare(
        fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt"
        GIT_TAG "11.1.4")
FetchContent_MakeAvailable(fmt)

# Pull in Core Guidelines support library
FetchContent_Declare(
        GSL
        GIT_REPOSITORY "https://github.com/microsoft/GSL"
        GIT_TAG "v4.2.0"
        GIT_SHALLOW ON)
FetchContent_MakeAvailable(GSL)

# Find libpng.a and libz.a explicitly. We'll statically link these so users
# don't need to install.
find_library(PNG_STATIC
        NAMES libpng.a
        PATHS /opt/homebrew/opt/libpng/lib
        PATHS /usr/local/opt/libpng/lib
        PATHS /usr/lib/x86_64-linux-gnu
        PATHS /usr/lib/aarch64-linux-gnu)
find_library(ZLIB_STATIC
        NAMES libz.a
        PATHS /opt/homebrew/opt/zlib/lib
        PATHS /usr/local/opt/zlib/lib
        PATHS /usr/lib/x86_64-linux-gnu
        PATHS /usr/lib/aarch64-linux-gnu)


##############################################
#### Set up Porytiles library compilation ####
##############################################

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_library(PorytilesLib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(PorytilesLib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


#######################################
#### Configure include directories ####
#######################################

# Force dependents to include files like <porytiles/compiler.h>, while
# internally we can simply use "./compiler.h"
target_include_directories(PorytilesLib INTERFACE ${PORYTILES_INCLUDE_DIR} PRIVATE ${PORYTILES_INCLUDE_DIR}/${CANONICAL_LIB_NAME})

# Find the libpng include dirs located via find_package
target_include_directories(PorytilesLib PUBLIC ${PNG_INCLUDE_DIRS})

# Mark public any includes our interface exposes.
target_include_directories(PorytilesLib PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles/vendor/png++-0.2.9)

# Disable doctest
target_compile_definitions(PorytilesLib PRIVATE DOCTEST_CONFIG_DISABLE)

# Add other vendor header-only libs to the include path
target_include_directories(PorytilesLib PRIVATE ${CMAKE_SOURCE_DIR}/Porytiles/vendor/fast-cpp-csv-parser)


###########################
#### Link dependencies ####
###########################

# Link the static libpng and zlib we searched for earlier
target_link_libraries(PorytilesLib PRIVATE ${PNG_STATIC})
target_link_libraries(PorytilesLib PRIVATE ${ZLIB_STATIC})

# Public link API exposed libs
target_link_libraries(PorytilesLib PUBLIC fmt::fmt)
target_link_libraries(PorytilesLib PUBLIC Microsoft.GSL::GSL)


##################################
#### Export Porytiles library ####
##################################

# Export the library.
# We have to export GSL explicitly since the GSL CMake disables exporting by default.
export(TARGETS PorytilesLib GSL FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
