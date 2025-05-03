project(PorytilesLegacyDriver CXX)

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_executable(PorytilesLegacyDriver ${CppSources})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(PorytilesLegacyDriver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(PorytilesLegacyDriver PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(PorytilesLegacyDriver PRIVATE PorytilesLib)
