#include "porytiles/domain/algorithms/palette_matchers.hpp"

#include <gtest/gtest.h>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;

// Note: Rgba32 only supports is_transparent(extrinsic), not parameterless is_transparent()
// Therefore, all tests use the extrinsic transparency overload with rgba_magenta as the transparency color

TEST(PaletteMatchersTest, CompleteCoverage)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(1, 0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 3);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_TRUE(result.covered_colors.contains(rgba_green));
    EXPECT_TRUE(result.covered_colors.contains(rgba_blue));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, PartialCoverage)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(1, 0, rgba_yellow);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 2);
    EXPECT_TRUE(result.missing_colors.contains(rgba_blue));
    EXPECT_TRUE(result.missing_colors.contains(rgba_yellow));
    EXPECT_EQ(result.covered_colors.size(), 2);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_TRUE(result.covered_colors.contains(rgba_green));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 2);
    EXPECT_TRUE(
        std::find(result.uncovered_pixel_indices.begin(), result.uncovered_pixel_indices.end(), 2) !=
        result.uncovered_pixel_indices.end());
    EXPECT_TRUE(
        std::find(result.uncovered_pixel_indices.begin(), result.uncovered_pixel_indices.end(), 8) !=
        result.uncovered_pixel_indices.end());
}

TEST(PaletteMatchersTest, NoCoverage)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_blue);
    palette.add(rgba_yellow);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 2);
    EXPECT_TRUE(result.missing_colors.contains(rgba_red));
    EXPECT_TRUE(result.missing_colors.contains(rgba_green));
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 2);
}

TEST(PaletteMatchersTest, AllTransparentTile_IntrinsicTransparency)
{
    PixelTile<Rgba32> tile{};

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, AllTransparentTile_ExtrinsicTransparency)
{
    PixelTile<Rgba32> tile{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, rgba_magenta);
    }

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, MixedIntrinsicAndExtrinsicTransparency)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, Rgba32{});
    tile.set(0, 2, rgba_magenta);
    tile.set(1, 0, Rgba32{0, 0, 0, 0});
    tile.set(1, 1, rgba_green);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 2);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_TRUE(result.covered_colors.contains(rgba_green));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, UncoveredPixelIndicesCorrect)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, rgba_red);
    tile.set(5, rgba_green);
    tile.set(10, rgba_blue);
    tile.set(20, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 2);
    EXPECT_TRUE(
        std::find(result.uncovered_pixel_indices.begin(), result.uncovered_pixel_indices.end(), 5) !=
        result.uncovered_pixel_indices.end());
    EXPECT_TRUE(
        std::find(result.uncovered_pixel_indices.begin(), result.uncovered_pixel_indices.end(), 10) !=
        result.uncovered_pixel_indices.end());
}

TEST(PaletteMatchersTest, EmptyPalette_Panics)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    Palette<Rgba32> palette{};

    EXPECT_DEATH({ std::ignore = match_tile_to_palette(tile, palette, rgba_magenta); }, "palette is empty");
}

TEST(PaletteMatchersTest, DuplicateColorsInTile)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_red);
    tile.set(0, 2, rgba_red);
    tile.set(1, 0, rgba_green);
    tile.set(1, 1, rgba_green);
    tile.set(2, 0, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 3);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, SingleNonTransparentPixel)
{
    PixelTile<Rgba32> tile{};
    tile.set(3, 4, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 1);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, SingleUncoveredPixel)
{
    PixelTile<Rgba32> tile{};
    tile.set(59, rgba_yellow);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 1);
    EXPECT_TRUE(result.missing_colors.contains(rgba_yellow));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 1);
    EXPECT_EQ(result.uncovered_pixel_indices[0], 59);
}

TEST(PaletteMatchersTest, FullyPopulatedPalette)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Slot 0
    palette.add(rgba_red);     // Slot 1
    palette.add(rgba_green);   // Slot 2
    palette.add(rgba_blue);    // Slot 3
    // Fill remaining slots
    for (std::size_t i = 4; i < palette::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 10),
                static_cast<std::uint8_t>(i * 15),
                static_cast<std::uint8_t>(i * 20)});
    }

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 3);
}

TEST(PaletteMatchersTest, PaletteIndexField)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_EQ(result.palette_index, 0);
}

TEST(PaletteMatchersTest, RepeatSlot0Color)
{
    // This simulates the case of a .pla blend color being used in the palette/tile itself
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_blue);
    tile.set(0, 2, rgba_green);
    tile.set(0, 3, rgba_yellow);

    // Slot 0 set to yellow, slot 4 also yellow
    Palette<Rgba32> palette{};
    palette.add(rgba_yellow);
    palette.add(rgba_red);
    palette.add(rgba_blue);
    palette.add(rgba_green);
    palette.add(rgba_yellow);

    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 4);
}

TEST(MatchOrBestTest, CompleteMatch_ReturnsAllCompleteMatches)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_red);
    palette1.add(rgba_green);
    palettes.push_back(palette1);

    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_red);
    palette2.add(rgba_green);
    palette2.add(rgba_blue);
    palettes.push_back(palette2);

    auto results = match_or_best(tile, palettes, rgba_magenta, 3);

    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].palette_index, 1);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[1].palette_index, 2);
    EXPECT_TRUE(results[1].is_covered);
}

