project(PorytilesDocTests CXX)

# TODO : this file duplicates a lot of code from Porytiles/lib/CMakeLists.txt.
# Unfortunately this is necessary so that we can cleanly separate the test
# build configuration from the main runner configuration. Ultimately it leads
# to a cleaner tool build with no warnings, and allows IDEs to correctly show
# the true value for DOCTEST_CONFIG_DISABLED. At some point if we switch to
# Google Test we can probably handle this more cleanly.

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

##################################################
#### Compile Porytiles lib code as object lib ####
##################################################

# Only grab the legacy code
FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Porytiles/lib/*.cpp)
# This needs to be an object library for now, otherwise doctest cannot find any
# tests
add_library(PorytilesLibDocTests OBJECT ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")

#######################################
#### Configure include directories ####
#######################################

target_include_directories(PorytilesLibDocTests PRIVATE ${PORYTILES_INCLUDE_DIR}/${CANONICAL_LIB_NAME})

# Find the libpng include dirs located via find_package
target_include_directories(PorytilesLibDocTests PUBLIC ${PNG_INCLUDE_DIRS})

# png++ library is a header-only C++ wrapper for libpng. Mark it public because
# our library interface uses types from it.
target_include_directories(PorytilesLibDocTests PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles/vendor/png++-0.2.9)

# Expose doctest header
target_include_directories(PorytilesLibDocTests PUBLIC ${CMAKE_SOURCE_DIR}/Porytiles/vendor/doctest-2.4.11)

# Add other vendor header-only libs to the include path
target_include_directories(PorytilesLibDocTests PRIVATE ${CMAKE_SOURCE_DIR}/Porytiles/vendor/fast-cpp-csv-parser)


###########################
#### Link dependencies ####
###########################

# Link the static libpng and zlib we searched for earlier
target_link_libraries(PorytilesLibDocTests PRIVATE ${PNG_STATIC})
target_link_libraries(PorytilesLibDocTests PRIVATE ${ZLIB_STATIC})

# Public link API exposed libs
target_link_libraries(PorytilesLibDocTests PUBLIC fmt::fmt)
target_link_libraries(PorytilesLibDocTests PUBLIC Microsoft.GSL::GSL)


##################################
#### Compile the test harness ####
##################################

add_executable(PorytilesDocTests tests.cpp)
target_link_libraries(PorytilesDocTests PRIVATE PorytilesLibDocTests)

# Add the test suite to a ctest configuration
add_test(NAME AllPorytilesDocTests COMMAND $<TARGET_FILE:PorytilesDocTests> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
