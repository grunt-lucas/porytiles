cmake_minimum_required(VERSION 3.5)

project(Porytiles CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(cli)
