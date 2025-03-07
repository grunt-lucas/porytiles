cmake_minimum_required(VERSION 3.18)

project(Porytiles CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Enable compilation warnings
if (MSVC)
    add_compile_options(/W4 /WX)
else ()
    # Cannot enable -Wextra since some of our header-only dependencies throw warnings
    # that break the build.
    #
    # Remove warnings-as-errors temporarily to stop GCC from complaining about one of the header libs.
    # add_compile_options(-Wall -Wpedantic -Werror)
    add_compile_options(-Wall -Wpedantic)
endif ()

add_subdirectory(Porytiles)
