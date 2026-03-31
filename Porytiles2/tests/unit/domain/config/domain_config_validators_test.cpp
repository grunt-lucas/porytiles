#include "gtest/gtest.h"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/config/packing_strategy_type.hpp"
#include "porytiles2/domain/config/tile_sharing_packing.hpp"

#include "support/mock_domain_config.hpp"

using namespace porytiles2;

namespace {

ChainableResult<ConfigValue<TileSharingPacking>>
call_tile_sharing_packing(const DomainConfig &config, ConfigScopeType type, const std::string &scope)
{
    return config.tile_sharing_packing(type, scope);
}

} // namespace

class DomainConfigValidatorTest : public ::testing::Test {
  protected:
    MockDomainConfig config_;
};

TEST_F(DomainConfigValidatorTest, OffWithAnyStrategyPasses)
{
    config_.tile_sharing_packing = TileSharingPacking::off;

    config_.packing_strategy = PackingStrategyType::best_fusion;
    auto result1 = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().value(), TileSharingPacking::off);

    config_.packing_strategy = PackingStrategyType::backtracking;
    auto result2 = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().value(), TileSharingPacking::off);

    config_.packing_strategy = PackingStrategyType::overload_and_remove;
    auto result3 = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().value(), TileSharingPacking::off);
}

TEST_F(DomainConfigValidatorTest, BiasedWithBacktrackingPasses)
{
    config_.tile_sharing_packing = TileSharingPacking::biased;
    config_.packing_strategy = PackingStrategyType::backtracking;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().value(), TileSharingPacking::biased);
}

TEST_F(DomainConfigValidatorTest, OptimalWithBacktrackingPasses)
{
    config_.tile_sharing_packing = TileSharingPacking::optimal;
    config_.packing_strategy = PackingStrategyType::backtracking;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().value(), TileSharingPacking::optimal);
}

TEST_F(DomainConfigValidatorTest, BiasedWithBestFusionFails)
{
    config_.tile_sharing_packing = TileSharingPacking::biased;
    config_.packing_strategy = PackingStrategyType::best_fusion;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    EXPECT_FALSE(result.has_value());
}

TEST_F(DomainConfigValidatorTest, BiasedWithOverloadAndRemoveFails)
{
    config_.tile_sharing_packing = TileSharingPacking::biased;
    config_.packing_strategy = PackingStrategyType::overload_and_remove;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    EXPECT_FALSE(result.has_value());
}

TEST_F(DomainConfigValidatorTest, OptimalWithBestFusionFails)
{
    config_.tile_sharing_packing = TileSharingPacking::optimal;
    config_.packing_strategy = PackingStrategyType::best_fusion;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    EXPECT_FALSE(result.has_value());
}

TEST_F(DomainConfigValidatorTest, OptimalWithOverloadAndRemoveFails)
{
    config_.tile_sharing_packing = TileSharingPacking::optimal;
    config_.packing_strategy = PackingStrategyType::overload_and_remove;

    auto result = call_tile_sharing_packing(config_, ConfigScopeType::tileset, "test");
    EXPECT_FALSE(result.has_value());
}
