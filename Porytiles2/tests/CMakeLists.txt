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

# Pull in CImg so we can test internal library impl that uses it
FetchContent_Declare(
        CImg
        GIT_REPOSITORY "https://github.com/dtschump/CImg.git"
        GIT_TAG "v.3.5.5"
)
FetchContent_MakeAvailable(CImg)

# Get all sources in a list
FILE(GLOB_RECURSE CppUnitSources CONFIGURE_DEPENDS unit/*.cpp)
FILE(GLOB_RECURSE CppIntegrationSources CONFIGURE_DEPENDS integration/*.cpp)

# Create the test executable
add_executable(Porytiles2UnitTests ${CppUnitSources})
add_executable(Porytiles2IntegrationTests ${CppIntegrationSources})

target_link_libraries(Porytiles2UnitTests PRIVATE GTest::gtest_main)
target_link_libraries(Porytiles2IntegrationTests PRIVATE GTest::gtest_main)
# gtest_discover_tests(Porytiles2UnitTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Link against the Porytiles2 library
target_link_libraries(Porytiles2UnitTests PRIVATE Porytiles2Lib)
target_link_libraries(Porytiles2IntegrationTests PRIVATE Porytiles2Lib)

# Add CImg to our include directories since we're testing the internal implementation
#if (cimg_SOURCE_DIR)
#    target_include_directories(Porytiles2UnitTests PRIVATE ${cimg_SOURCE_DIR})
#    target_include_directories(Porytiles2IntegrationTests PRIVATE ${cimg_SOURCE_DIR})
#else ()
#    message(FATAL_ERROR "cimg_SOURCE_DIR was empty")
#endif ()
