project(Porytiles2xDriver CXX)

add_executable(Porytiles2xDriver main.cpp)
set_target_properties(Porytiles2xDriver PROPERTIES OUTPUT_NAME "porytiles")

# Link against the library
target_link_libraries(Porytiles2xDriver PRIVATE Porytiles2xLib)

# Add include directories
target_include_directories(Porytiles2xDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