TEST(MatchOrBestTest, NoCompleteMatch_ReturnsTopNSorted)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(0, 3, rgba_yellow);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_red);
    palette1.add(rgba_green);
    palettes.push_back(palette1);

    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_red);
    palette2.add(rgba_green);
    palette2.add(rgba_blue);
    palettes.push_back(palette2);

    auto results = match_or_best(tile, palettes, rgba_magenta, 2);

    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].palette_index, 2);
    EXPECT_EQ(results[0].missing_colors.size(), 1);
    EXPECT_EQ(results[1].palette_index, 1);
    EXPECT_EQ(results[1].missing_colors.size(), 2);
}

TEST(MatchOrBestTest, SortingByMissingColors)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_red);
    palette1.add(rgba_green);
    palettes.push_back(palette1);

    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_blue);
    palettes.push_back(palette2);

    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].palette_index, 1);
    EXPECT_EQ(results[0].missing_colors.size(), 1);
    EXPECT_EQ(results[1].palette_index, 0);
    EXPECT_EQ(results[1].missing_colors.size(), 2);
    EXPECT_EQ(results[2].palette_index, 2);
    EXPECT_EQ(results[2].missing_colors.size(), 2);
}

TEST(MatchOrBestTest, TopNLargerThanPalettesSize_ReturnsAll)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_green);
    palettes.push_back(palette1);

    auto results = match_or_best(tile, palettes, rgba_magenta, 5);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].palette_index, 0);
    EXPECT_TRUE(results[0].is_covered);
}

TEST(MatchOrBestTest, TopNEquals1_ReturnsOnlyBest)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_blue);
    palettes.push_back(palette1);

    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].palette_index, 0);
    EXPECT_EQ(results[0].missing_colors.size(), 1);
}

TEST(MatchOrBestTest, EmptyPalettes_Panics)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    EXPECT_DEATH({ std::ignore = match_or_best(tile, palettes, rgba_magenta, 1); }, "palettes container is empty");
}

TEST(MatchOrBestTest, TopNZero_Panics)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;
    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palettes.push_back(palette0);

    EXPECT_DEATH({ std::ignore = match_or_best(tile, palettes, rgba_magenta, 0); }, "top_n must be greater than 0");
}

TEST(MatchOrBestTest, AllPalettesEqualQuality_MaintainsOrder)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_green);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_blue);
    palettes.push_back(palette1);

    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_yellow);
    palettes.push_back(palette2);

    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].palette_index, 0);
    EXPECT_EQ(results[1].palette_index, 1);
    EXPECT_EQ(results[2].palette_index, 2);
}

TEST(MatchOrBestTest, PaletteIndexCorrectlySet)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    for (int i = 0; i < 5; ++i) {
        Palette<Rgba32> palette{};
        palette.add(rgba_magenta);
        palette.add(rgba_green);
        palettes.push_back(palette);
    }

    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    EXPECT_EQ(results.size(), 5);
    for (std::size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i].palette_index, i);
    }
}

TEST(MatchOrBestTest, ExtrinsicTransparencyHandledCorrectly)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_magenta);
    tile.set(0, 2, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palette0.add(rgba_green);
    palettes.push_back(palette0);

    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[0].covered_colors.size(), 2);
}

TEST(MatchOrBestTest, AllCompleteMatchesReturned)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palette0.add(rgba_green);
    palette0.add(rgba_blue);
    palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_red);
    palettes.push_back(palette1);

    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_blue);
    palettes.push_back(palette2);

    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].palette_index, 0);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[1].palette_index, 1);
    EXPECT_TRUE(results[1].is_covered);
}

TEST(MatchOrBestTest, InvariantCheck_FirstElementIndicatesAllElements)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    // Scenario 1: Complete matches
    std::vector<Palette<Rgba32>> complete_palettes;
    Palette<Rgba32> palette0{};
    palette0.add(rgba_magenta);
    palette0.add(rgba_red);
    palette0.add(rgba_green);
    complete_palettes.push_back(palette0);

    Palette<Rgba32> palette1{};
    palette1.add(rgba_magenta);
    palette1.add(rgba_red);
    palette1.add(rgba_green);
    palette1.add(rgba_blue);
    complete_palettes.push_back(palette1);

    auto complete_results = match_or_best(tile, complete_palettes, rgba_magenta, 1);

    EXPECT_TRUE(complete_results.at(0).is_covered);
    for (const auto &result : complete_results) {
        EXPECT_TRUE(result.is_covered);
    }

    // Scenario 2: Incomplete matches
    std::vector<Palette<Rgba32>> incomplete_palettes;
    Palette<Rgba32> palette2{};
    palette2.add(rgba_magenta);
    palette2.add(rgba_red);
    incomplete_palettes.push_back(palette2);

    Palette<Rgba32> palette3{};
    palette3.add(rgba_magenta);
    palette3.add(rgba_blue);
    incomplete_palettes.push_back(palette3);

    auto incomplete_results = match_or_best(tile, incomplete_palettes, rgba_magenta, 2);

    EXPECT_FALSE(incomplete_results.at(0).is_covered);
    for (const auto &result : incomplete_results) {
        EXPECT_FALSE(result.is_covered);
    }
}
