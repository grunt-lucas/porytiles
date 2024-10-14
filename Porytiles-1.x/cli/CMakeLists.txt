project(PorytilesCliTool CXX)

add_executable(PorytilesCliTool main.cpp)
set_target_properties(PorytilesCliTool PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(PorytilesCliTool PRIVATE PorytilesLibrary)
target_include_directories(PorytilesCliTool PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
