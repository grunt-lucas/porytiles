project(Porytiles2x CXX)

# Enable compilation warnings for 2.x
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Set a variable that points to the global include directory for Porytiles-2.x
set(PORYTILES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles-2.x/include)

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
