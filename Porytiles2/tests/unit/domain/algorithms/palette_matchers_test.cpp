#include "porytiles2/domain/algorithms/palette_matchers.hpp"

#include <gtest/gtest.h>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

// ===========================
// Extrinsic Transparency Tests (Rgba32)
// ===========================
// Note: Rgba32 only supports is_transparent(extrinsic), not parameterless is_transparent()
// Therefore, all tests use the extrinsic transparency overload with rgba_magenta as the transparency color

TEST(PaletteMatchersTest, CompleteCoverage)
{
    // Arrange: Create a tile with colors all in the palette
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(1, 0, rgba_red); // Duplicate color

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: All colors should be covered
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
    // Arrange: Create a tile with colors, but palette only has some of them
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(1, 0, rgba_yellow);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Only some colors should be covered
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
    // Arrange: Create a tile with colors not in the palette
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_blue);
    palette.add(rgba_yellow);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: No colors should be covered
    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 2);
    EXPECT_TRUE(result.missing_colors.contains(rgba_red));
    EXPECT_TRUE(result.missing_colors.contains(rgba_green));
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 2);
}

TEST(PaletteMatchersTest, AllTransparentTile_IntrinsicTransparency)
{
    // Arrange: Create a tile with all intrinsically transparent pixels (alpha=0)
    PixelTile<Rgba32> tile{}; // Default constructor creates all transparent pixels

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Should report complete coverage since no non-transparent colors to match
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, AllTransparentTile_ExtrinsicTransparency)
{
    // Arrange: Create a tile with all extrinsically transparent pixels
    PixelTile<Rgba32> tile{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, rgba_magenta); // All pixels are extrinsic transparency
    }

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Should report complete coverage since all pixels are transparent
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, MixedIntrinsicAndExtrinsicTransparency)
{
    // Arrange: Create a tile with both intrinsic (alpha=0) and extrinsic transparent pixels
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, Rgba32{});           // Intrinsic transparency (alpha=0)
    tile.set(0, 2, rgba_magenta);       // Extrinsic transparency
    tile.set(1, 0, Rgba32{0, 0, 0, 0}); // Explicitly intrinsic transparency
    tile.set(1, 1, rgba_green);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Both types of transparency should be ignored
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 2);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_TRUE(result.covered_colors.contains(rgba_green));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, UncoveredPixelIndicesCorrect)
{
    // Arrange: Create a tile with specific pixel positions having uncovered colors
    PixelTile<Rgba32> tile{};
    tile.set(0, rgba_red);   // Linear index 0
    tile.set(5, rgba_green); // Linear index 5
    tile.set(10, rgba_blue); // Linear index 10
    tile.set(20, rgba_red);  // Linear index 20, duplicate color

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Verify uncovered pixel indices are correct
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
    // Arrange: Create a tile with colors and an empty palette
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    Palette<Rgba32> palette{}; // Empty palette

    // Act & Assert: Should panic when palette is empty
    EXPECT_DEATH({ std::ignore = match_tile_to_palette(tile, palette, rgba_magenta); }, "palette is empty");
}

TEST(PaletteMatchersTest, DuplicateColorsInTile)
{
    // Arrange: Create a tile with many duplicate colors
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

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Duplicates should be deduplicated in the color sets
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 3);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, SingleNonTransparentPixel)
{
    // Arrange: Create a tile with only one non-transparent pixel
    PixelTile<Rgba32> tile{}; // All transparent by default
    tile.set(3, 4, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: The single pixel should be covered
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 1);
    EXPECT_TRUE(result.covered_colors.contains(rgba_red));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
}

TEST(PaletteMatchersTest, SingleUncoveredPixel)
{
    // Arrange: Create a tile with one pixel not in the palette
    PixelTile<Rgba32> tile{}; // All transparent by default
    tile.set(59, rgba_yellow);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: The single pixel should be uncovered
    EXPECT_FALSE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 1);
    EXPECT_TRUE(result.missing_colors.contains(rgba_yellow));
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 1);
    EXPECT_EQ(result.uncovered_pixel_indices[0], 59);
}

TEST(PaletteMatchersTest, FullyPopulatedPalette)
{
    // Arrange: Create a tile and a fully populated 16-color palette
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
    for (std::size_t i = 4; i < pal::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 10),
                static_cast<std::uint8_t>(i * 15),
                static_cast<std::uint8_t>(i * 20)});
    }

    // Act: Match the tile to the palette with extrinsic transparency
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: All tile colors should be covered
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.covered_colors.size(), 3);
}

TEST(PaletteMatchersTest, PaletteIndexField)
{
    // Arrange: Create a simple matching scenario
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Match the tile to the palette
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Verify the pal_index field is initialized (should be 0 by default)
    EXPECT_EQ(result.pal_index, 0);
}

