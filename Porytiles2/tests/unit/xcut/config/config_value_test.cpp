#include "gtest/gtest.h"

#include <string>

#include "porytiles2/xcut/config/config_value.hpp"

using namespace porytiles2;

class ConfigValueTest : public ::testing::Test {};

TEST_F(ConfigValueTest, ValueAccessorReturnsCorrectValue)
{
    ConfigValue config{42, "test_config", "test source"};
    EXPECT_EQ(config.value(), 42);
}

TEST_F(ConfigValueTest, NameAccessorReturnsCorrectName)
{
    ConfigValue config{42, "num_tiles_primary", "test source"};
    EXPECT_EQ(config.name(), "num_tiles_primary");
}

TEST_F(ConfigValueTest, SourceAccessorReturnsCorrectSource)
{
    ConfigValue config{42, "test_config", "DefaultProvider: default value"};
    EXPECT_EQ(config.source(), "DefaultProvider: default value");
}

TEST_F(ConfigValueTest, ImplicitConversionToValue)
{
    ConfigValue config{100, "test_config", "test source"};
    int value = config;
    EXPECT_EQ(value, 100);
}

TEST_F(ConfigValueTest, ImplicitConversionInFunctionCall)
{
    auto add_ten = [](int x) { return x + 10; };
    ConfigValue config{5, "test_config", "test source"};
    EXPECT_EQ(add_ten(config), 15);
}

TEST_F(ConfigValueTest, WorksWithStdSizeT)
{
    ConfigValue<std::size_t> config{512, "max_map_data_size", "MockProvider: from test"};
    std::size_t value = config;
    EXPECT_EQ(value, 512);
    EXPECT_EQ(config.name(), "max_map_data_size");
    EXPECT_EQ(config.source(), "MockProvider: from test");
}

TEST_F(ConfigValueTest, WorksWithString)
{
    ConfigValue<std::string> config{"hello", "string_config", "test source"};
    EXPECT_EQ(config.value(), "hello");
    EXPECT_EQ(config.name(), "string_config");
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

    ConfigValue config{TestStruct{10, 20}, "complex_config", "computed source"};
    TestStruct value = config;
    EXPECT_EQ(value.x, 10);
    EXPECT_EQ(value.y, 20);
    EXPECT_EQ(config.name(), "complex_config");
    EXPECT_EQ(config.source(), "computed source");
}

TEST_F(ConfigValueTest, MoveSemantics)
{
    ConfigValue<std::string> config{"moveable", "string_config", "test source"};
    std::string moved_value = std::move(config).value();
    EXPECT_EQ(moved_value, "moveable");
}

TEST_F(ConfigValueTest, ComputedValueSourceFormat)
{
    std::string computed_source =
        "computed: num_tiles_total (MockTomlProvider: from toml) - num_tiles_primary (DefaultProvider: default value)";
    ConfigValue<std::size_t> config{1500, "num_tiles_secondary", computed_source};
    EXPECT_EQ(config.value(), 1500);
    EXPECT_EQ(config.name(), "num_tiles_secondary");
    EXPECT_EQ(config.source(), computed_source);
}

TEST_F(ConfigValueTest, ConstReferenceBinding)
{
    ConfigValue config{42, "test_config", "test source"};
    const int &ref = config.value();
    EXPECT_EQ(ref, 42);
}

TEST_F(ConfigValueTest, CopyConstructor)
{
    ConfigValue config1{42, "test_config", "original source"};
    ConfigValue<int> config2 = config1;
    EXPECT_EQ(config2.value(), 42);
    EXPECT_EQ(config2.name(), "test_config");
    EXPECT_EQ(config2.source(), "original source");
}
