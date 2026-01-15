#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

class MockInfraConfig : public InfraConfig {
  protected:
    [[nodiscard]] ChainableResult<ConfigValue<TilesPalMode>>
    tiles_pal_mode_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{TilesPalMode::true_color, "tiles_pal_mode", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::string>>
    tileset_paths_primary_src_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::string{"data/tilesets/primary"}, "tileset_paths_primary_src", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::string>>
    tileset_paths_primary_bin_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::string{"data/tilesets/primary"}, "tileset_paths_primary_bin", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::string>>
    tileset_paths_secondary_src_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::string{"data/tilesets/secondary"}, "tileset_paths_secondary_src", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::string>>
    tileset_paths_secondary_bin_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::string{"data/tilesets/secondary"}, "tileset_paths_secondary_bin", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<bool>>
    tileset_animations_overwrite_callback_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{true, "tileset_animations_overwrite_callback", "mock", {}};
    }
};

} // namespace

/**
 * @brief Base fixture for ProjectTilesetMetadataProvider tests.
 *
 * @details
 * Subclass this fixture and override project_root_path() to test against different mock pokeemerald projects.
 */
class ProjectTilesetMetadataProviderTestBase : public ::testing::Test {
  protected:
    /**
     * @brief Returns the path to the mock pokeemerald project root.
     *
     * @details
     * Override this in derived fixtures to test against different project structures.
     */
    [[nodiscard]] virtual std::filesystem::path project_root_path() const = 0;

    void SetUp() override
    {
        project_root_ = project_root_path();

        ASSERT_TRUE(std::filesystem::exists(project_root_))
            << "Mock pokeemerald project not found at: " << project_root_;

        config_ = std::make_unique<MockInfraConfig>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();

        metadata_provider_ =
            std::make_unique<ProjectTilesetMetadataProvider>(project_root_, formatter_.get(), diag_.get());
        key_provider_ = std::make_unique<ProjectTilesetArtifactKeyProvider>(
            project_root_, config_.get(), formatter_.get(), diag_.get());
    }

    std::filesystem::path project_root_;
    std::unique_ptr<MockInfraConfig> config_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diag_;
    std::unique_ptr<ProjectTilesetMetadataProvider> metadata_provider_;
    std::unique_ptr<ProjectTilesetArtifactKeyProvider> key_provider_;
};

/**
 * @brief Tests using the pokeemerald_porytilestesttilesets mock project.
 */
class ProjectTilesetMetadataProviderTest_Fixture1 : public ProjectTilesetMetadataProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/repos/pokeemerald_porytilestesttilesets";
    }
};

