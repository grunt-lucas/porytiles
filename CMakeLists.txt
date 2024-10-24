cmake_minimum_required(VERSION 3.20)

project(Porytiles CXX)

# Test discovery so ctest can work
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_subdirectory(Porytiles-1.x)
add_subdirectory(Porytiles-2.x)
