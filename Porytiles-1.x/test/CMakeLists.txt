project(Porytiles1xTestSuite CXX)

add_executable(Porytiles1xTestSuite tests.cpp)
target_link_libraries(Porytiles1xTestSuite PRIVATE Porytiles1xLib)
target_include_directories(Porytiles1xTestSuite PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
