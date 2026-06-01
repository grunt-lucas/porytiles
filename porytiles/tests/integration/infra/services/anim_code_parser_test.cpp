#include "porytiles/infra/services/anim_code_parser.hpp"

#include <gtest/gtest.h>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {

class AnimCodeParserTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
    AnimCodeParser parser_{&formatter_, &diag_};
};

TEST_F(AnimCodeParserTest, DiscoversAnimsInPokeemeraldGeneralGeneratedHeader)
{
    // The callback for Porytiles-managed tilesets: InitTilesetAnim_PorytilesManaged_General
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_general_generated_anim_code.h",
        callback_func,
        DynamicCasedName{"General"},
        true);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    EXPECT_EQ(anims.size(), 5u);

    EXPECT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"land_water_edge"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"sand_water_edge"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"water"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"waterfall"}));
}

TEST_F(AnimCodeParserTest, ExtractsAnimParamsFromPokeemeraldGeneralGeneratedHeader)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_general_generated_anim_code.h",
        callback_func,
        DynamicCasedName{"General"},
        true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"})) << "Should contain 'flower' animation";
    const auto &flower = anims.at(DynamicCasedName{"flower"});
    EXPECT_EQ(flower.cased_name().canonical(), "flower");
    EXPECT_EQ(flower.tile_offset(), 508u);
    EXPECT_EQ(flower.tile_count(), 4u);
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 0u);
    const auto &flower_frames = flower.frame_order();
    ASSERT_EQ(flower_frames.size(), 4u);
    EXPECT_EQ(flower_frames[0], DynamicCasedName{"0"});
    EXPECT_EQ(flower_frames[1], DynamicCasedName{"1"});
    EXPECT_EQ(flower_frames[2], DynamicCasedName{"0"});
    EXPECT_EQ(flower_frames[3], DynamicCasedName{"2"});

    // Find land_water_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"land_water_edge"})) << "Should contain 'land_water_edge' animation";
    const auto &land_water_edge = anims.at(DynamicCasedName{"land_water_edge"});
    EXPECT_EQ(land_water_edge.tile_offset(), 480u);
    EXPECT_EQ(land_water_edge.tile_count(), 10u);
    EXPECT_EQ(land_water_edge.frame_factor(), 16u);
    EXPECT_EQ(land_water_edge.frame_offset(), 4u);
    const auto &land_water_edge_frames = land_water_edge.frame_order();
    ASSERT_EQ(land_water_edge_frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(land_water_edge_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find sand_water_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"sand_water_edge"})) << "Should contain 'sand_water_edge' animation";
    const auto &sand_water_edge = anims.at(DynamicCasedName{"sand_water_edge"});
    EXPECT_EQ(sand_water_edge.tile_offset(), 464u);
    EXPECT_EQ(sand_water_edge.tile_count(), 10u);
    EXPECT_EQ(sand_water_edge.frame_factor(), 16u);
    EXPECT_EQ(sand_water_edge.frame_offset(), 2u);
    const auto &sand_water_edge_frames = sand_water_edge.frame_order();
    ASSERT_EQ(sand_water_edge_frames.size(), 8u);
    EXPECT_EQ(sand_water_edge_frames[0], DynamicCasedName{"0"});
    EXPECT_EQ(sand_water_edge_frames[1], DynamicCasedName{"1"});
    EXPECT_EQ(sand_water_edge_frames[2], DynamicCasedName{"2"});
    EXPECT_EQ(sand_water_edge_frames[3], DynamicCasedName{"3"});
    EXPECT_EQ(sand_water_edge_frames[4], DynamicCasedName{"4"});
    EXPECT_EQ(sand_water_edge_frames[5], DynamicCasedName{"5"});
    EXPECT_EQ(sand_water_edge_frames[6], DynamicCasedName{"6"});
    EXPECT_EQ(sand_water_edge_frames[7], DynamicCasedName{"0"});

    // Find water animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"water"})) << "Should contain 'water' animation";
    const auto &water = anims.at(DynamicCasedName{"water"});
    EXPECT_EQ(water.tile_offset(), 432u);
    EXPECT_EQ(water.tile_count(), 30u);
    EXPECT_EQ(water.frame_factor(), 16u);
    EXPECT_EQ(water.frame_offset(), 1u);
    const auto &water_frames = water.frame_order();
    ASSERT_EQ(water_frames.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(water_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find waterfall animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"waterfall"})) << "Should contain 'waterfall' animation";
    const auto &waterfall = anims.at(DynamicCasedName{"waterfall"});
    EXPECT_EQ(waterfall.tile_offset(), 496u);
    EXPECT_EQ(waterfall.tile_count(), 6u);
    EXPECT_EQ(waterfall.frame_factor(), 16u);
    EXPECT_EQ(waterfall.frame_offset(), 3u);
    const auto &waterfall_frames = waterfall.frame_order();
    ASSERT_EQ(waterfall_frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(waterfall_frames[i], DynamicCasedName{std::to_string(i)});
    }
}

