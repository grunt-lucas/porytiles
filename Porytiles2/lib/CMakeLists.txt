project(Porytiles2Lib CXX)

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


##############################################
#### Set up Porytiles library compilation ####
##############################################

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_library(Porytiles2Lib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles2")
set_target_properties(Porytiles2Lib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


#######################################
#### Configure include directories ####
#######################################

# Force dependents to include files like <porytiles/compiler.h>, while
# internally we can simply use "./compiler.h"
target_include_directories(Porytiles2Lib INTERFACE ${PORYTILES2_INCLUDE_DIR} PRIVATE ${PORYTILES2_INCLUDE_DIR}/${CANONICAL_LIB_NAME})


###########################
#### Link dependencies ####
###########################

# Public link API exposed libs
target_link_libraries(Porytiles2Lib PUBLIC fmt::fmt)
target_link_libraries(Porytiles2Lib PUBLIC Microsoft.GSL::GSL)


##################################
#### Export Porytiles library ####
##################################

# Export the library.
# We have to export GSL explicitly since the GSL CMake disables exporting by default.
export(TARGETS Porytiles2Lib GSL FILE ${CMAKE_CURRENT_BINARY_DIR}/Porytiles2LibraryTargets.cmake)
