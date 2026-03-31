#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

/**
 * @brief Base fixture for ProjectLayoutMetadataProvider tests.
 *
 * @details
 * Subclass this fixture and override project_root_path() to test against different mock pokeemerald projects.
 */
class ProjectLayoutMetadataProviderTestBase : public ::testing::Test {
  protected:
    [[nodiscard]] virtual std::filesystem::path project_root_path() const = 0;

    void SetUp() override
    {
        project_root_ = project_root_path();

        ASSERT_TRUE(std::filesystem::exists(project_root_))
            << "Mock pokeemerald project not found at: " << project_root_;

        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();

        layout_provider_ =
            std::make_unique<ProjectLayoutMetadataProvider>(project_root_, formatter_.get(), diag_.get());
    }

    std::filesystem::path project_root_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diag_;
    std::unique_ptr<ProjectLayoutMetadataProvider> layout_provider_;
};

class ProjectLayoutMetadataProviderTest_Fixture1 : public ProjectLayoutMetadataProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/shared/repos/pokeemerald_porytilestesttilesets";
    }
};

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, ExistsReturnsTrueForKnownLayoutsByName)
{
    EXPECT_TRUE(layout_provider_->exists("PetalburgCity_Layout"));
    EXPECT_TRUE(layout_provider_->exists("RustboroCity_Layout"));
    EXPECT_TRUE(layout_provider_->exists("PetalburgCity_PokemonCenter_1F_Layout"));
    EXPECT_TRUE(layout_provider_->exists("RustboroCity_BikeShop_Layout"));
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, ExistsReturnsTrueForKnownLayoutsById)
{
    EXPECT_TRUE(layout_provider_->exists("LAYOUT_PETALBURG_CITY"));
    EXPECT_TRUE(layout_provider_->exists("LAYOUT_RUSTBORO_CITY"));
    EXPECT_TRUE(layout_provider_->exists("LAYOUT_PETALBURG_CITY_POKEMON_CENTER_1F"));
    EXPECT_TRUE(layout_provider_->exists("LAYOUT_RUSTBORO_CITY_BIKE_SHOP"));
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, ExistsReturnsFalseForUnknownLayouts)
{
    EXPECT_FALSE(layout_provider_->exists("NonexistentLayout"));
    EXPECT_FALSE(layout_provider_->exists("LAYOUT_NONEXISTENT"));
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, WidthAndHeightReturnCorrectValues)
{
    // Test by name
    {
        auto width_result = layout_provider_->width("PetalburgCity_Layout");
        ASSERT_TRUE(width_result.has_value()) << "Failed to get width for PetalburgCity_Layout";
        EXPECT_EQ(width_result.value(), 30);

        auto height_result = layout_provider_->height("PetalburgCity_Layout");
        ASSERT_TRUE(height_result.has_value()) << "Failed to get height for PetalburgCity_Layout";
        EXPECT_EQ(height_result.value(), 30);
    }

    // Test by ID
    {
        auto width_result = layout_provider_->width("LAYOUT_RUSTBORO_CITY");
        ASSERT_TRUE(width_result.has_value()) << "Failed to get width for LAYOUT_RUSTBORO_CITY";
        EXPECT_EQ(width_result.value(), 40);

        auto height_result = layout_provider_->height("LAYOUT_RUSTBORO_CITY");
        ASSERT_TRUE(height_result.has_value()) << "Failed to get height for LAYOUT_RUSTBORO_CITY";
        EXPECT_EQ(height_result.value(), 60);
    }

    // Test small interior layout
    {
        auto width_result = layout_provider_->width("LAYOUT_PETALBURG_CITY_POKEMON_CENTER_1F");
        ASSERT_TRUE(width_result.has_value());
        EXPECT_EQ(width_result.value(), 11);

        auto height_result = layout_provider_->height("LAYOUT_PETALBURG_CITY_POKEMON_CENTER_1F");
        ASSERT_TRUE(height_result.has_value());
        EXPECT_EQ(height_result.value(), 8);
    }
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, TilesetNamesReturnCorrectValues)
{
    // Outdoor layout with General primary tileset
    {
        auto primary_result = layout_provider_->primary_tileset("PetalburgCity_Layout");
        ASSERT_TRUE(primary_result.has_value()) << "Failed to get primary tileset for PetalburgCity_Layout";
        EXPECT_EQ(primary_result.value(), "gTileset_General");

        auto secondary_result = layout_provider_->secondary_tileset("PetalburgCity_Layout");
        ASSERT_TRUE(secondary_result.has_value()) << "Failed to get secondary tileset for PetalburgCity_Layout";
        EXPECT_EQ(secondary_result.value(), "gTileset_Petalburg");
    }

    // Interior layout with Building primary tileset
    {
        auto primary_result = layout_provider_->primary_tileset("LAYOUT_PETALBURG_CITY_POKEMON_CENTER_1F");
        ASSERT_TRUE(primary_result.has_value());
        EXPECT_EQ(primary_result.value(), "gTileset_Building");

        auto secondary_result = layout_provider_->secondary_tileset("LAYOUT_PETALBURG_CITY_POKEMON_CENTER_1F");
        ASSERT_TRUE(secondary_result.has_value());
        EXPECT_EQ(secondary_result.value(), "gTileset_PokemonCenter");
    }
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, LayoutsTableLabelReturnsCorrectValue)
{
    auto result = layout_provider_->layouts_table_label();
    ASSERT_TRUE(result.has_value()) << "Failed to get layouts table label";
    EXPECT_EQ(result.value(), "gMapLayouts");
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, BorderFilepathReturnsResolvedPath)
{
    auto result = layout_provider_->border_filepath("PetalburgCity_Layout");
    ASSERT_TRUE(result.has_value()) << "Failed to get border filepath for PetalburgCity_Layout";

    const auto expected = project_root_ / "data/layouts/PetalburgCity/border.bin";
    EXPECT_EQ(result.value(), expected);
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, BlockdataFilepathReturnsResolvedPath)
{
    auto result = layout_provider_->blockdata_filepath("LAYOUT_RUSTBORO_CITY");
    ASSERT_TRUE(result.has_value()) << "Failed to get blockdata filepath for LAYOUT_RUSTBORO_CITY";

    const auto expected = project_root_ / "data/layouts/RustboroCity/map.bin";
    EXPECT_EQ(result.value(), expected);
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, LookupByNameAndIdReturnSameValues)
{
    auto width_by_name = layout_provider_->width("PetalburgCity_Layout");
    auto width_by_id = layout_provider_->width("LAYOUT_PETALBURG_CITY");

    ASSERT_TRUE(width_by_name.has_value());
    ASSERT_TRUE(width_by_id.has_value());
    EXPECT_EQ(width_by_name.value(), width_by_id.value());

    auto primary_by_name = layout_provider_->primary_tileset("PetalburgCity_Layout");
    auto primary_by_id = layout_provider_->primary_tileset("LAYOUT_PETALBURG_CITY");

    ASSERT_TRUE(primary_by_name.has_value());
    ASSERT_TRUE(primary_by_id.has_value());
    EXPECT_EQ(primary_by_name.value(), primary_by_id.value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, WidthReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->width("LAYOUT_NONEXISTENT");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, HeightReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->height("LAYOUT_NONEXISTENT");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, PrimaryTilesetReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->primary_tileset("NonexistentLayout");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, SecondaryTilesetReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->secondary_tileset("NonexistentLayout");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, BorderFilepathReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->border_filepath("NonexistentLayout");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProjectLayoutMetadataProviderTest_Fixture1, BlockdataFilepathReturnsErrorForNonexistentLayout)
{
    auto result = layout_provider_->blockdata_filepath("NonexistentLayout");
    EXPECT_FALSE(result.has_value());
}
