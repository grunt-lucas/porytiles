# Enable testing
enable_testing()

# These tests are "grey box".
# They're designed to test the Porytiles2 library via the public interface
# defined in the "include" directory.
# However, they have access to internal implementation details of the library
# so that they can test certain classes which aren't exposed in the interface.
#
# TODO : Restructure things so those internal classes can be tested via the public interface

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
FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)

# Create the test executable
add_executable(Porytiles2Tests ${CppSources})

# Allow the test suite to see private internal implementation files of the library
target_include_directories(Porytiles2Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
target_link_libraries(Porytiles2Tests PRIVATE GTest::gtest_main)
# gtest_discover_tests(Porytiles2Tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Link against the Porytiles2 library
target_link_libraries(Porytiles2Tests PRIVATE Porytiles2Lib)

# Add CImg to our include directories since we're testing the internal implementation
if (cimg_SOURCE_DIR)
    target_include_directories(Porytiles2Tests PRIVATE ${cimg_SOURCE_DIR})
else ()
    message(FATAL_ERROR "cimg_SOURCE_DIR was empty")
endif ()
