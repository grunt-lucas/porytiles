#include "gtest/gtest.h"

#include "porytiles2/infra/services/project_vanilla_anim_importer.hpp"

#include <filesystem>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles2 {

class ProjectVanillaAnimImporterTest : public ::testing::Test {
  protected:
    static constexpr auto kTestProjectRoot = "Resources/Tests/integration/shared/repos/pokeemerald_vanilla_stock";

    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
    ProjectVanillaAnimImporter importer_{std::filesystem::path{kTestProjectRoot}, &formatter_, &diag_};
};

// =============================================================================
// General Tileset Animation Import Tests
// =============================================================================

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsDiscoversAllAnimationsForGeneralTileset)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value()) << "Importing animations should succeed";
    const auto &animations = result.value();

    // General tileset has 5 animations
    EXPECT_EQ(animations.size(), 5u);

    // Verify all expected animations are present
    EXPECT_TRUE(animations.contains("flower"));
    EXPECT_TRUE(animations.contains("land_water_edge"));
    EXPECT_TRUE(animations.contains("sand_water_edge"));
    EXPECT_TRUE(animations.contains("water"));
    EXPECT_TRUE(animations.contains("waterfall"));
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsExtractsFlowerAnimationParams)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("flower"));
    const auto &flower = animations.at("flower");
    const auto &params = flower.params();

    // Verify AnimationParams from tileset_anims.c parsing
    EXPECT_EQ(params.tile_offset(), 508u);
    EXPECT_EQ(params.tile_count(), 4u);
    EXPECT_EQ(params.frame_factor(), 16u);
    EXPECT_EQ(params.frame_offset(), 0u);

    // Verify frame order (0, 1, 0, 2)
    const auto &frame_order = params.frame_order();
    ASSERT_EQ(frame_order.size(), 4u);
    EXPECT_EQ(frame_order[0], "0");
    EXPECT_EQ(frame_order[1], "1");
    EXPECT_EQ(frame_order[2], "0");
    EXPECT_EQ(frame_order[3], "2");
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsExtractsWaterAnimationParams)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("water"));
    const auto &water = animations.at("water");
    const auto &params = water.params();

    // Verify AnimationParams
    EXPECT_EQ(params.tile_offset(), 432u);
    EXPECT_EQ(params.tile_count(), 30u);
    EXPECT_EQ(params.frame_factor(), 16u);
    EXPECT_EQ(params.frame_offset(), 1u);

    // Verify frame order (8 frames)
    const auto &frame_order = params.frame_order();
    ASSERT_EQ(frame_order.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(frame_order[i], std::to_string(i));
    }
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsDoesNotSetKeyFrame)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    // Key frame extraction is AnimationDecompiler's job, not ours
    for (const auto &[name, anim] : animations) {
        EXPECT_FALSE(anim.has_key_frame()) << "Animation '" << name << "' should not have key frame set";
    }
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsPopulatesFrames)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("flower"));
    const auto &flower = animations.at("flower");

    // Flower has 3 unique frames (0, 1, 2) from the anim folder
    EXPECT_EQ(flower.frames_values().size(), 3u);

    // Verify frames are loaded (0, 1, 2)
    const auto &frames = flower.frames_values();
    std::vector<std::string> frame_names;
    frame_names.reserve(frames.size());
    for (const auto &frame : frames) {
        frame_names.push_back(frame.frame_name());
    }

    EXPECT_TRUE(std::find(frame_names.begin(), frame_names.end(), "0") != frame_names.end());
    EXPECT_TRUE(std::find(frame_names.begin(), frame_names.end(), "1") != frame_names.end());
    EXPECT_TRUE(std::find(frame_names.begin(), frame_names.end(), "2") != frame_names.end());
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsFramesHaveCorrectTileCount)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("water"));
    const auto &water = animations.at("water");

    // Water animation has 30 tiles per frame
    for (const auto &frame : water.frames_values()) {
        EXPECT_EQ(frame.tiles().size(), 30u) << "Frame '" << frame.frame_name() << "' should have 30 tiles";
    }
}

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsFrameTilesAreIndexPixel)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("flower"));
    const auto &flower = animations.at("flower");
    ASSERT_TRUE(flower.has_frames());

    // Verify we can access IndexPixel data from the tiles
    const auto &frame0 = flower.frame_for_name("0");
    ASSERT_GT(frame0.tiles().size(), 0u);

    // IndexPixel tiles have an index() method - verify we can access it
    const auto &tile = frame0.tiles().front();
    // Just verify we can read pixel data without crashing
    // IndexPixel pixels have index values 0-15
    EXPECT_LE(tile.at(0, 0).index(), 15u);
}

// =============================================================================
// Tileset Without Animations Tests
// =============================================================================

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsReturnsEmptyMapForTilesetWithoutAnimations)
{
    // Shop tileset has callback = NULL, meaning no animations
    auto result = importer_.import_animations("gTileset_Shop");

    ASSERT_TRUE(result.has_value()) << "Import should succeed even for tilesets without animations";
    EXPECT_TRUE(result.value().empty()) << "Shop tileset should have no animations";
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsReturnsErrorForNonExistentTileset)
{
    auto result = importer_.import_animations("NonExistentTileset");

    EXPECT_FALSE(result.has_value()) << "Import should fail for non-existent tileset";
}

// =============================================================================
// Waterfall Animation Tests
// =============================================================================

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsExtractsWaterfallAnimation)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("waterfall"));
    const auto &waterfall = animations.at("waterfall");
    const auto &params = waterfall.params();

    // Waterfall animation params from tileset_anims.c
    EXPECT_GT(params.tile_offset(), 0u);
    EXPECT_GT(params.tile_count(), 0u);

    // Waterfall has 4 unique frames
    EXPECT_EQ(waterfall.frames_values().size(), 4u);
}

// =============================================================================
// Sand Water Edge Animation Tests
// =============================================================================

TEST_F(ProjectVanillaAnimImporterTest, ImportAnimationsExtractsSandWaterEdgeAnimation)
{
    auto result = importer_.import_animations("gTileset_General");

    ASSERT_TRUE(result.has_value());
    const auto &animations = result.value();

    ASSERT_TRUE(animations.contains("sand_water_edge"));
    const auto &sand_water_edge = animations.at("sand_water_edge");
    const auto &params = sand_water_edge.params();

    // sand_water_edge has params from tileset_anims.c
    EXPECT_GT(params.tile_offset(), 0u);
    EXPECT_GT(params.tile_count(), 0u);

    // sand_water_edge has 7 unique frames (0-6)
    EXPECT_EQ(sand_water_edge.frames_values().size(), 7u);
}

} // namespace porytiles2