TEST_F(ProjectTilesetMetadataProviderTest_Fixture1, GetsCorrectValuesForTilesets)
{
    // Test 1: Primary tileset with animations (gTileset_General)
    {
        auto result = metadata_provider_->metadata_for("gTileset_General");
        ASSERT_TRUE(result.has_value()) << "Failed to fetch metadata for gTileset_General";

        const auto &metadata = result.value();
        EXPECT_EQ(metadata.name(), "gTileset_General");
        EXPECT_FALSE(metadata.is_secondary());
        EXPECT_EQ(metadata.tiles_var(), "gTilesetTiles_General");
        EXPECT_EQ(metadata.palettes_var(), "gTilesetPalettes_General");
        EXPECT_EQ(metadata.metatiles_var(), "gMetatiles_General");
        EXPECT_EQ(metadata.metatile_attributes_var(), "gMetatileAttributes_General");
        EXPECT_TRUE(metadata.has_animations());
        ASSERT_TRUE(metadata.callback_func().has_value());
        EXPECT_EQ(metadata.callback_func().value(), "InitTilesetAnim_General");
    }

    // Test 2: Secondary tileset without animations (gTileset_Shop)
    {
        auto result = metadata_provider_->metadata_for("gTileset_Shop");
        ASSERT_TRUE(result.has_value()) << "Failed to fetch metadata for gTileset_Shop";

        const auto &metadata = result.value();
        EXPECT_EQ(metadata.name(), "gTileset_Shop");
        EXPECT_TRUE(metadata.is_secondary());
        EXPECT_EQ(metadata.tiles_var(), "gTilesetTiles_Shop");
        EXPECT_EQ(metadata.palettes_var(), "gTilesetPalettes_Shop");
        EXPECT_EQ(metadata.metatiles_var(), "gMetatiles_Shop");
        EXPECT_EQ(metadata.metatile_attributes_var(), "gMetatileAttributes_Shop");
        EXPECT_FALSE(metadata.has_animations());
    }

    // Test 3: Primary tileset with different variable naming (gTileset_Building)
    // Note: This tileset uses "InsideBuilding" in its variable names rather than "Building"
    {
        auto result = metadata_provider_->metadata_for("gTileset_Building");
        ASSERT_TRUE(result.has_value()) << "Failed to fetch metadata for gTileset_Building";

        const auto &metadata = result.value();
        EXPECT_EQ(metadata.name(), "gTileset_Building");
        EXPECT_FALSE(metadata.is_secondary());
        EXPECT_EQ(metadata.tiles_var(), "gTilesetTiles_InsideBuilding");
        EXPECT_EQ(metadata.palettes_var(), "gTilesetPalettes_InsideBuilding");
        EXPECT_EQ(metadata.metatiles_var(), "gMetatiles_InsideBuilding");
        EXPECT_EQ(metadata.metatile_attributes_var(), "gMetatileAttributes_InsideBuilding");
        EXPECT_TRUE(metadata.has_animations());
        ASSERT_TRUE(metadata.callback_func().has_value());
        EXPECT_EQ(metadata.callback_func().value(), "InitTilesetAnim_Building");
    }

    // Test 4: Primary tileset without animations (gTileset_SecretBase)
    {
        auto result = metadata_provider_->metadata_for("gTileset_SecretBase");
        ASSERT_TRUE(result.has_value()) << "Failed to fetch metadata for gTileset_SecretBase";

        const auto &metadata = result.value();
        EXPECT_EQ(metadata.name(), "gTileset_SecretBase");
        EXPECT_FALSE(metadata.is_secondary());
        EXPECT_EQ(metadata.tiles_var(), "gTilesetTiles_SecretBase");
        EXPECT_EQ(metadata.palettes_var(), "gTilesetPalettes_SecretBase");
        // Note: metatiles/attributes use different naming (SecretBasePrimary)
        EXPECT_EQ(metadata.metatiles_var(), "gMetatiles_SecretBasePrimary");
        EXPECT_EQ(metadata.metatile_attributes_var(), "gMetatileAttributes_SecretBasePrimary");
        EXPECT_FALSE(metadata.has_animations());
    }

    // Test 5: is_secondary() and has_animations() convenience methods
    {
        auto is_sec_result = metadata_provider_->is_secondary("gTileset_General");
        ASSERT_TRUE(is_sec_result.has_value());
        EXPECT_FALSE(is_sec_result.value());

        is_sec_result = metadata_provider_->is_secondary("gTileset_Shop");
        ASSERT_TRUE(is_sec_result.has_value());
        EXPECT_TRUE(is_sec_result.value());

        auto has_anim_result = metadata_provider_->has_animations("gTileset_General");
        ASSERT_TRUE(has_anim_result.has_value());
        EXPECT_TRUE(has_anim_result.value());

        has_anim_result = metadata_provider_->has_animations("gTileset_Shop");
        ASSERT_TRUE(has_anim_result.has_value());
        EXPECT_FALSE(has_anim_result.value());
    }
}

TEST_F(ProjectTilesetMetadataProviderTest_Fixture1, TilesetRootIsComputedByKeyProvider)
{
    // Test that tileset_root is now computed by the key provider, not metadata
    {
        auto result = key_provider_->tileset_root("gTileset_General");
        ASSERT_TRUE(result.has_value()) << "Failed to get tileset root for gTileset_General";

        const auto &tileset_root = result.value();
        EXPECT_TRUE(tileset_root.string().find("general") != std::string::npos)
            << "tileset_root should contain 'general', got: " << tileset_root.string();
    }

    // Test secondary tileset
    {
        auto result = key_provider_->tileset_root("gTileset_Shop");
        ASSERT_TRUE(result.has_value()) << "Failed to get tileset root for gTileset_Shop";

        const auto &tileset_root = result.value();
        EXPECT_FALSE(tileset_root.empty());
    }
}

TEST_F(ProjectTilesetMetadataProviderTest_Fixture1, ArtifactPathsForReturnsCorrectPaths)
{
    // Test artifact_paths_for with gTileset_General (via key provider)
    {
        auto result = key_provider_->artifact_paths_for("gTileset_General");
        ASSERT_TRUE(result.has_value()) << "Failed to get artifact paths for gTileset_General";

        const auto &paths = result.value();

        // Verify tiles path
        EXPECT_FALSE(paths.tiles_path().empty());
        EXPECT_TRUE(paths.tiles_path().string().find("tiles") != std::string::npos)
            << "tiles_path should contain 'tiles', got: " << paths.tiles_path().string();

        // Verify metatiles path
        EXPECT_FALSE(paths.metatiles_path().empty());
        EXPECT_TRUE(paths.metatiles_path().string().find("metatiles") != std::string::npos)
            << "metatiles_path should contain 'metatiles', got: " << paths.metatiles_path().string();
    }

    // Test artifact_paths_for with gTileset_Shop (secondary tileset, via key provider)
    {
        auto result = key_provider_->artifact_paths_for("gTileset_Shop");
        ASSERT_TRUE(result.has_value()) << "Failed to get artifact paths for gTileset_Shop";

        const auto &paths = result.value();

        // Verify basic paths are populated
        EXPECT_FALSE(paths.tiles_path().empty());
        EXPECT_FALSE(paths.metatiles_path().empty());
    }
}
