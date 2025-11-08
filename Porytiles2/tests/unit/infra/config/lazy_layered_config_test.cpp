#include "gtest/gtest.h"

#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"

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

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary(const std::string &tileset) const override
    {
        if (num_tiles_primary_.contains(tileset)) {
            return LayerValue<std::size_t>::valid(num_tiles_primary_.at(tileset), metadata_);
        }
        return LayerValue<std::size_t>::not_provided();
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total(const std::string &tileset) const override
    {
        if (num_tiles_total_.contains(tileset)) {
            return LayerValue<std::size_t>::valid(num_tiles_total_.at(tileset), metadata_);
        }
        return LayerValue<std::size_t>::not_provided();
    }

    [[nodiscard]] LayerValue<bool> patch_build_enabled(const std::string &tileset) const override
    {
        if (patch_build_enabled_.contains(tileset)) {
            return LayerValue<bool>::valid(patch_build_enabled_.at(tileset), metadata_);
        }
        return LayerValue<bool>::not_provided();
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
    mock_header->patch_build_enabled_[tileset_name] = true;
    mock_header->num_tiles_total_[tileset_name] = 0; // this value is overridden by toml layer
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::move(mock_env));
    providers.push_back(std::move(mock_header));
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    auto tiles_primary_result = config.num_tiles_primary(tileset_name);
    auto tiles_total_result = config.num_tiles_total(tileset_name);
    auto max_map_size_result = config.max_map_data_size(tileset_name);
    auto test_tileset_mode_result = config.patch_build_enabled("test_tileset");
    auto another_tileset_mode_result = config.patch_build_enabled("another_tileset");

    ASSERT_TRUE(tiles_primary_result.has_value());
    ASSERT_TRUE(tiles_total_result.has_value());
    ASSERT_TRUE(max_map_size_result.has_value());
    ASSERT_TRUE(test_tileset_mode_result.has_value());
    ASSERT_TRUE(another_tileset_mode_result.has_value());

    // The second value() call is unnecessary since ConfigValue provides implicit unwrapping
    EXPECT_EQ(tiles_primary_result.value().value(), 2000);
    EXPECT_EQ(tiles_total_result.value(), 4000);
    EXPECT_EQ(max_map_size_result.value(), 10240);
    EXPECT_EQ(test_tileset_mode_result.value(), true);
    EXPECT_EQ(another_tileset_mode_result.value(), false);
}

TEST(LazyLayeredConfigTest, DumpShouldReturnNoCachedValuesWhenCold)
{
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    std::string dump_result = config.dump();
    EXPECT_EQ(dump_result, "LazyLayeredConfig {}");
}

TEST(LazyLayeredConfigTest, DumpShouldShowCachedValuesWithProvenance)
{
    const std::string tileset_name = "test_tileset";
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_primary_[tileset_name] = 2000;
    auto mock_env = std::make_unique<MockConfigurableProvider>("MockEnvProvider", "from env");
    mock_env->num_tiles_total_[tileset_name] = 4000;
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::move(mock_env));
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Trigger caching by calling some config methods
    auto tiles_primary_result = config.num_tiles_primary(tileset_name);
    auto tiles_total_result = config.num_tiles_total(tileset_name);
    auto max_map_size_result = config.max_map_data_size(tileset_name);

    ASSERT_TRUE(tiles_primary_result.has_value());
    ASSERT_TRUE(tiles_total_result.has_value());
    ASSERT_TRUE(max_map_size_result.has_value());

    EXPECT_EQ(tiles_primary_result.value().value(), 2000);
    EXPECT_EQ(tiles_total_result.value().value(), 4000);
    EXPECT_EQ(max_map_size_result.value().value(), 10240);

    std::string dump_result = config.dump();

    // Verify the dump contains the expected cached values and provenance
    EXPECT_TRUE(dump_result.find("LazyLayeredConfig {") != std::string::npos);
    EXPECT_TRUE(dump_result.find("test_tileset:num_tiles_primary = 2000 [from toml file]") != std::string::npos);
    EXPECT_TRUE(dump_result.find("test_tileset:num_tiles_total = 4000 [from env]") != std::string::npos);
    EXPECT_TRUE(dump_result.find("test_tileset:max_map_data_size = 10240 [default value]") != std::string::npos);
}

TEST(LazyLayeredConfigTest, DumpShouldOnlyShowCachedValues)
{
    const std::string tileset_name = "test_tileset";
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    providers.push_back(std::make_unique<MockConfigurableProvider>("MockProvider1", "metadata"));
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Only call one config method to cache one value
    auto tiles_primary_result = config.num_tiles_primary(tileset_name);
    ASSERT_TRUE(tiles_primary_result.has_value());
    EXPECT_EQ(tiles_primary_result.value().value(), 512);

    std::string dump_result = config.dump();

    // Should only show the one cached value, which came from default layer
    EXPECT_TRUE(dump_result.find("test_tileset:num_tiles_primary = 512") != std::string::npos);
    EXPECT_FALSE(dump_result.find("test_tileset:num_tiles_total") != std::string::npos);
    EXPECT_FALSE(dump_result.find("test_tileset:max_map_data_size") != std::string::npos);
}

TEST(LazyLayeredConfigTest, WarmupCacheShouldCacheAllValues)
{
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_total_["test_tileset"] = 1000;
    mock_toml->num_tiles_primary_["another_tileset"] = 5000;
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::make_unique<DefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Verify cache is empty initially
    std::string initial_dump = config.dump();
    EXPECT_EQ(initial_dump, "LazyLayeredConfig {}");

    // Warmup cache with test tilesets
    std::vector<std::string> tileset_names{"test_tileset", "another_tileset"};
    config.warmup_cache(tileset_names);

    std::string warmed_dump = config.dump();

    // Verify that all expected values are now cached
    EXPECT_TRUE(warmed_dump.find("LazyLayeredConfig {") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("test_tileset:num_tiles_primary") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("another_tileset:num_tiles_primary = 5000 [from toml file]") != std::string::npos);

    EXPECT_TRUE(warmed_dump.find("test_tileset:num_tiles_total = 1000 [from toml file]") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("another_tileset:num_tiles_total") != std::string::npos);

    EXPECT_TRUE(warmed_dump.find("test_tileset:max_map_data_size") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("another_tileset:max_map_data_size") != std::string::npos);

    EXPECT_TRUE(warmed_dump.find("test_tileset:num_tiles_per_metatile") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("another_tileset:num_tiles_per_metatile") != std::string::npos);

    EXPECT_TRUE(warmed_dump.find("test_tileset:patch_build_enabled") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("another_tileset:patch_build_enabled") != std::string::npos);
}