TEST_F(AnimCodeParserTest, DiscoversAnimsInCustom1GeneratedHeader)
{
    // The callback for Porytiles-managed tilesets: InitTilesetAnim_PorytilesManaged_General
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "Custom1";

    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/custom1_generated_anim_code.h",
        callback_func,
        DynamicCasedName{"Custom1"},
        true);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    EXPECT_EQ(anims.size(), 2u);

    EXPECT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"tv_turned_on"}));
}

TEST_F(AnimCodeParserTest, ExtractsAnimParamsFromCustom1GeneratedHeader)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "Custom1";

    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/custom1_generated_anim_code.h",
        callback_func,
        DynamicCasedName{"Custom1"},
        true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"})) << "Should contain 'flower' animation";
    const auto &flower = anims.at(DynamicCasedName{"flower"});
    EXPECT_EQ(flower.cased_name().canonical(), "flower");
    EXPECT_EQ(flower.tile_offset(), 22);
    EXPECT_EQ(flower.tile_count(), 4u);
    EXPECT_EQ(flower.frame_factor(), 8u);
    EXPECT_EQ(flower.frame_offset(), 0u);
    const auto &flower_frames = flower.frame_order();
    ASSERT_EQ(flower_frames.size(), 4u);
    EXPECT_EQ(flower_frames[0], DynamicCasedName{"FooBar"});
    EXPECT_EQ(flower_frames[1], DynamicCasedName{"BazBat"});
    EXPECT_EQ(flower_frames[2], DynamicCasedName{"CatMat"});
    EXPECT_EQ(flower_frames[3], DynamicCasedName{"BazBat"});

    // Find tv_turned_on animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"tv_turned_on"})) << "Should contain 'flower' animation";
    const auto &tv_turned_on = anims.at(DynamicCasedName{"tv_turned_on"});
    EXPECT_EQ(tv_turned_on.cased_name().canonical(), "tvturnedon");
    EXPECT_EQ(tv_turned_on.tile_offset(), 351);
    EXPECT_EQ(tv_turned_on.tile_count(), 8u);
    EXPECT_EQ(tv_turned_on.frame_factor(), 16u);
    EXPECT_EQ(tv_turned_on.frame_offset(), 1u);
    const auto &tv_turned_on_frames = tv_turned_on.frame_order();
    ASSERT_EQ(tv_turned_on_frames.size(), 2u);
    EXPECT_EQ(tv_turned_on_frames[0], DynamicCasedName{"Zero"});
    EXPECT_EQ(tv_turned_on_frames[1], DynamicCasedName{"1"});
}

TEST_F(AnimCodeParserTest, NonExistentFile)
{
    auto result = parser_.parse_from_callback(
        "nonexistent_file.h", "InitTilesetAnim_General", DynamicCasedName{"General"}, false);

    EXPECT_FALSE(result.has_value()) << "Should return error for non-existent file";
}

