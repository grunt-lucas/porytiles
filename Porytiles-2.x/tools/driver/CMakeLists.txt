project(PorytilesDriver CXX)

add_executable(PorytilesDriver main.cpp)
set_target_properties(PorytilesDriver PROPERTIES OUTPUT_NAME "porytiles")

# Link against the library
target_link_libraries(PorytilesDriver PRIVATE PorytilesLibrary)

# Add include directories
target_include_directories(PorytilesDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
