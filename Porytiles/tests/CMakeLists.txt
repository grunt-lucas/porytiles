project(PorytilesTestSuite CXX)

add_executable(PorytilesTestSuite tests.cpp)
target_link_libraries(PorytilesTestSuite PRIVATE PorytilesLib)
target_include_directories(PorytilesTestSuite PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)

# Add the test suite to a ctest configuration
add_test(NAME AllPorytilesTests COMMAND $<TARGET_FILE:PorytilesTestSuite> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
