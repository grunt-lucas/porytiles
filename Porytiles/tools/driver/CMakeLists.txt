project(PorytilesDriver CXX)

add_executable(PorytilesDriver main.cpp)
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(PorytilesDriver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(PorytilesDriver PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(PorytilesDriver PRIVATE PorytilesLib)
