project(PorytilesTestSuite CXX)

add_executable(PorytilesTestSuite tests.cpp)
target_link_libraries(PorytilesTestSuite PRIVATE PorytilesLibrary)
target_include_directories(PorytilesTestSuite PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
