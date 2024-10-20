project(Porytiles2x CXX)

# Fetch and build Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

# Set a variable that points to the global include directory for Porytiles-2.x
set(PORYTILES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles-2.x/include)

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)

# Export the library
export(PACKAGE Porytiles2xLib)
