project(Porytiles2xUnitTests CXX)

# Fetch and build Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

# Get all sources in a list
FILE(GLOB TEST_SOURCES */*.cpp)

include(GoogleTest)
# Create a separate test target and gtest test for each unit
#foreach(TEST_SOURCE ${TEST_SOURCES})
#    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
#    add_executable(${TEST_NAME} ${TEST_SOURCE})
#    target_include_directories(${TEST_NAME} PRIVATE ${PORYTILES_INCLUDE_DIR})
#    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main)
#    target_link_libraries(${TEST_NAME} PRIVATE Porytiles2xLib)
#    gtest_discover_tests(${TEST_NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#endforeach()

add_executable(Porytiles2xUnitTests ${TEST_SOURCES})
target_include_directories(Porytiles2xUnitTests PRIVATE ${PORYTILES_INCLUDE_DIR})
target_link_libraries(Porytiles2xUnitTests PRIVATE GTest::gtest_main)
target_link_libraries(Porytiles2xUnitTests PRIVATE Porytiles2xLib)
gtest_discover_tests(Porytiles2xUnitTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
