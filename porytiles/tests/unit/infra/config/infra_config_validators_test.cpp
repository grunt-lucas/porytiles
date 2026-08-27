#include "gtest/gtest.h"

#include <cstddef>
#include <optional>
#include <string>

#include "porytiles/infra/config/infra_config.hpp"

#include "support/mock_infra_config.hpp"

using namespace porytiles;

namespace {

// The mock's public data member shadows the inherited accessor, so call through the base interface.
ChainableResult<ConfigValue<std::optional<std::size_t>>>
call_metatile_attribute_size(const InfraConfig &config, ConfigScopeType type, const std::string &scope)
{
    return config.metatile_attribute_size(type, scope);
}

} // namespace

class InfraConfigValidatorTest : public ::testing::Test {
  protected:
    MockInfraConfig config_;
};

TEST_F(InfraConfigValidatorTest, MetatileAttributeSizeAcceptsOneTwoAndFour)
{
    for (const std::size_t size : {std::size_t{1}, std::size_t{2}, std::size_t{4}}) {
        config_.metatile_attribute_size = size;
        auto result = call_metatile_attribute_size(config_, ConfigScopeType::tileset, "test");
        ASSERT_TRUE(result.has_value()) << "size " << size;
        EXPECT_EQ(result.value().value(), size);
    }
}

TEST_F(InfraConfigValidatorTest, MetatileAttributeSizeAcceptsUnset)
{
    // nullopt is the definite answer "the user did not pin the width", not a value to validate.
    config_.metatile_attribute_size = std::nullopt;
    auto result = call_metatile_attribute_size(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().value(), std::nullopt);
}

TEST_F(InfraConfigValidatorTest, MetatileAttributeSizeRejectsOtherWidths)
{
    for (const std::size_t size : {std::size_t{0}, std::size_t{3}, std::size_t{8}}) {
        config_.metatile_attribute_size = size;
        auto result = call_metatile_attribute_size(config_, ConfigScopeType::tileset, "test");
        EXPECT_FALSE(result.has_value()) << "size " << size;
    }
}
