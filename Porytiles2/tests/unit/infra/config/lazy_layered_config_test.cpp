#include "gtest/gtest.h"

#include "porytiles2/infra/config/lazy_layered_config.hpp"

using namespace porytiles2;

class MockDefaultProvider final : public ConfigProvider {
  public:
    explicit MockDefaultProvider() : name_{"MockDefaultProvider"}, metadata_{"defaulted"} {}

    [[nodiscard]] std::string name() const override {
        return name_;
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary() const override {
        return LayerValue<std::size_t>{512, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total() const override {
        return LayerValue<std::size_t>{1024, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_primary() const override {
        return LayerValue<std::size_t>{512, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_total() const override {
        return LayerValue<std::size_t>{1024, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_primary() const override {
        return LayerValue<std::size_t>{6, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_total() const override {
        return LayerValue<std::size_t>{13, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> max_map_data_size() const override {
        return LayerValue<std::size_t>{8192, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_per_metatile() const override {
        return LayerValue<std::size_t>{8, metadata_};
    }

    [[nodiscard]] LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const override {
        return LayerValue<IncrementalBuildMode>{IncrementalBuildMode::off, metadata_};
    }

  private:
    std::string name_;
    std::string metadata_;
};

class MockConfigurableProvider final : public ConfigProvider {
  public:
    explicit MockConfigurableProvider(std::string name, std::string metadata)
        : name_{std::move(name)}, metadata_{std::move(metadata)} {}

    // Public fields to make tests easy to write, reduce bloat
    std::optional<size_t> num_tiles_primary_;
    std::optional<size_t> num_tiles_total_;
    std::unordered_map<std::string, IncrementalBuildMode> incremental_build_modes_;

    [[nodiscard]] std::string name() const override {
        return name_;
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary() const override {
        if (num_tiles_primary_.has_value()) {
            return LayerValue<std::size_t>{num_tiles_primary_.value(), metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total() const override {
        if (num_tiles_total_.has_value()) {
            return LayerValue<std::size_t>{num_tiles_total_.value(), metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_primary() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_total() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_primary() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_total() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> max_map_data_size() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_per_metatile() const override {
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const override {
        if (incremental_build_modes_.contains(tileset_name)) {
            return LayerValue<IncrementalBuildMode>{incremental_build_modes_.at(tileset_name), metadata_};
        }
        return LayerValue<IncrementalBuildMode>{std::nullopt, metadata_};
    }

  private:
    std::string name_;
    std::string metadata_;
};

TEST(LazyLayeredConfigTest, OverrideLayeringShouldSelectHighestPriorityValue) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_primary_ = 2000;
    auto mock_env = std::make_unique<MockConfigurableProvider>("MockEnvProvider", "from env");
    mock_env->num_tiles_total_ = 4000;
    auto mock_header = std::make_unique<MockConfigurableProvider>("MockHeaderProvider", "from header");
    mock_header->incremental_build_modes_["test_tileset"] = IncrementalBuildMode::keep_unused;
    mock_header->num_tiles_total_ = 0; // this value is overridden by toml layer
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::move(mock_env));
    providers.push_back(std::move(mock_header));
    providers.push_back(std::make_unique<MockDefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    std::size_t tiles_primary = config.num_tiles_primary();
    std::size_t tiles_total = config.num_tiles_total();
    std::size_t tiles_secondary = config.num_tiles_secondary();
    std::size_t max_map_size = config.max_map_data_size();
    IncrementalBuildMode test_tileset_mode = config.incremental_build_mode("test_tileset");
    IncrementalBuildMode another_tileset_mode = config.incremental_build_mode("another_tileset");

    EXPECT_EQ(tiles_primary, 2000);
    EXPECT_EQ(tiles_total, 4000);
    EXPECT_EQ(tiles_secondary, 2000);
    EXPECT_EQ(max_map_size, 8192);
    EXPECT_EQ(test_tileset_mode, IncrementalBuildMode::keep_unused);
    EXPECT_EQ(another_tileset_mode, IncrementalBuildMode::off);
}

TEST(LazyLayeredConfigTest, DumpShouldReturnNoCachedValuesWhenCold) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    providers.push_back(std::make_unique<MockDefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    std::string dump_result = config.dump();
    EXPECT_EQ(dump_result, "LazyLayeredConfig {}");
}

TEST(LazyLayeredConfigTest, DumpShouldShowCachedValuesWithProvenance) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_primary_ = 2000;
    auto mock_env = std::make_unique<MockConfigurableProvider>("MockEnvProvider", "from env");
    mock_env->num_tiles_total_ = 4000;
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::move(mock_env));
    providers.push_back(std::make_unique<MockDefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Trigger caching by calling some config methods
    std::size_t tiles_primary = config.num_tiles_primary();
    std::size_t tiles_total = config.num_tiles_total();
    std::size_t max_map_size = config.max_map_data_size();

    EXPECT_EQ(tiles_primary, 2000);
    EXPECT_EQ(tiles_total, 4000);
    EXPECT_EQ(max_map_size, 8192);

    std::string dump_result = config.dump();

    // Verify the dump contains the expected cached values and provenance
    EXPECT_TRUE(dump_result.find("LazyLayeredConfig {") != std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_primary = 2000 [MockTomlProvider: from toml file]") != std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_total = 4000 [MockEnvProvider: from env]") != std::string::npos);
    EXPECT_TRUE(dump_result.find("max_map_data_size = 8192 [MockDefaultProvider: defaulted]") != std::string::npos);
}

TEST(LazyLayeredConfigTest, DumpShouldOnlyShowCachedValues) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    providers.push_back(std::make_unique<MockConfigurableProvider>("MockProvider1", "metadata"));
    providers.push_back(std::make_unique<MockDefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Only call one config method to cache one value
    std::size_t tiles_primary = config.num_tiles_primary();
    EXPECT_EQ(tiles_primary, 512);

    std::string dump_result = config.dump();

    // Should only show the one cached value, which came from default layer
    EXPECT_TRUE(dump_result.find("num_tiles_primary = 512") != std::string::npos);
    EXPECT_FALSE(dump_result.find("num_tiles_total") != std::string::npos);
    EXPECT_FALSE(dump_result.find("max_map_data_size") != std::string::npos);
}

TEST(LazyLayeredConfigTest, WarmupCacheShouldCacheAllValues) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    auto mock_toml = std::make_unique<MockConfigurableProvider>("MockTomlProvider", "from toml file");
    mock_toml->num_tiles_total_ = 1000;
    providers.push_back(std::move(mock_toml));
    providers.push_back(std::make_unique<MockDefaultProvider>());

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
    EXPECT_TRUE(warmed_dump.find("num_tiles_primary") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_total = 1000 [MockTomlProvider: from toml file]") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_primary") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_total") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("max_map_data_size") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_per_metatile") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("incremental_build_mode:test_tileset") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("incremental_build_mode:another_tileset") != std::string::npos);
}

TEST(LazyLayeredConfigTest, WarmupCacheWithEmptyTilesetList) {
    std::vector<std::unique_ptr<ConfigProvider>> providers;
    providers.push_back(std::make_unique<MockDefaultProvider>());

    LazyLayeredConfig config{std::move(providers)};

    // Warmup with empty tileset list should only cache global values
    std::vector<std::string> empty_tilesets{};
    config.warmup_cache(empty_tilesets);

    std::string dump_result = config.dump();

    // Should have global values but no tileset-specific ones
    EXPECT_TRUE(dump_result.find("num_tiles_primary = 512") != std::string::npos);
    EXPECT_TRUE(dump_result.find("max_map_data_size = 8192") != std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_per_metatile = 8") != std::string::npos);
    EXPECT_FALSE(dump_result.find("incremental_build_mode") != std::string::npos);
}