TEST(PaletteMatchersTest, RepeatSlot0Color)
{
    // Arrange: Create a scenario where pal slot 0 color is repeated elsewhere in the pal
    // This simulates the case of a .pla blend color being used in the pal/tile itself
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

    // Act: Match the tile to the palette
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Verify all pixels are covered
    EXPECT_TRUE(result.is_covered);
    EXPECT_EQ(result.missing_colors.size(), 0);
    EXPECT_EQ(result.uncovered_pixel_indices.size(), 0);
    EXPECT_EQ(result.covered_colors.size(), 4);
}

// ===========================
// match_or_best Tests
// ===========================

TEST(MatchOrBestTest, CompleteMatch_ReturnsAllCompleteMatches)
{
    // Arrange: Create a tile and palettes where multiple palettes match completely
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    // Palette 0: doesn't have all colors (should not be returned)
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    // Palette 1: has all colors (complete match)
    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_red);
    pal1.add(rgba_green);
    palettes.push_back(pal1);

    // Palette 2: also has all colors (complete match)
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_red);
    pal2.add(rgba_green);
    pal2.add(rgba_blue);
    palettes.push_back(pal2);

    // Act: Find best match with top_n = 3
    auto results = match_or_best(tile, palettes, rgba_magenta, 3);

    // Assert: Should return all complete matches (palettes 1 and 2), ignoring top_n
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].pal_index, 1);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[1].pal_index, 2);
    EXPECT_TRUE(results[1].is_covered);
}

TEST(MatchOrBestTest, NoCompleteMatch_ReturnsTopNSorted)
{
    // Arrange: Create a tile and palettes with varying quality
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);
    tile.set(0, 3, rgba_yellow);

    std::vector<Palette<Rgba32>> palettes;

    // Palette 0: missing 3 colors (worst)
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    // Palette 1: missing 2 colors
    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_red);
    pal1.add(rgba_green);
    palettes.push_back(pal1);

    // Palette 2: missing 1 color (best)
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_red);
    pal2.add(rgba_green);
    pal2.add(rgba_blue);
    palettes.push_back(pal2);

    // Act: Find best match with top_n = 2
    auto results = match_or_best(tile, palettes, rgba_magenta, 2);

    // Assert: Should return top 2, sorted by quality
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].pal_index, 2); // Best match (1 missing color)
    EXPECT_EQ(results[0].missing_colors.size(), 1);
    EXPECT_EQ(results[1].pal_index, 1); // Second best (2 missing colors)
    EXPECT_EQ(results[1].missing_colors.size(), 2);
}

TEST(MatchOrBestTest, SortingByMissingColors)
{
    // Arrange: Create palettes with different numbers of missing colors
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);
    tile.set(0, 2, rgba_blue);

    std::vector<Palette<Rgba32>> palettes;

    // Palette 0: missing 2 colors
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    // Palette 1: missing 1 color (better)
    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_red);
    pal1.add(rgba_green);
    palettes.push_back(pal1);

    // Palette 2: missing 2 colors (same as palette 0)
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_blue);
    palettes.push_back(pal2);

    // Act: Find all best matches
    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    // Assert: Palette 1 should be first, then 0 and 2 (original order maintained for ties)
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].pal_index, 1);
    EXPECT_EQ(results[0].missing_colors.size(), 1);
    EXPECT_EQ(results[1].pal_index, 0);
    EXPECT_EQ(results[1].missing_colors.size(), 2);
    EXPECT_EQ(results[2].pal_index, 2);
    EXPECT_EQ(results[2].missing_colors.size(), 2);
}

TEST(MatchOrBestTest, TopNLargerThanPalettesSize_ReturnsAll)
{
    // Arrange: Create 2 palettes but ask for top 5
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_green);
    palettes.push_back(pal1);

    // Act: Ask for top 5 when only 2 palettes exist
    auto results = match_or_best(tile, palettes, rgba_magenta, 5);

    // Assert: Should return all 2 palettes (first one is complete match)
    EXPECT_EQ(results.size(), 1); // Returns only complete match
    EXPECT_EQ(results[0].pal_index, 0);
    EXPECT_TRUE(results[0].is_covered);
}

TEST(MatchOrBestTest, TopNEquals1_ReturnsOnlyBest)
{
    // Arrange: Create multiple palettes but ask for only top 1
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    // Palette 0: missing 1 color
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    // Palette 1: missing 0 colors (best, but not checked due to order)
    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_blue);
    palettes.push_back(pal1);

    // Act: Ask for top 1
    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    // Assert: Should return only the best match
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pal_index, 0);
    EXPECT_EQ(results[0].missing_colors.size(), 1);
}

TEST(MatchOrBestTest, EmptyPalettes_Panics)
{
    // Arrange: Create empty palettes vector
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes; // Empty

    // Act & Assert: Should panic
    EXPECT_DEATH({ std::ignore = match_or_best(tile, palettes, rgba_magenta, 1); }, "palettes container is empty");
}

