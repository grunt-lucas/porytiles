project(PorytilesLib CXX)

# Find the PNG package in the standard way. We won't use this for linking, only
# for including the headers correctly.
find_package(PNG REQUIRED)

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

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
# This needs to be an object library for now, otherwise doctest cannot find any tests
add_library(PorytilesLib OBJECT ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(PorytilesLib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")
target_include_directories(PorytilesLib PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles/vendor/fmt-10.1.1/include)
target_include_directories(PorytilesLib PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles/vendor/png++-0.2.9)

# Find the libpng include dirs located via find_package
target_include_directories(PorytilesLib PUBLIC ${PNG_INCLUDE_DIRS})

# Force dependents to include files like "porytiles/compiler.h", while internally we can simply use "compiler.h"
target_include_directories(PorytilesLib INTERFACE ${PORYTILES_INCLUDE_DIR} PRIVATE ${PORYTILES_INCLUDE_DIR}/${CANONICAL_LIB_NAME})

target_include_directories(PorytilesLib PRIVATE ${CMAKE_SOURCE_DIR}/Porytiles/vendor/doctest-2.4.11)
target_include_directories(PorytilesLib PRIVATE ${CMAKE_SOURCE_DIR}/Porytiles/vendor/fast-cpp-csv-parser)

# Link the static versions of libpng and libz
target_link_libraries(PorytilesLib PRIVATE ${PNG_STATIC})
target_link_libraries(PorytilesLib PRIVATE ${ZLIB_STATIC})

# Export the library
# TODO : what's the difference between these two
export(TARGETS PorytilesLib FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
export(PACKAGE PorytilesLib)
