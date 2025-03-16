project(PorytilesDriver CXX)

add_executable(PorytilesDriver main.cpp)
set_target_properties(PorytilesDriver PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(PorytilesDriver PRIVATE PorytilesLib)
target_include_directories(PorytilesDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Porytiles/vendor/doctest-2.4.11)
