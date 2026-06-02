
# Set a variable that points to the global include directory for Porytiles
set(PORYTILES_LEGACY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/legacy/include)

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
