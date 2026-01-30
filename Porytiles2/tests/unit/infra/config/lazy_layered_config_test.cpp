#include "gtest/gtest.h"

#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"

using namespace porytiles2;

class MockConfigurableProvider final : public ConfigProvider {
  public:
    explicit MockConfigurableProvider(std::string name, std::string metadata)
        : name_{std::move(name)}, metadata_{std::move(metadata)}
    {
    }

    // Public fields to make tests easy to write, reduce bloat
    std::unordered_map<std::string, std::size_t> num_tiles_primary_;
    std::unordered_map<std::string, std::size_t> num_tiles_total_;
    std::unordered_map<std::string, bool> patch_build_enabled_;

    [[nodiscard]] std::string name() const override
    {
        return name_;
    }

    [[nodiscard]] LayerValue<std::size_t>
    num_tiles_in_primary(ConfigScopeType type, const std::string &scope) const override
    {
        if (num_tiles_primary_.contains(scope)) {
            return LayerValue<std::size_t>::valid(num_tiles_primary_.at(scope), metadata_);
        }
        return LayerValue<std::size_t>::not_provided();
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total(ConfigScopeType type, const std::string &scope) const override
    {
        if (num_tiles_total_.contains(scope)) {
            return LayerValue<std::size_t>::valid(num_tiles_total_.at(scope), metadata_);
        }
        return LayerValue<std::size_t>::not_provided();
    }

  private:
    std::string name_;
    std::string metadata_;
};

TEST(LazyLayeredConfigTest, OverrideLayeringShouldSelectHighestPriorityValue)
{
    const std::string tileset_name = "test_tileset";
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_primary_[tileset_name] = 2000;
    auto mock_env = std::make_unique<MockConfigurableProvider>("MockEnvProvider", "from env");
    mock_env->num_tiles_total_[tileset_name] = 4000;
    auto mock_header = std::make_unique<MockConfigurableProvider>("MockHeaderProvider", "from header");
    mock_header->num_tiles_total_[tileset_name] = 0; // this value is overridden by toml layer
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::move(mock_env));
    providers.push_back(std::move(mock_header));
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    auto tiles_primary_result = config.num_tiles_in_primary(ConfigScopeType::tileset, tileset_name);
    auto tiles_total_result = config.num_tiles_total(ConfigScopeType::tileset, tileset_name);
    auto max_map_size_result = config.max_map_data_size(ConfigScopeType::tileset, tileset_name);

    ASSERT_TRUE(tiles_primary_result.has_value());
    ASSERT_TRUE(tiles_total_result.has_value());
    ASSERT_TRUE(max_map_size_result.has_value());

    // The second value() call is unnecessary since ConfigValue provides implicit unwrapping
    EXPECT_EQ(tiles_primary_result.value().value(), 2000);
    EXPECT_EQ(tiles_total_result.value(), 4000);
    EXPECT_EQ(max_map_size_result.value(), 10240);
}
