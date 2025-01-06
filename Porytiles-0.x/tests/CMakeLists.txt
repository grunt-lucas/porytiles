project(Porytiles0xTestSuite CXX)

add_executable(Porytiles0xTestSuite tests.cpp)
target_link_libraries(Porytiles0xTestSuite PRIVATE Porytiles0xLib)
target_include_directories(Porytiles0xTestSuite PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)

# Add the test suite to a ctest configuration
add_test(NAME All0xPorytilesTests COMMAND $<TARGET_FILE:Porytiles0xTestSuite> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
