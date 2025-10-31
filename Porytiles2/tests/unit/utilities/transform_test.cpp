#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "porytiles2/utilities/transform.hpp"

using namespace porytiles2;

class TransformTest : public ::testing::Test {};

TEST_F(TransformTest, MapIntegersToSquares)
{
    std::vector<int> input{1, 2, 3, 4, 5};
    auto result = map(input, [](int n) { return n * n; });

    std::vector<int> expected{1, 4, 9, 16, 25};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapIntegersToStrings)
{
    std::vector<int> input{1, 2, 3};
    auto result = map(input, [](int n) { return std::to_string(n); });

    std::vector<std::string> expected{"1", "2", "3"};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapEmptyVector)
{
    std::vector<int> input{};
    auto result = map(input, [](int n) { return n * 2; });

    EXPECT_TRUE(result.empty());
}
