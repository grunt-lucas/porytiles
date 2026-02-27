project(Porytiles1Lib CXX)

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
add_library(Porytiles1Lib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(Porytiles1Lib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


#######################################
#### Configure include directories ####
#######################################

# Force dependents to include files like <porytiles/compiler.h>, while
# internally we can simply use "./compiler.h"
target_include_directories(Porytiles1Lib INTERFACE ${PORYTILES1_INCLUDE_DIR} PRIVATE ${PORYTILES1_INCLUDE_DIR}/${CANONICAL_LIB_NAME})

# Find the libpng include dirs located via find_package
target_include_directories(Porytiles1Lib SYSTEM PUBLIC ${PNG_INCLUDE_DIRS})

# Mark public any includes our interface exposes.
target_include_directories(Porytiles1Lib PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles1/vendor/png++-0.2.9)

# Disable doctest
target_compile_definitions(Porytiles1Lib PRIVATE DOCTEST_CONFIG_DISABLE)

# Add other vendor header-only libs to the include path
target_include_directories(Porytiles1Lib PRIVATE ${CMAKE_SOURCE_DIR}/Porytiles1/vendor/fast-cpp-csv-parser)


###########################
#### Link dependencies ####
###########################

# Link the static libpng and zlib we searched for earlier
target_link_libraries(Porytiles1Lib PRIVATE ${PNG_STATIC})
target_link_libraries(Porytiles1Lib PRIVATE ${ZLIB_STATIC})

# Public link API exposed libs
target_link_libraries(Porytiles1Lib PUBLIC fmt::fmt)
target_link_libraries(Porytiles1Lib PUBLIC Microsoft.GSL::GSL)


##################################
#### Export Porytiles library ####
##################################

# Export the library.
export(TARGETS Porytiles1Lib FILE ${CMAKE_CURRENT_BINARY_DIR}/Porytiles1LibraryTargets.cmake)
