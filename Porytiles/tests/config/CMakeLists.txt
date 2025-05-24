# Create the test executable
add_executable(ConfigTest config_test.cpp)

target_include_directories(ConfigTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(ConfigTest PRIVATE GTest::gtest_main)
target_link_libraries(ConfigTest PRIVATE PorytilesLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(ConfigTest)
