#include "gtest/gtest.h"

#include <string>

#include "porytiles2/xcut/config/config_value.hpp"

using namespace porytiles2;

class ConfigValueTest : public ::testing::Test {};

TEST_F(ConfigValueTest, ValueAccessorReturnsCorrectValue)
{
    ConfigValue config{42, "Test Config", "test_config", "test source", {}};
    EXPECT_EQ(config.value(), 42);
}

TEST_F(ConfigValueTest, CanonicalNameAccessorReturnsCorrectName)
{
    ConfigValue config{42, "Number Of Tiles In Primary", "num_tiles_primary", "test source", {}};
    EXPECT_EQ(config.canonical_name(), "Number Of Tiles In Primary");
}

TEST_F(ConfigValueTest, ProviderNameAccessorReturnsCorrectName)
{
    ConfigValue config{42, "Number Of Tiles In Primary", "fieldmap.num_tiles_in_primary", "test source", {}};
    EXPECT_EQ(config.provider_name(), "fieldmap.num_tiles_in_primary");
}

TEST_F(ConfigValueTest, SourceAccessorReturnsCorrectSource)
{
    ConfigValue config{42, "Test Config", "test_config", "DefaultProvider: default value", {}};
    EXPECT_EQ(config.source(), "DefaultProvider: default value");
}

TEST_F(ConfigValueTest, ImplicitConversionToValue)
{
    ConfigValue config{100, "Test Config", "test_config", "test source", {}};
    int value = config;
    EXPECT_EQ(value, 100);
}

TEST_F(ConfigValueTest, ImplicitConversionInFunctionCall)
{
    auto add_ten = [](int x) { return x + 10; };
    ConfigValue config{5, "Test Config", "test_config", "test source", {}};
    EXPECT_EQ(add_ten(config), 15);
}

TEST_F(ConfigValueTest, WorksWithStdSizeT)
{
    ConfigValue<std::size_t> config{512, "Max Map Data Size", "max_map_data_size", "MockProvider: from test", {}};
    std::size_t value = config;
    EXPECT_EQ(value, 512);
    EXPECT_EQ(config.canonical_name(), "Max Map Data Size");
    EXPECT_EQ(config.provider_name(), "max_map_data_size");
    EXPECT_EQ(config.source(), "MockProvider: from test");
}

TEST_F(ConfigValueTest, WorksWithString)
{
    ConfigValue<std::string> config{"hello", "String Config", "string_config", "test source", {}};
    EXPECT_EQ(config.value(), "hello");
    EXPECT_EQ(config.canonical_name(), "String Config");
    EXPECT_EQ(config.provider_name(), "string_config");
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

    ConfigValue config{TestStruct{10, 20}, "Complex Config", "complex_config", "computed source", {}};
    TestStruct value = config;
    EXPECT_EQ(value.x, 10);
    EXPECT_EQ(value.y, 20);
    EXPECT_EQ(config.canonical_name(), "Complex Config");
    EXPECT_EQ(config.provider_name(), "complex_config");
    EXPECT_EQ(config.source(), "computed source");
}

TEST_F(ConfigValueTest, MoveSemantics)
{
    ConfigValue<std::string> config{"moveable", "String Config", "string_config", "test source", {}};
    std::string moved_value = std::move(config).value();
    EXPECT_EQ(moved_value, "moveable");
}

TEST_F(ConfigValueTest, ComputedValueSourceFormat)
{
    std::string computed_source =
        "computed: num_tiles_total (MockTomlProvider: from toml) - num_tiles_primary (DefaultProvider: default value)";
    ConfigValue<std::size_t> config{1500, "Number Of Tiles Secondary", "num_tiles_secondary", computed_source, {}};
    EXPECT_EQ(config.value(), 1500);
    EXPECT_EQ(config.canonical_name(), "Number Of Tiles Secondary");
    EXPECT_EQ(config.provider_name(), "num_tiles_secondary");
    EXPECT_EQ(config.source(), computed_source);
}

TEST_F(ConfigValueTest, ConstReferenceBinding)
{
    ConfigValue config{42, "Test Config", "test_config", "test source", {}};
    const int &ref = config.value();
    EXPECT_EQ(ref, 42);
}

TEST_F(ConfigValueTest, CopyConstructor)
{
    ConfigValue config1{42, "Test Config", "test_config", "original source", {}};
    ConfigValue<int> config2 = config1;
    EXPECT_EQ(config2.value(), 42);
    EXPECT_EQ(config2.canonical_name(), "Test Config");
    EXPECT_EQ(config2.provider_name(), "test_config");
    EXPECT_EQ(config2.source(), "original source");
}
