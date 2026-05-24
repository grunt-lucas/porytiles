#include "gtest/gtest.h"

#include <source_location>

#include "porytiles/utilities/source_locations.hpp"

using namespace porytiles;

class SourceLocationsTest : public ::testing::Test {};

namespace {
std::string get_current_function_name(const std::source_location location = std::source_location::current())
{
    return extract_function_name(location);
}
} // namespace

// Test with actual std::source_location output from the current compiler
TEST_F(SourceLocationsTest, RealSourceLocationOutput)
{
    const std::string function_name = get_current_function_name();
    // Google Test creates a TestBody() method for each test
    EXPECT_EQ(function_name, "TestBody");
}
