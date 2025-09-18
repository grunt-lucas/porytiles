
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles2/include)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fstandalone-debug -fno-limit-debug-info")

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
