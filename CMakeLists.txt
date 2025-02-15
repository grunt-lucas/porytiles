cmake_minimum_required(VERSION 3.20)

# Test discovery so ctest can work
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

project(Porytiles0x CXX)

# Enable compilation warnings for 0.x
if (MSVC)
    add_compile_options(/W4 /WX)
else ()
    # Cannot enable -Wextra in 0.x since some of our header-only dependencies throw warnings
    # that break the build.
    #
    # Remove warnings-as-errors temporarily to stop GCC from complaining about one of the header libs.
    # add_compile_options(-Wall -Wpedantic -Werror)
    add_compile_options(-Wall -Wpedantic)
endif ()

# Enable testing
enable_testing()

add_subdirectory(Porytiles-0.x)
