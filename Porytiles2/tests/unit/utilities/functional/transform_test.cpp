#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "porytiles2/utilities/functional/transform.hpp"

using namespace porytiles2;

class TransformTest : public ::testing::Test {};

TEST_F(TransformTest, MapIntegersToSquares)
{
    std::vector<int> input{1, 2, 3, 4, 5};
    auto result = transform(input, [](int n) { return n * n; });

    std::vector<int> expected{1, 4, 9, 16, 25};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapIntegersToStrings)
{
    std::vector<int> input{1, 2, 3};
    auto result = transform(input, [](int n) { return std::to_string(n); });

    std::vector<std::string> expected{"1", "2", "3"};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapEmptyVector)
{
    std::vector<int> input{};
    auto result = transform(input, [](int n) { return n * 2; });

    EXPECT_TRUE(result.empty());
}

// Test helper class for type conversion tests
struct SourceType {
    int value;
    explicit SourceType(int v) : value{v} {}
};

struct TargetType {
    int value;
    explicit TargetType(const SourceType &src) : value{src.value * 10} {}

    bool operator==(const TargetType &other) const
    {
        return value == other.value;
    }
};

TEST_F(TransformTest, MapWithDirectTypeConversion_IntToDouble)
{
    std::vector<int> input{1, 2, 3, 4, 5};
    auto result = transform<double>(input);

    std::vector<double> expected{1.0, 2.0, 3.0, 4.0, 5.0};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapWithDirectTypeConversion_CustomTypes)
{
    std::vector<SourceType> input{SourceType{1}, SourceType{2}, SourceType{3}};
    auto result = transform<TargetType>(input);

    std::vector<TargetType> expected{TargetType{SourceType{1}}, TargetType{SourceType{2}}, TargetType{SourceType{3}}};
    EXPECT_EQ(result, expected);
}

TEST_F(TransformTest, MapWithDirectTypeConversion_EmptyVector)
{
    std::vector<int> input{};
    auto result = transform<double>(input);

    EXPECT_TRUE(result.empty());
}
