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
add_library(Porytiles2Lib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles2")
set_target_properties(Porytiles2Lib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")

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

if (cimg_SOURCE_DIR)
    target_include_directories(Porytiles2Lib PRIVATE ${cimg_SOURCE_DIR})
else ()
    message(FATAL_ERROR "cimg_SOURCE_DIR was empty")
endif ()


###########################
#### Link dependencies ####
###########################

# Link the static libpng and zlib we searched for earlier
target_link_libraries(Porytiles2Lib PRIVATE ${PNG_STATIC})
target_link_libraries(Porytiles2Lib PRIVATE ${ZLIB_STATIC})

# Public link API exposed libs
target_link_libraries(Porytiles2Lib PUBLIC fmt::fmt)
target_link_libraries(Porytiles2Lib PUBLIC Microsoft.GSL::GSL)


##################################
#### Export Porytiles library ####
##################################

# Export the library.
# We have to export GSL explicitly since the GSL CMake disables exporting by default.
export(TARGETS Porytiles2Lib GSL FILE ${CMAKE_CURRENT_BINARY_DIR}/Porytiles2LibraryTargets.cmake)