TEST(MatchOrBestTest, TopNZero_Panics)
{
    // Arrange: Create valid palettes but top_n = 0
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    palettes.push_back(pal0);

    // Act & Assert: Should panic
    EXPECT_DEATH({ std::ignore = match_or_best(tile, palettes, rgba_magenta, 0); }, "top_n must be greater than 0");
}

TEST(MatchOrBestTest, AllPalettesEqualQuality_MaintainsOrder)
{
    // Arrange: Create palettes with same number of missing colors
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    // All palettes missing 1 color
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_green);
    palettes.push_back(pal0);

    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_blue);
    palettes.push_back(pal1);

    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_yellow);
    palettes.push_back(pal2);

    // Act: Get all results
    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    // Assert: Should maintain original order since quality is equal
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].pal_index, 0);
    EXPECT_EQ(results[1].pal_index, 1);
    EXPECT_EQ(results[2].pal_index, 2);
}

TEST(MatchOrBestTest, PalIndexCorrectlySet)
{
    // Arrange: Create palettes and verify pal_index tracking
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    for (int i = 0; i < 5; ++i) {
        Palette<Rgba32> pal{};
        pal.add(rgba_magenta);
        pal.add(rgba_green);
        palettes.push_back(pal);
    }

    // Act: Get all results
    auto results = match_or_best(tile, palettes, rgba_magenta, 10);

    // Assert: Each result should have correct pal_index
    EXPECT_EQ(results.size(), 5);
    for (std::size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i].pal_index, i);
    }
}

TEST(MatchOrBestTest, ExtrinsicTransparencyHandledCorrectly)
{
    // Arrange: Create tile with extrinsic transparent pixels
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_magenta); // Extrinsic transparency
    tile.set(0, 2, rgba_green);

    std::vector<Palette<Rgba32>> palettes;

    // Palette that covers both non-transparent colors
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    pal0.add(rgba_green);
    palettes.push_back(pal0);

    // Act: Match with extrinsic transparency
    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    // Assert: Should be complete match (magenta is transparent, not counted)
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[0].covered_colors.size(), 2); // red and green
}

TEST(MatchOrBestTest, AllCompleteMatchesReturned)
{
    // Arrange: Multiple complete matches
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    std::vector<Palette<Rgba32>> palettes;

    // Palette 0: complete match with extra colors
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    pal0.add(rgba_green);
    pal0.add(rgba_blue);
    palettes.push_back(pal0);

    // Palette 1: also complete match
    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_red);
    palettes.push_back(pal1);

    // Palette 2: incomplete match (should not be returned)
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_blue);
    palettes.push_back(pal2);

    // Act: Get best matches (top_n should be ignored when complete matches exist)
    auto results = match_or_best(tile, palettes, rgba_magenta, 1);

    // Assert: Should return ALL complete matches, ignoring top_n
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].pal_index, 0);
    EXPECT_TRUE(results[0].is_covered);
    EXPECT_EQ(results[1].pal_index, 1);
    EXPECT_TRUE(results[1].is_covered);
}

TEST(MatchOrBestTest, InvariantCheck_FirstElementIndicatesAllElements)
{
    // Arrange: Test both complete and incomplete match scenarios
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);
    tile.set(0, 1, rgba_green);

    // Scenario 1: Complete matches
    std::vector<Palette<Rgba32>> complete_palettes;
    Palette<Rgba32> pal0{};
    pal0.add(rgba_magenta);
    pal0.add(rgba_red);
    pal0.add(rgba_green);
    complete_palettes.push_back(pal0);

    Palette<Rgba32> pal1{};
    pal1.add(rgba_magenta);
    pal1.add(rgba_red);
    pal1.add(rgba_green);
    pal1.add(rgba_blue);
    complete_palettes.push_back(pal1);

    auto complete_results = match_or_best(tile, complete_palettes, rgba_magenta, 1);

    // Assert: .at(0).is_covered is true, and all results are covered
    EXPECT_TRUE(complete_results.at(0).is_covered);
    for (const auto &result : complete_results) {
        EXPECT_TRUE(result.is_covered);
    }

    // Scenario 2: Incomplete matches
    std::vector<Palette<Rgba32>> incomplete_palettes;
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    pal2.add(rgba_red);
    incomplete_palettes.push_back(pal2);

    Palette<Rgba32> pal3{};
    pal3.add(rgba_magenta);
    pal3.add(rgba_blue);
    incomplete_palettes.push_back(pal3);

    auto incomplete_results = match_or_best(tile, incomplete_palettes, rgba_magenta, 2);

    // Assert: .at(0).is_covered is false, and all results are not covered
    EXPECT_FALSE(incomplete_results.at(0).is_covered);
    for (const auto &result : incomplete_results) {
        EXPECT_FALSE(result.is_covered);
    }
}