TEST_F(AnimCodeParserTest, DiscoversAnimsInPokeemeraldTilesetAnimsC)
{
    // The callback for vanilla tilesets: InitTilesetAnim_General
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    EXPECT_EQ(anims.size(), 5u);

    EXPECT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"land_water_edge"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"sand_water_edge"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"water"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"waterfall"}));
}

TEST_F(AnimCodeParserTest, ExtractsAnimParamsInPokeemeraldTilesetAnimsC)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"})) << "Should contain 'flower' animation";
    const auto &flower = anims.at(DynamicCasedName{"flower"});
    EXPECT_EQ(flower.tile_offset(), 508u);
    EXPECT_EQ(flower.tile_count(), 4u);
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 0u);
    const auto &flower_frames = flower.frame_order();
    ASSERT_EQ(flower_frames.size(), 4u);
    EXPECT_EQ(flower_frames[0], DynamicCasedName{"0"});
    EXPECT_EQ(flower_frames[1], DynamicCasedName{"1"});
    EXPECT_EQ(flower_frames[2], DynamicCasedName{"0"});
    EXPECT_EQ(flower_frames[3], DynamicCasedName{"2"});

    // Find land_water_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"land_water_edge"})) << "Should contain 'land_water_edge' animation";
    const auto &land_water_edge = anims.at(DynamicCasedName{"land_water_edge"});
    EXPECT_EQ(land_water_edge.tile_offset(), 480u);
    EXPECT_EQ(land_water_edge.tile_count(), 10u);
    EXPECT_EQ(land_water_edge.frame_factor(), 16u);
    EXPECT_EQ(land_water_edge.frame_offset(), 4u);
    const auto &land_water_edge_frames = land_water_edge.frame_order();
    ASSERT_EQ(land_water_edge_frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(land_water_edge_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find sand_water_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"sand_water_edge"})) << "Should contain 'sand_water_edge' animation";
    const auto &sand_water_edge = anims.at(DynamicCasedName{"sand_water_edge"});
    EXPECT_EQ(sand_water_edge.tile_offset(), 464u);
    EXPECT_EQ(sand_water_edge.tile_count(), 10u);
    EXPECT_EQ(sand_water_edge.frame_factor(), 16u);
    EXPECT_EQ(sand_water_edge.frame_offset(), 2u);
    const auto &sand_water_edge_frames = sand_water_edge.frame_order();
    ASSERT_EQ(sand_water_edge_frames.size(), 8u);
    EXPECT_EQ(sand_water_edge_frames[0], DynamicCasedName{"0"});
    EXPECT_EQ(sand_water_edge_frames[1], DynamicCasedName{"1"});
    EXPECT_EQ(sand_water_edge_frames[2], DynamicCasedName{"2"});
    EXPECT_EQ(sand_water_edge_frames[3], DynamicCasedName{"3"});
    EXPECT_EQ(sand_water_edge_frames[4], DynamicCasedName{"4"});
    EXPECT_EQ(sand_water_edge_frames[5], DynamicCasedName{"5"});
    EXPECT_EQ(sand_water_edge_frames[6], DynamicCasedName{"6"});
    EXPECT_EQ(sand_water_edge_frames[7], DynamicCasedName{"0"});

    // Find water animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"water"})) << "Should contain 'water' animation";
    const auto &water = anims.at(DynamicCasedName{"water"});
    EXPECT_EQ(water.tile_offset(), 432u);
    EXPECT_EQ(water.tile_count(), 30u);
    EXPECT_EQ(water.frame_factor(), 16u);
    EXPECT_EQ(water.frame_offset(), 1u);
    const auto &water_frames = water.frame_order();
    ASSERT_EQ(water_frames.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(water_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find waterfall animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"waterfall"})) << "Should contain 'waterfall' animation";
    const auto &waterfall = anims.at(DynamicCasedName{"waterfall"});
    EXPECT_EQ(waterfall.tile_offset(), 496u);
    EXPECT_EQ(waterfall.tile_count(), 6u);
    EXPECT_EQ(waterfall.frame_factor(), 16u);
    EXPECT_EQ(waterfall.frame_offset(), 3u);
    const auto &waterfall_frames = waterfall.frame_order();
    ASSERT_EQ(waterfall_frames.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(waterfall_frames[i], DynamicCasedName{std::to_string(i)});
    }
}

