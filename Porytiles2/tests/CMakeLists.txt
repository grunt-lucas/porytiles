# Enable testing
enable_testing()

# Fetch and build Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(googletest)

# Get all sources in a list
FILE(GLOB TEST_SOURCES */*.cpp)

add_executable(Porytiles2Tests ${TEST_SOURCES})
target_include_directories(Porytiles2Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
target_link_libraries(Porytiles2Tests PRIVATE GTest::gtest_main)
target_link_libraries(Porytiles2Tests PRIVATE Porytiles2Lib)
# gtest_discover_tests(Porytiles2Tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
