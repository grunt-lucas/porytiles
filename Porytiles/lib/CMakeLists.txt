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
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 11.1.4)
FetchContent_MakeAvailable(fmt)

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

# png++ library is a header-only C++ wrapper for libpng. Mark it public because
# our library interface uses types from it.
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

# Link fmtlib publicly since our API exposes it
target_link_libraries(PorytilesLib PUBLIC fmt::fmt)


##################################
#### Export Porytiles library ####
##################################

# Export the library
# TODO : what's the difference between these two
export(TARGETS PorytilesLib FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
export(PACKAGE PorytilesLib)
