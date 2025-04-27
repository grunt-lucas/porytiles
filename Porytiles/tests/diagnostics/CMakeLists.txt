# Create the test executable
add_executable(DiagnosticEngineTest diagnostic_engine_test.cpp)

target_include_directories(DiagnosticEngineTest PRIVATE ${PORYTILES_INCLUDE_DIR})

# Link against Google Test and Porytiles library
target_link_libraries(DiagnosticEngineTest PRIVATE GTest::gtest_main)
target_link_libraries(DiagnosticEngineTest PRIVATE PorytilesLib)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(DiagnosticEngineTest)
