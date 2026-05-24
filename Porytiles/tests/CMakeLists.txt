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
add_executable(PorytilesUnitTests ${TEST_MAIN} ${CppUnitSources})
add_executable(PorytilesIntegrationTests ${TEST_MAIN} ${CppIntegrationSources})
add_executable(PorytilesAllTests ${TEST_MAIN} ${CppAllSources})

target_link_libraries(PorytilesUnitTests PRIVATE GTest::gtest GTest::gmock)
target_link_libraries(PorytilesIntegrationTests PRIVATE GTest::gtest GTest::gmock)
target_link_libraries(PorytilesAllTests PRIVATE GTest::gtest GTest::gmock)
# gtest_discover_tests(PorytilesUnitTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Add test support headers (shared mocks, utilities)
target_include_directories(PorytilesUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(PorytilesIntegrationTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(PorytilesAllTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link against the Porytiles library
target_link_libraries(PorytilesUnitTests PRIVATE PorytilesLib)
target_link_libraries(PorytilesIntegrationTests PRIVATE PorytilesLib)
target_link_libraries(PorytilesAllTests PRIVATE PorytilesLib)
