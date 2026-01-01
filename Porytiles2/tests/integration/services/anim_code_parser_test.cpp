#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <gtest/gtest.h>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles2 {

class AnimCodeParserTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
    AnimCodeParser parser_{&formatter_, &diag_};
};

// =============================================================================
// Generated Header Format Tests (Porytiles-managed)
// =============================================================================

TEST_F(AnimCodeParserTest, ParseFromCallbackDiscoversAllAnimationsInGeneratedHeader)
{
    // The callback for Porytiles-managed tilesets: InitTilesetAnim_PorytilesManaged_General
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/generated_anim_code.h", callback_func, "General", true);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    // Should discover all 5 animations without needing to provide expected_anim_names
    EXPECT_EQ(anims.size(), 5u)
        << "Should discover 5 animations (flower, land_water_edge, sand_water_edge, water, waterfall)";

    EXPECT_TRUE(anims.contains("flower"));
    EXPECT_TRUE(anims.contains("land_water_edge"));
    EXPECT_TRUE(anims.contains("sand_water_edge"));
    EXPECT_TRUE(anims.contains("water"));
    EXPECT_TRUE(anims.contains("waterfall"));
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsFlowerAnimationParamsFromGeneratedHeader)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/generated_anim_code.h", callback_func, "General", true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation (should be keyed as "flower" in snake_case)
    ASSERT_TRUE(anims.contains("flower")) << "Should contain 'flower' animation";
    const auto &flower = anims.at("flower");

    // Verify tile_offset from TILE_OFFSET_4BPP(508)
    EXPECT_EQ(flower.tile_offset(), 508u);

    // Verify tile_count from 4 * TILE_SIZE_4BPP
    EXPECT_EQ(flower.tile_count(), 4u);

    // Verify frame_factor and frame_offset from timer % 16 == 0
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 0u);

    // Verify frames from pointer array [Frame0, Frame1, Frame0, Frame2]
    const auto &frames = flower.frames();
    ASSERT_EQ(frames.size(), 4u);
    EXPECT_EQ(frames[0], "0");
    EXPECT_EQ(frames[1], "1");
    EXPECT_EQ(frames[2], "0");
    EXPECT_EQ(frames[3], "2");
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsLandWaterEdgeAnimationParamsFromGeneratedHeader)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/generated_anim_code.h", callback_func, "General", true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains("land_water_edge")) << "Should contain 'land_water_edge' animation";
    const auto &land_water_edge = anims.at("land_water_edge");

    // Verify tile_offset from TILE_OFFSET_4BPP(480)
    EXPECT_EQ(land_water_edge.tile_offset(), 480u);

    // Verify tile_count from 10 * TILE_SIZE_4BPP
    EXPECT_EQ(land_water_edge.tile_count(), 10u);

    // Verify frame_factor and frame_offset from timer % 16 == 4
    EXPECT_EQ(land_water_edge.frame_factor(), 16u);
    EXPECT_EQ(land_water_edge.frame_offset(), 4u);

    // Verify frames from pointer array (4 frames for vanilla land_water_edge)
    const auto &frames = land_water_edge.frames();
    ASSERT_EQ(frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(frames[i], std::to_string(i));
    }
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsWaterAnimationParamsFromGeneratedHeader)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/generated_anim_code.h", callback_func, "General", true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find water animation (should be keyed as "water" in snake_case)
    ASSERT_TRUE(anims.contains("water")) << "Should contain 'water' animation";
    const auto &water = anims.at("water");

    // Verify tile_offset from TILE_OFFSET_4BPP(432)
    EXPECT_EQ(water.tile_offset(), 432u);

    // Verify tile_count from 30 * TILE_SIZE_4BPP
    EXPECT_EQ(water.tile_count(), 30u);

    // Verify frame_factor and frame_offset from timer % 16 == 1
    EXPECT_EQ(water.frame_factor(), 16u);
    EXPECT_EQ(water.frame_offset(), 1u);

    // Verify frames from pointer array
    const auto &frames = water.frames();
    ASSERT_EQ(frames.size(), 8u);
    EXPECT_EQ(frames[0], "0");
    EXPECT_EQ(frames[1], "1");
    EXPECT_EQ(frames[2], "2");
    EXPECT_EQ(frames[3], "3");
    EXPECT_EQ(frames[4], "4");
    EXPECT_EQ(frames[5], "5");
    EXPECT_EQ(frames[6], "6");
    EXPECT_EQ(frames[7], "7");
}

