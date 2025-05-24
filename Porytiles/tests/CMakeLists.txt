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

add_subdirectory(colors)
add_subdirectory(config)
add_subdirectory(diagnostics)
add_subdirectory(tiles)

# Get all sources in a list
FILE(GLOB TEST_SOURCES */*.cpp)

add_executable(PorytilesGoogleTests ${TEST_SOURCES})
target_include_directories(PorytilesGoogleTests PRIVATE ${PORYTILES_INCLUDE_DIR})
target_link_libraries(PorytilesGoogleTests PRIVATE GTest::gtest_main)
target_link_libraries(PorytilesGoogleTests PRIVATE PorytilesLib)
gtest_discover_tests(PorytilesGoogleTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
