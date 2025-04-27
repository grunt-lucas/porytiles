
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles/include)

add_subdirectory(lib)
add_subdirectory(doctests)
add_subdirectory(tests)
add_subdirectory(tools)