TEST_F(AnimCodeParserTest, NonExistentCallback)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_tileset_anims.c",
        "InitTilesetAnim_NonExistent",
        DynamicCasedName{"NonExistent"},
        false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed even with no matches";
    EXPECT_TRUE(result.value().empty()) << "Should return empty map for unknown tileset callback";
}

TEST_F(AnimCodeParserTest, DiscoversAnimsInPokefireredTilesetAnimsC)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokefirered_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed";
    const auto &anims = result.value();

    EXPECT_EQ(anims.size(), 3u)
        << "Should discover 3 animations (sand_waters_edge, water_current_landwatersedge, flower)";
    EXPECT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"sand_waters_edge"}));
    EXPECT_TRUE(anims.contains(DynamicCasedName{"water_current_land_waters_edge"}));
}

TEST_F(AnimCodeParserTest, ExtractsAnimParamsInPokefireredTilesetAnimsC)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokefirered_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Find flower animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"})) << "Should contain 'flower' animation";
    const auto &flower = anims.at(DynamicCasedName{"flower"});
    EXPECT_EQ(flower.tile_offset(), 508u);
    EXPECT_EQ(flower.tile_count(), 4u);
    EXPECT_EQ(flower.frame_factor(), 16u);
    EXPECT_EQ(flower.frame_offset(), 2u);
    const auto &flower_frames = flower.frame_order();
    ASSERT_EQ(flower_frames.size(), 5u);
    for (std::size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(flower_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find sand_waters_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"sand_waters_edge"})) << "Should contain 'sand_waters_edge' animation";
    const auto &sand_waters_edge = anims.at(DynamicCasedName{"sand_waters_edge"});
    EXPECT_EQ(sand_waters_edge.tile_offset(), 464u);
    EXPECT_EQ(sand_waters_edge.tile_count(), 18u);
    EXPECT_EQ(sand_waters_edge.frame_factor(), 8u);
    EXPECT_EQ(sand_waters_edge.frame_offset(), 0u);
    const auto &sand_waters_edge_frames = sand_waters_edge.frame_order();
    ASSERT_EQ(sand_waters_edge_frames.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(sand_waters_edge_frames[i], DynamicCasedName{std::to_string(i)});
    }

    // Find water_current_land_waters_edge animation
    ASSERT_TRUE(anims.contains(DynamicCasedName{"water_current_land_waters_edge"}))
        << "Should contain 'water_current_land_waters_edge' animation";
    const auto &water_current_land_waters_edge = anims.at(DynamicCasedName{"water_current_land_waters_edge"});
    EXPECT_EQ(water_current_land_waters_edge.tile_offset(), 416u);
    EXPECT_EQ(water_current_land_waters_edge.tile_count(), 48u);
    EXPECT_EQ(water_current_land_waters_edge.frame_factor(), 16u);
    EXPECT_EQ(water_current_land_waters_edge.frame_offset(), 1u);
    const auto &water_current_land_waters_edge_frames = water_current_land_waters_edge.frame_order();
    ASSERT_EQ(water_current_land_waters_edge_frames.size(), 8u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(water_current_land_waters_edge_frames[i], DynamicCasedName{std::to_string(i)});
    }
}

TEST_F(AnimCodeParserTest, PreservesCasedNameForSimpleNames)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Simple PascalCase names should losslessly round-trip via DynamicCasedName
    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_EQ(anims.at(DynamicCasedName{"flower"}).cased_name().to_c_identifier(), "Flower");
}

