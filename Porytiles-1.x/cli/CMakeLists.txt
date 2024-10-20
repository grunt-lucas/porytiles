project(Porytiles1xCliTool CXX)

add_executable(Porytiles1xCliTool main.cpp)
set_target_properties(Porytiles1xCliTool PROPERTIES OUTPUT_NAME "porytiles")
target_link_libraries(Porytiles1xCliTool PRIVATE Porytiles1xLib)
target_include_directories(Porytiles1xCliTool PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
