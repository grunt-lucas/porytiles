project(LegacyDriver CXX)

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_executable(LegacyDriver ${CppSources})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(LegacyDriver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(LegacyDriver PROPERTIES OUTPUT_NAME "porytiles-legacy")
target_link_libraries(LegacyDriver PRIVATE LegacyLib)

install(TARGETS LegacyDriver
        RUNTIME DESTINATION bin)
