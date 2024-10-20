# Get all sources in a list
FILE(GLOB TEST_SOURCES *.cpp)

include(GoogleTest)

# Create a separate test target for each unit
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_include_directories(${TEST_NAME} PRIVATE ${PORYTILES_INCLUDE_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main)
    target_link_libraries(${TEST_NAME} PRIVATE Porytiles2xLib)
    gtest_discover_tests(${TEST_NAME})
endforeach()
