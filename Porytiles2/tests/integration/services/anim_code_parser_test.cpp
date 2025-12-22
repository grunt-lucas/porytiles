#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <gtest/gtest.h>

#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {

class AnimCodeParserTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    AnimCodeParser parser_{&formatter_};
};

// =============================================================================
// Generated Header Format Tests
// =============================================================================

TEST_F(AnimCodeParserTest, ParseGeneratedHeaderReturnsCorrectAnimationCount)
{
    auto result = parser_.parse_generated_header("Resources/Tests/integration/c_parser/generated_anim_code.h");

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();
    EXPECT_EQ(anims.size(), 2u) << "Should find 2 animations (flower, water)";
}

TEST_F(AnimCodeParserTest, ParseGeneratedHeaderExtractsFlowerAnimationParams)
{
    auto result = parser_.parse_generated_header("Resources/Tests/integration/c_parser/generated_anim_code.h");

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation (should be keyed as "flower" in snake_case)
    ASSERT_TRUE(anims.contains("flower")) << "Should contain 'flower' animation";
    const auto &flower = anims.at("flower");

    // Verify tile_offset from TILE_OFFSET_4BPP(12)
    EXPECT_EQ(flower.tile_offset(), 12u);

    // Verify tile_count from 4 * TILE_SIZE_4BPP
    EXPECT_EQ(flower.tile_count(), 4u);

    // Verify frame_factor and frame_offset from timer % 16 == 0
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 0u);

    // Verify frames from pointer array [Frame0, Frame1, Frame0, Frame2]
    const auto &frames = flower.frames();
    ASSERT_EQ(frames.size(), 4u);
    EXPECT_EQ(frames[0], 0u);
    EXPECT_EQ(frames[1], 1u);
    EXPECT_EQ(frames[2], 0u);
    EXPECT_EQ(frames[3], 2u);
}

TEST_F(AnimCodeParserTest, ParseGeneratedHeaderExtractsWaterAnimationParams)
{
    auto result = parser_.parse_generated_header("Resources/Tests/integration/c_parser/generated_anim_code.h");

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

    // Verify frames from pointer array [Frame0, Frame1, Frame2, Frame3, Frame4]
    const auto &frames = water.frames();
    ASSERT_EQ(frames.size(), 5u);
    EXPECT_EQ(frames[0], 0u);
    EXPECT_EQ(frames[1], 1u);
    EXPECT_EQ(frames[2], 2u);
    EXPECT_EQ(frames[3], 3u);
    EXPECT_EQ(frames[4], 4u);
}

TEST_F(AnimCodeParserTest, ParseGeneratedHeaderReturnsErrorForNonExistentFile)
{
    auto result = parser_.parse_generated_header("nonexistent_file.h");

    EXPECT_FALSE(result.has_value()) << "Should return error for non-existent file";
}

// =============================================================================
// Vanilla Format Tests
// =============================================================================

TEST_F(AnimCodeParserTest, ParseVanillaAnimsReturnsCorrectAnimationCount)
{
    auto result = parser_.parse_vanilla_anims("Resources/Tests/integration/c_parser/tileset_anims.c", "general");

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    // Should find at least Flower and Water for General tileset
    EXPECT_GE(anims.size(), 2u);
}

TEST_F(AnimCodeParserTest, ParseVanillaAnimsExtractsFlowerAnimationParams)
{
    auto result = parser_.parse_vanilla_anims("Resources/Tests/integration/c_parser/tileset_anims.c", "general");

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
    EXPECT_EQ(frames[0], 0u);
    EXPECT_EQ(frames[1], 1u);
    EXPECT_EQ(frames[2], 0u);
    EXPECT_EQ(frames[3], 2u);
}

TEST_F(AnimCodeParserTest, ParseVanillaAnimsExtractsWaterAnimationParams)
{
    auto result = parser_.parse_vanilla_anims("Resources/Tests/integration/c_parser/tileset_anims.c", "general");

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
        EXPECT_EQ(frames[i], i);
    }
}

TEST_F(AnimCodeParserTest, ParseVanillaAnimsExtractsLandWaterEdgeAnimationParams)
{
    auto result = parser_.parse_vanilla_anims("Resources/Tests/integration/c_parser/tileset_anims.c", "general");

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains("land_water_edge")) << "Should contain 'water' animation";
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
        EXPECT_EQ(frames[i], i);
    }
}

TEST_F(AnimCodeParserTest, ParseVanillaAnimsReturnsErrorForNonExistentFile)
{
    auto result = parser_.parse_vanilla_anims("nonexistent_file.c", "general");

    EXPECT_FALSE(result.has_value()) << "Should return error for non-existent file";
}

TEST_F(AnimCodeParserTest, ParseVanillaAnimsReturnsEmptyForUnknownTileset)
{
    auto result = parser_.parse_vanilla_anims("Resources/Tests/integration/c_parser/tileset_anims.c", "non_existent");

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed even with no matches";
    EXPECT_TRUE(result.value().empty()) << "Should return empty map for unknown tileset";
}

} // namespace porytiles2
