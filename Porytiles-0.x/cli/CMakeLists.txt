project(Porytiles0xCliTool CXX)

add_executable(Porytiles0xCliTool main.cpp)
set_target_properties(Porytiles0xCliTool PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(Porytiles0xCliTool PRIVATE Porytiles0xLib)
target_include_directories(Porytiles0xCliTool PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
