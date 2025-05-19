# Create the test executable
add_executable(TilesTest tiles_test.cpp)

target_include_directories(TilesTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(TilesTest PRIVATE GTest::gtest_main)
target_link_libraries(TilesTest PRIVATE PorytilesLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(TilesTest)
