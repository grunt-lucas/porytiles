project(Porytiles1xTestSuite CXX)

add_executable(Porytiles1xTestSuite tests.cpp)
target_link_libraries(Porytiles1xTestSuite PRIVATE Porytiles1xLib)
target_include_directories(Porytiles1xTestSuite PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)

# Add the test suite to a ctest configuration
add_test(NAME All1xPorytilesTests COMMAND $<TARGET_FILE:Porytiles1xTestSuite> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
