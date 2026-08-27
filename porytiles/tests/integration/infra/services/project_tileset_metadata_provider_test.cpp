#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>

#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/utilities/filesystem_utils.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_infra_config.hpp"

using namespace porytiles;

/// @brief Base fixture for ProjectTilesetMetadataProvider tests.
///
/// @details
/// Subclass this fixture and override project_root_path() to test against different mock pokeemerald projects.
class ProjectTilesetMetadataProviderTestBase : public ::testing::Test {
  protected:
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
    }

    std::filesystem::path project_root_;
    std::unique_ptr<MockInfraConfig> config_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diag_;
    std::unique_ptr<ProjectTilesetMetadataProvider> metadata_provider_;
};

class ProjectTilesetMetadataProviderTest_Fixture1 : public ProjectTilesetMetadataProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "resources/tests/integration/shared/repos/pokeemerald_porytilestesttilesets";
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

TEST_F(ProjectTilesetMetadataProviderTest_Fixture1, ArtifactPathsForReturnsCorrectPaths)
{
    // Test artifact_paths_for with gTileset_General (primary tileset)
    {
        auto result = metadata_provider_->artifact_paths_for("gTileset_General");
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

    // Test artifact_paths_for with gTileset_Shop (secondary tileset)
    {
        auto result = metadata_provider_->artifact_paths_for("gTileset_Shop");
        ASSERT_TRUE(result.has_value()) << "Failed to get artifact paths for gTileset_Shop";

        const auto &paths = result.value();

        // Verify basic paths are populated
        EXPECT_FALSE(paths.tiles_path().empty());
        EXPECT_FALSE(paths.metatiles_path().empty());
    }
}

class ProjectTilesetMetadataProviderTest_IncgfxTiles : public ProjectTilesetMetadataProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "resources/tests/integration/shared/repos/incgfx_tileset";
    }
};

TEST_F(ProjectTilesetMetadataProviderTest_IncgfxTiles, ArtifactPathsForResolvesIncgfxTiles)
{
    auto result = metadata_provider_->artifact_paths_for("gTileset_VelvetForest");
    ASSERT_TRUE(result.has_value()) << "INCGFX_U32-declared tiles should resolve. Got: "
                                    << result.chain().back()->join(*formatter_);

    const auto &paths = result.value();

    // The parser captures INCGFX's first string-literal argument: the source PNG, not the compiled binary.
    EXPECT_EQ(paths.tiles_path(), std::filesystem::path{"data/tilesets/primary/velvet_forest/tiles.png"});

    // The importer turns the stored path into the PNG to load via strip_all_extensions(...) + ".png".
    auto tiles_png = strip_all_extensions(paths.tiles_path());
    tiles_png += ".png";
    EXPECT_EQ(tiles_png, std::filesystem::path{"data/tilesets/primary/velvet_forest/tiles.png"});
}

class ProjectTilesetMetadataProviderTest_MissingVar : public ProjectTilesetMetadataProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "resources/tests/integration/shared/repos/missing_incbin_var";
    }
};

TEST_F(ProjectTilesetMetadataProviderTest_MissingVar, ArtifactPathsForMissingVarReportsDiagnostic)
{
    auto result = metadata_provider_->artifact_paths_for("gTileset_Broken");
    ASSERT_FALSE(result.has_value()) << "Resolution should fail when the tiles variable is undeclared.";

    std::string error_text = result.chain().back()->join(*formatter_);
    EXPECT_TRUE(error_text.find("gTilesetTiles_Broken") != std::string::npos)
        << "Diagnostic should name the unresolved variable. Got: " << error_text;
    EXPECT_TRUE(error_text.find("tiles") != std::string::npos)
        << "Diagnostic should name the artifact field. Got: " << error_text;
    EXPECT_TRUE(error_text.find("was not found in any scanned file") != std::string::npos)
        << "Diagnostic should explain the failure reason. Got: " << error_text;
}
