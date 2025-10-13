#include "gtest/gtest.h"

#include <string>

#include "porytiles2/xcut/config/config_value.hpp"

using namespace porytiles2;

class ConfigValueTest : public ::testing::Test {};

TEST_F(ConfigValueTest, ValueAccessorReturnsCorrectValue)
{
    ConfigValue config{42, "test source"};
    EXPECT_EQ(config.value(), 42);
}

TEST_F(ConfigValueTest, SourceAccessorReturnsCorrectSource)
{
    ConfigValue config{42, "DefaultProvider: default value"};
    EXPECT_EQ(config.source(), "DefaultProvider: default value");
}

TEST_F(ConfigValueTest, ImplicitConversionToValue)
{
    ConfigValue config{100, "test source"};
    int value = config;
    EXPECT_EQ(value, 100);
}

TEST_F(ConfigValueTest, ImplicitConversionInFunctionCall)
{
    auto add_ten = [](int x) { return x + 10; };
    ConfigValue config{5, "test source"};
    EXPECT_EQ(add_ten(config), 15);
}

TEST_F(ConfigValueTest, WorksWithStdSizeT)
{
    ConfigValue<std::size_t> config{512, "MockProvider: from test"};
    std::size_t value = config;
    EXPECT_EQ(value, 512);
    EXPECT_EQ(config.source(), "MockProvider: from test");
}

TEST_F(ConfigValueTest, WorksWithString)
{
    ConfigValue<std::string> config{"hello", "test source"};
    EXPECT_EQ(config.value(), "hello");
    EXPECT_EQ(config.source(), "test source");
}

TEST_F(ConfigValueTest, WorksWithComplexTypes)
{
    struct TestStruct {
        int x;
        int y;
        bool operator==(const TestStruct &other) const
        {
            return x == other.x && y == other.y;
        }
    };

    ConfigValue config{TestStruct{10, 20}, "computed source"};
    TestStruct value = config;
    EXPECT_EQ(value.x, 10);
    EXPECT_EQ(value.y, 20);
    EXPECT_EQ(config.source(), "computed source");
}

TEST_F(ConfigValueTest, MoveSemantics)
{
    ConfigValue<std::string> config{"moveable", "test source"};
    std::string moved_value = std::move(config).value();
    EXPECT_EQ(moved_value, "moveable");
}

TEST_F(ConfigValueTest, ComputedValueSourceFormat)
{
    std::string computed_source =
        "computed: num_tiles_total (MockTomlProvider: from toml) - num_tiles_primary (DefaultProvider: default value)";
    ConfigValue<std::size_t> config{1500, computed_source};
    EXPECT_EQ(config.value(), 1500);
    EXPECT_EQ(config.source(), computed_source);
}

TEST_F(ConfigValueTest, ConstReferenceBinding)
{
    ConfigValue config{42, "test source"};
    const int &ref = config.value();
    EXPECT_EQ(ref, 42);
}

TEST_F(ConfigValueTest, CopyConstructor)
{
    ConfigValue config1{42, "original source"};
    ConfigValue<int> config2 = config1;
    EXPECT_EQ(config2.value(), 42);
    EXPECT_EQ(config2.source(), "original source");
}
