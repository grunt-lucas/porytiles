# Create the test executable
FILE(GLOB TEST_SOURCES *.cpp)
add_executable(TemplatesTest ${TEST_SOURCES})

target_include_directories(TemplatesTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(TemplatesTest PRIVATE GTest::gtest_main)
target_link_libraries(TemplatesTest PRIVATE PorytilesLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(TemplatesTest)
