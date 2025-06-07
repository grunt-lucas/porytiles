project(Porytiles1Driver CXX)

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_executable(Porytiles1Driver ${CppSources})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(Porytiles1Driver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(Porytiles1Driver PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(Porytiles1Driver PRIVATE Porytiles1Lib)
