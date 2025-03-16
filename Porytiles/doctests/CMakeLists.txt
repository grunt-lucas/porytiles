project(PorytilesDocTests CXX)

add_executable(PorytilesDocTests tests.cpp)
target_link_libraries(PorytilesDocTests PRIVATE PorytilesLib)
target_include_directories(PorytilesDocTests PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)

# Add the test suite to a ctest configuration
add_test(NAME AllPorytilesDoctests COMMAND $<TARGET_FILE:PorytilesDocTests> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
