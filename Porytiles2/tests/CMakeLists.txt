# Enable testing
enable_testing()

# Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(googletest)

# Get all sources in a list
FILE(GLOB_RECURSE CppUnitSources CONFIGURE_DEPENDS unit/*.cpp)
FILE(GLOB_RECURSE CppIntegrationSources CONFIGURE_DEPENDS integration/*.cpp)
FILE(GLOB_RECURSE CppAllSources CONFIGURE_DEPENDS unit/*.cpp integration/*.cpp)

# Custom test main that disables stacktrace generation by default
set(TEST_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp)

# Create the test executable
add_executable(Porytiles2UnitTests ${TEST_MAIN} ${CppUnitSources})
add_executable(Porytiles2IntegrationTests ${TEST_MAIN} ${CppIntegrationSources})
add_executable(Porytiles2AllTests ${TEST_MAIN} ${CppAllSources})

target_link_libraries(Porytiles2UnitTests PRIVATE GTest::gtest GTest::gmock)
target_link_libraries(Porytiles2IntegrationTests PRIVATE GTest::gtest GTest::gmock)
target_link_libraries(Porytiles2AllTests PRIVATE GTest::gtest GTest::gmock)
# gtest_discover_tests(Porytiles2UnitTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Link against the Porytiles2 library
target_link_libraries(Porytiles2UnitTests PRIVATE Porytiles2Lib)
target_link_libraries(Porytiles2IntegrationTests PRIVATE Porytiles2Lib)
target_link_libraries(Porytiles2AllTests PRIVATE Porytiles2Lib)
