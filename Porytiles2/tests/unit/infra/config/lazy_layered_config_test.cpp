#include "gtest/gtest.h"

#include "porytiles2/infra/config/lazy_layered_config.hpp"

using namespace porytiles2;

// Mock ConfigLayerProvider for testing
class MockConfigLayerProvider final : public ConfigLayerProvider {
  public:
    explicit MockConfigLayerProvider(std::string name, std::string metadata = "test metadata")
        : name_{std::move(name)}, metadata_{std::move(metadata)} {}

    [[nodiscard]] std::string name() const override {
        return name_;
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary(const std::string &tileset_name) const override {
        if (name_ == "MockProvider1") {
            return LayerValue<std::size_t>{512, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total(const std::string &tileset_name) const override {
        if (name_ == "MockProvider2") {
            return LayerValue<std::size_t>{1024, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_primary(const std::string &tileset_name) const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{512, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_total(const std::string &tileset_name) const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{1024, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_primary(const std::string &tileset_name) const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{6, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_pals_total(const std::string &tileset_name) const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{13, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> max_map_data_size() const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{8192, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<std::size_t> num_tiles_per_metatile() const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<std::size_t>{8, metadata_};
        }
        return LayerValue<std::size_t>{std::nullopt, metadata_};
    }

    [[nodiscard]] LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const override {
        if (name_ == "DefaultProvider") {
            return LayerValue<IncrementalBuildMode>{IncrementalBuildMode::off, metadata_};
        }
        return LayerValue<IncrementalBuildMode>{std::nullopt, metadata_};
    }

  private:
    std::string name_;
    std::string metadata_;
};

TEST(LazyLayeredConfigTest, DumpShouldReturnNoCachedValuesWhenEmpty) {
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider1"));

    LazyLayeredConfig config{std::move(providers)};

    std::string dump_result = config.dump();
    EXPECT_EQ(dump_result, "LazyLayeredConfig {}");
}

TEST(LazyLayeredConfigTest, DumpShouldShowCachedValuesWithProvenance) {
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider1", "from config file"));
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider2", "from environment"));
    providers.push_back(std::make_unique<MockConfigLayerProvider>("DefaultProvider", "default value"));

    LazyLayeredConfig config{std::move(providers)};

    // Trigger caching by calling some config methods
    std::size_t tiles_primary = config.num_tiles_primary("test_tileset");
    std::size_t tiles_total = config.num_tiles_total("test_tileset");
    std::size_t max_map_size = config.max_map_data_size();

    EXPECT_EQ(tiles_primary, 512);
    EXPECT_EQ(tiles_total, 1024);
    EXPECT_EQ(max_map_size, 8192);

    std::string dump_result = config.dump();

    // Verify the dump contains the expected cached values and provenance
    EXPECT_TRUE(dump_result.find("LazyLayeredConfig {") != std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_primary:test_tileset = 512 [MockProvider1: from config file]") !=
                std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_total:test_tileset = 1024 [MockProvider2: from environment]") !=
                std::string::npos);
    EXPECT_TRUE(dump_result.find("max_map_data_size = 8192 [DefaultProvider: default value]") != std::string::npos);
}

TEST(LazyLayeredConfigTest, DumpShouldOnlyShowCachedValues) {
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider1"));
    providers.push_back(std::make_unique<MockConfigLayerProvider>("DefaultProvider"));

    LazyLayeredConfig config{std::move(providers)};

    // Only call one config method to cache one value
    std::size_t tiles_primary = config.num_tiles_primary("test_tileset");
    EXPECT_EQ(tiles_primary, 512);

    std::string dump_result = config.dump();

    // Should only show the one cached value
    EXPECT_TRUE(dump_result.find("num_tiles_primary:test_tileset = 512") != std::string::npos);
    EXPECT_FALSE(dump_result.find("num_tiles_total") != std::string::npos);
    EXPECT_FALSE(dump_result.find("max_map_data_size") != std::string::npos);
}

TEST(LazyLayeredConfigTest, WarmupCacheShouldCacheAllValues) {
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider1", "from config file"));
    providers.push_back(std::make_unique<MockConfigLayerProvider>("MockProvider2", "from environment"));
    providers.push_back(std::make_unique<MockConfigLayerProvider>("DefaultProvider", "default value"));

    LazyLayeredConfig config{std::move(providers)};

    // Verify cache is empty initially
    std::string initial_dump = config.dump();
    EXPECT_EQ(initial_dump, "LazyLayeredConfig {}");

    // Warmup cache with test tilesets
    std::vector<std::string> tileset_names{"test_tileset", "another_tileset"};
    config.warmup_cache(tileset_names);

    std::string warmed_dump = config.dump();
    std::cout << warmed_dump << std::endl;

    // Verify that all expected values are now cached
    EXPECT_TRUE(warmed_dump.find("LazyLayeredConfig {") != std::string::npos);

    // Tileset-specific values for both tilesets
    EXPECT_TRUE(warmed_dump.find("num_tiles_primary:test_tileset") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_total:test_tileset") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_primary:another_tileset") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_total:another_tileset") != std::string::npos);

    // Global values
    EXPECT_TRUE(warmed_dump.find("max_map_data_size") != std::string::npos);
    EXPECT_TRUE(warmed_dump.find("num_tiles_per_metatile") != std::string::npos);
}

TEST(LazyLayeredConfigTest, WarmupCacheWithEmptyTilesetList) {
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
    providers.push_back(std::make_unique<MockConfigLayerProvider>("DefaultProvider", "default value"));

    LazyLayeredConfig config{std::move(providers)};

    // Warmup with empty tileset list should only cache global values
    std::vector<std::string> empty_tilesets{};
    config.warmup_cache(empty_tilesets);

    std::string dump_result = config.dump();

    // Should have global values but no tileset-specific ones
    EXPECT_TRUE(dump_result.find("max_map_data_size = 8192") != std::string::npos);
    EXPECT_TRUE(dump_result.find("num_tiles_per_metatile = 8") != std::string::npos);
    EXPECT_FALSE(dump_result.find("num_tiles_primary") != std::string::npos);
    EXPECT_FALSE(dump_result.find("num_tiles_total") != std::string::npos);
}