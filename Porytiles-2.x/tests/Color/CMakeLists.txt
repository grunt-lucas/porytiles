# Create the test executable
add_executable(ColorTest ColorTest.cpp)

target_include_directories(ColorTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(ColorTest PRIVATE GTest::gtest_main)
target_link_libraries(ColorTest PRIVATE Porytiles2xLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(ColorTest)