TEST_F(AnimCodeParserTest, ParseFromCallbackReturnsErrorForNonExistentFile)
{
    auto result = parser_.parse_from_callback("nonexistent_file.h", "InitTilesetAnim_General", "General", false);

    EXPECT_FALSE(result.has_value()) << "Should return error for non-existent file";
}

// =============================================================================
// Vanilla Format Tests
// =============================================================================

TEST_F(AnimCodeParserTest, ParseFromCallbackDiscoversAnimationsInVanillaFile)
{
    // The callback for vanilla tilesets: InitTilesetAnim_General
    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/tileset_anims.c", "InitTilesetAnim_General", "General", false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    // Should discover at least Flower and Water for General tileset
    EXPECT_GE(anims.size(), 2u);
    EXPECT_TRUE(anims.contains("flower"));
    EXPECT_TRUE(anims.contains("water"));
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsFlowerAnimationParamsFromVanilla)
{
    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/tileset_anims.c", "InitTilesetAnim_General", "General", false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains("flower")) << "Should contain 'flower' animation";
    const auto &flower = anims.at("flower");

    // Verify tile_offset from TILE_OFFSET_4BPP(508)
    EXPECT_EQ(flower.tile_offset(), 508u);

    // Verify tile_count from 4 * TILE_SIZE_4BPP
    EXPECT_EQ(flower.tile_count(), 4u);

    // Verify frame_factor and frame_offset from timer % 16 == 0
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 0u);

    // Verify frames from pointer array
    const auto &frames = flower.frames();
    ASSERT_EQ(frames.size(), 4u);
    EXPECT_EQ(frames[0], "0");
    EXPECT_EQ(frames[1], "1");
    EXPECT_EQ(frames[2], "0");
    EXPECT_EQ(frames[3], "2");
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsLandWaterEdgeAnimationParamsFromVanilla)
{
    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/tileset_anims.c", "InitTilesetAnim_General", "General", false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains("land_water_edge")) << "Should contain 'land_water_edge' animation";
    const auto &land_water_edge = anims.at("land_water_edge");

    // Verify tile_offset from TILE_OFFSET_4BPP(480)
    EXPECT_EQ(land_water_edge.tile_offset(), 480u);

    // Verify tile_count from 10 * TILE_SIZE_4BPP
    EXPECT_EQ(land_water_edge.tile_count(), 10u);

    // Verify frame_factor and frame_offset from timer % 16 == 4
    EXPECT_EQ(land_water_edge.frame_factor(), 16u);
    EXPECT_EQ(land_water_edge.frame_offset(), 4u);

    // Verify frames from pointer array (4 frames for vanilla land_water_edge)
    const auto &frames = land_water_edge.frames();
    ASSERT_EQ(frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(frames[i], std::to_string(i));
    }
}

TEST_F(AnimCodeParserTest, ParseFromCallbackExtractsWaterAnimationParamsFromVanilla)
{
    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/tileset_anims.c", "InitTilesetAnim_General", "General", false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains("water")) << "Should contain 'water' animation";
    const auto &water = anims.at("water");

    // Verify tile_offset from TILE_OFFSET_4BPP(432)
    EXPECT_EQ(water.tile_offset(), 432u);

    // Verify tile_count from 30 * TILE_SIZE_4BPP
    EXPECT_EQ(water.tile_count(), 30u);

    // Verify frame_factor and frame_offset from timer % 16 == 1
    EXPECT_EQ(water.frame_factor(), 16u);
    EXPECT_EQ(water.frame_offset(), 1u);

    // Verify frames from pointer array (8 frames for vanilla water)
    const auto &frames = water.frames();
    ASSERT_EQ(frames.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(frames[i], std::to_string(i));
    }
}

TEST_F(AnimCodeParserTest, ParseFromCallbackReturnsEmptyForNonExistentCallback)
{
    auto result = parser_.parse_from_callback(
        "Resources/Tests/integration/anim/tileset_anims.c", "InitTilesetAnim_NonExistent", "NonExistent", false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed even with no matches";
    EXPECT_TRUE(result.value().empty()) << "Should return empty map for unknown tileset callback";
}

} // namespace porytiles2