TEST_F(AnimCodeParserTest, PreservesCasedNameForCompoundNames)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokefirered_tileset_anims.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    // Compound PascalCase names must be preserved for frame variable lookup via to_c_identifier()
    ASSERT_TRUE(anims.contains(DynamicCasedName{"sand_waters_edge"}));
    EXPECT_EQ(anims.at(DynamicCasedName{"sand_waters_edge"}).cased_name().to_c_identifier(), "SandWatersEdge");

    ASSERT_TRUE(anims.contains(DynamicCasedName{"water_current_land_waters_edge"}));
    EXPECT_EQ(
        anims.at(DynamicCasedName{"water_current_land_waters_edge"}).cased_name().to_c_identifier(),
        "Water_Current_LandWatersEdge");
}

TEST_F(AnimCodeParserTest, PreservesCasedNameForPorytilesManaged)
{
    const std::string callback_func = "InitTilesetAnim_" + anim::porytiles_managed_prefix + "General";

    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/pokeemerald_general_generated_anim_code.h",
        callback_func,
        DynamicCasedName{"General"},
        true);

    ASSERT_TRUE(result.has_value());
    const auto &anims = result.value();

    ASSERT_TRUE(anims.contains(DynamicCasedName{"flower"}));
    EXPECT_EQ(anims.at(DynamicCasedName{"flower"}).cased_name().to_c_identifier(), "Flower");

    ASSERT_TRUE(anims.contains(DynamicCasedName{"land_water_edge"}));
    EXPECT_EQ(anims.at(DynamicCasedName{"land_water_edge"}).cased_name().to_c_identifier(), "LandWaterEdge");
}

TEST_F(AnimCodeParserTest, FrlgVariantDoesNotCauseAmbiguousMatch)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/frlg_variant_prefix_match.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);

    ASSERT_TRUE(result.has_value()) << "Parsing should succeed when FRLG variant exists";
    const auto &anims = result.value();

    EXPECT_EQ(anims.size(), 1u);
    EXPECT_TRUE(anims.contains(DynamicCasedName{"flower"}));
}

// ── Error condition tests ─────────────────────────────────────────────────────

TEST_F(AnimCodeParserTest, MultipleCallbacks)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step1_multiple_callbacks.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when multiple callback functions match";
}

TEST_F(AnimCodeParserTest, NoCallbackAssignment)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step1_no_callback_assignment.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when callback has no driver assignment";
}

TEST_F(AnimCodeParserTest, DriverFuncNotFound)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step2_driver_func_not_found.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when driver function is not defined";
}

TEST_F(AnimCodeParserTest, NoTimerConditions)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step2_no_timer_conditions.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when driver has no timer conditions";
}

TEST_F(AnimCodeParserTest, QueueFuncNotFound)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_queue_func_not_found.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when queue function is not defined";
}

TEST_F(AnimCodeParserTest, NoAppendCalls)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_no_append_calls.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value())
        << "Should return error when queue function has no AppendTilesetAnimToBuffer calls";
}

TEST_F(AnimCodeParserTest, FewerThan3Args)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_fewer_than_3_args.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when AppendTilesetAnimToBuffer has fewer than 3 arguments";
}

TEST_F(AnimCodeParserTest, NoIdentifierInFirstArg)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_no_identifier_in_first_arg.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when first argument has no identifier";
}

TEST_F(AnimCodeParserTest, BadAnimNamePrefix)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_bad_anim_name_prefix.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when array name has wrong prefix";
}

TEST_F(AnimCodeParserTest, MissingTileOffset)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_missing_tile_offset.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when TILE_OFFSET_4BPP pattern is missing";
}

TEST_F(AnimCodeParserTest, MissingTileSize)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_missing_tile_size.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when TILE_SIZE_4BPP pattern is missing";
}

TEST_F(AnimCodeParserTest, VDestsMultiAppend)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step4_vdests_multi_append.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value())
        << "Should return error when queue function has multiple AppendTilesetAnimToBuffer calls";
}

TEST_F(AnimCodeParserTest, MissingFrameArray)
{
    auto result = parser_.parse_from_callback(
        "resources/tests/integration/shared/anim/errors/error_step6_missing_frame_array.c",
        "InitTilesetAnim_General",
        DynamicCasedName{"General"},
        false);
    EXPECT_FALSE(result.has_value()) << "Should return error when frame array is not found for animation";
}

} // namespace porytiles
