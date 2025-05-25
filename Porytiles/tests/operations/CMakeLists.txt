# Create the test executable
FILE(GLOB TEST_SOURCES *.cpp)
add_executable(OperationsTest ${TEST_SOURCES})

target_include_directories(OperationsTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(OperationsTest PRIVATE GTest::gtest_main)
target_link_libraries(OperationsTest PRIVATE PorytilesLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(OperationsTest)
