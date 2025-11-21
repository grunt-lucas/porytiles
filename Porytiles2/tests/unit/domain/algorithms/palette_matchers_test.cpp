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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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

TEST(PaletteMatchersTest, MismatchedExtrinsicInSlot0_Panics)
{
    // Arrange: Create a palette where slot 0 does not match extrinsic transparency
    PixelTile<Rgba32> tile{};
    tile.set(0, 0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_blue); // Slot 0 is blue, not magenta
    palette.add(rgba_red);

    // Act & Assert: Should panic when extrinsic doesn't match slot 0
    EXPECT_DEATH(
        { std::ignore = match_tile_to_palette(tile, palette, rgba_magenta); },
        "palette slot 0 did not match provided extrinsic transparency value");
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
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
    palette.add(rgba_magenta); // Slot 0 must be extrinsic transparency
    palette.add(rgba_red);

    // Act: Match the tile to the palette
    auto result = match_tile_to_palette(tile, palette, rgba_magenta);

    // Assert: Verify the pal_index field is initialized (should be 0 by default)
    EXPECT_EQ(result.pal_index, 0);
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
    EXPECT_DEATH({ std::ignore = match_or_best(tile, palettes, rgba_magenta, 1); }, "palettes vector is empty");
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

// ===========================
// index_tile_to_color_tile Tests
// ===========================

TEST(IndexTileToColorTileTest, BasicConversion)
{
    // Arrange: Create a simple indexed tile and palette
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0}); // Maps to rgba_magenta
    index_tile.set(0, 1, IndexPixel{1}); // Maps to rgba_red
    index_tile.set(0, 2, IndexPixel{2}); // Maps to rgba_green
    index_tile.set(1, 0, IndexPixel{3}); // Maps to rgba_blue

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify colors are correctly mapped
    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_green);
    EXPECT_EQ(color_tile.at(1, 0), rgba_blue);
}

TEST(IndexTileToColorTileTest, MultipleDifferentColors)
{
    // Arrange: Create a tile with various colors from palette
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, IndexPixel{1});  // Linear index 0
    index_tile.set(5, IndexPixel{2});  // Linear index 5
    index_tile.set(10, IndexPixel{3}); // Linear index 10
    index_tile.set(63, IndexPixel{4}); // Linear index 63 (last pixel)

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);
    palette.add(rgba_yellow);

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify specific positions have correct colors
    EXPECT_EQ(color_tile.at(0), rgba_red);
    EXPECT_EQ(color_tile.at(5), rgba_green);
    EXPECT_EQ(color_tile.at(10), rgba_blue);
    EXPECT_EQ(color_tile.at(63), rgba_yellow);
}

TEST(IndexTileToColorTileTest, DuplicateIndices)
{
    // Arrange: Create a tile where the same index is used multiple times
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{1}); // red
    index_tile.set(0, 1, IndexPixel{1}); // red
    index_tile.set(0, 2, IndexPixel{1}); // red
    index_tile.set(1, 0, IndexPixel{2}); // green
    index_tile.set(1, 1, IndexPixel{2}); // green
    index_tile.set(2, 0, IndexPixel{1}); // red

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify all duplicate indices map to the same color
    EXPECT_EQ(color_tile.at(0, 0), rgba_red);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_red);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
    EXPECT_EQ(color_tile.at(1, 1), rgba_green);
    EXPECT_EQ(color_tile.at(2, 0), rgba_red);
}

TEST(IndexTileToColorTileTest, CorrectColorMappingAllPixels)
{
    // Arrange: Create a tile with a pattern across all 64 pixels
    PixelTile<IndexPixel> index_tile{};
    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2
    palette.add(rgba_blue);    // Index 3

    // Fill tile with repeating pattern: 0, 1, 2, 3, 0, 1, 2, 3, ...
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        index_tile.set(i, IndexPixel{static_cast<unsigned int>(i % 4)});
    }

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify the pattern is correctly mapped
    const std::array<Rgba32, 4> expected_colors = {rgba_magenta, rgba_red, rgba_green, rgba_blue};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(color_tile.at(i), expected_colors[i % 4]);
    }
}

TEST(IndexTileToColorTileTest, EmptyPalette_Panics)
{
    // Arrange: Create an indexed tile and an empty palette
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});

    Palette<Rgba32> palette{}; // Empty palette

    // Act & Assert: Should panic when palette is empty
    EXPECT_DEATH({ std::ignore = color_tile_from_index_tile(index_tile, palette); }, "palette is empty");
}

TEST(IndexTileToColorTileTest, OutOfBoundsIndex_Panics)
{
    // Arrange: Create a tile with an index that exceeds palette size
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});
    index_tile.set(0, 1, IndexPixel{5}); // Out of bounds!

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    // Palette size is 3, but we're trying to access index 5

    // Act & Assert: Should panic when index is out of bounds
    EXPECT_DEATH({ std::ignore = color_tile_from_index_tile(index_tile, palette); }, "index 5 out of palette bounds");
}

TEST(IndexTileToColorTileTest, FullyPopulatedPalette)
{
    // Arrange: Create a palette with all 16 color slots filled
    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2
    palette.add(rgba_blue);    // Index 3
    palette.add(rgba_yellow);  // Index 4

    // Fill remaining slots with generated colors
    for (std::size_t i = 5; i < pal::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 10),
                static_cast<std::uint8_t>(i * 15),
                static_cast<std::uint8_t>(i * 20)});
    }

    // Create a tile using all palette indices
    PixelTile<IndexPixel> index_tile{};
    for (std::size_t i = 0; i < pal::max_size; ++i) {
        index_tile.set(i, IndexPixel{static_cast<unsigned int>(i)});
    }

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify all 16 colors are correctly mapped
    EXPECT_EQ(color_tile.at(0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1), rgba_red);
    EXPECT_EQ(color_tile.at(2), rgba_green);
    EXPECT_EQ(color_tile.at(3), rgba_blue);
    EXPECT_EQ(color_tile.at(4), rgba_yellow);

    // Check a generated color
    EXPECT_EQ(color_tile.at(5), (Rgba32{50, 75, 100}));
}

TEST(IndexTileToColorTileTest, TransparencyAtIndex0)
{
    // Arrange: Create a palette with transparency at index 0 (standard pattern)
    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0 - extrinsic transparency
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2

    // Create a tile with some pixels using index 0 (transparent)
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0}); // Transparent pixel
    index_tile.set(0, 1, IndexPixel{1}); // Red pixel
    index_tile.set(0, 2, IndexPixel{0}); // Transparent pixel
    index_tile.set(1, 0, IndexPixel{2}); // Green pixel

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify transparency color is correctly mapped
    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_magenta);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
}

TEST(IndexTileToColorTileTest, SingleColorPalette)
{
    // Arrange: Create a palette with only one color
    Palette<Rgba32> palette{};
    palette.add(rgba_red);

    // Create a tile where all used pixels have index 0
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});
    index_tile.set(1, 1, IndexPixel{0});
    index_tile.set(2, 2, IndexPixel{0});

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: All pixels should map to the single palette color
    EXPECT_EQ(color_tile.at(0, 0), rgba_red);
    EXPECT_EQ(color_tile.at(1, 1), rgba_red);
    EXPECT_EQ(color_tile.at(2, 2), rgba_red);
}

TEST(IndexTileToColorTileTest, BoundaryIndexValues)
{
    // Arrange: Test with boundary index values (0 and max_size-1)
    Palette<Rgba32> palette{};
    for (std::size_t i = 0; i < pal::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 16),
                static_cast<std::uint8_t>(i * 16),
                static_cast<std::uint8_t>(i * 16)});
    }

    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, IndexPixel{0});                 // Minimum valid index
    index_tile.set(1, IndexPixel{pal::max_size - 1}); // Maximum valid index
    index_tile.set(2, IndexPixel{pal::max_size / 2}); // Middle index

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette);

    // Assert: Verify boundary values are handled correctly
    EXPECT_EQ(color_tile.at(0), (Rgba32{0, 0, 0}));
    EXPECT_EQ(color_tile.at(1), (Rgba32{240, 240, 240}));
    EXPECT_EQ(color_tile.at(2), (Rgba32{128, 128, 128}));
}

// ===========================
// index_tile_from_color_tile Tests (Extrinsic Transparency)
// ===========================

TEST(IndexTileFromColorTileTest, BasicConversion)
{
    // Arrange: Create a tile with colors that are all in the palette
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, 0, rgba_magenta); // Transparent, maps to index 0
    color_tile.set(0, 1, rgba_red);     // Maps to index 1
    color_tile.set(0, 2, rgba_green);   // Maps to index 2
    color_tile.set(1, 0, rgba_blue);    // Maps to index 3

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0 - extrinsic transparency
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2
    palette.add(rgba_blue);    // Index 3

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Verify the conversion was successful
    EXPECT_EQ(index_tile.at(0, 0).index(), 0);
    EXPECT_EQ(index_tile.at(0, 1).index(), 1);
    EXPECT_EQ(index_tile.at(0, 2).index(), 2);
    EXPECT_EQ(index_tile.at(1, 0).index(), 3);
}

TEST(IndexTileFromColorTileTest, AllColorsFound)
{
    // Arrange: Create a tile with colors all in the palette
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_red);
    color_tile.set(1, rgba_green);
    color_tile.set(2, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Verify correct indices
    EXPECT_EQ(index_tile.at(0).index(), 1);
    EXPECT_EQ(index_tile.at(1).index(), 2);
    EXPECT_EQ(index_tile.at(2).index(), 3);
}

TEST(IndexTileFromColorTileTest, ColorNotInPalette_Panics)
{
    // Arrange: Create a tile with a color not in the palette
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_blue); // NOT in palette

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act & Assert: Should panic when color is not found
    EXPECT_DEATH(
        { std::ignore = index_tile_from_color_tile(color_tile, palette, rgba_magenta); }, "color not found in palette");
}

TEST(IndexTileFromColorTileTest, AllTransparent_IntrinsicAndExtrinsic)
{
    // Arrange: Create a tile with all transparent pixels (mix of intrinsic and extrinsic)
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, Rgba32{});           // Intrinsic transparency (alpha=0)
    color_tile.set(1, rgba_magenta);       // Extrinsic transparency
    color_tile.set(2, Rgba32{0, 0, 0, 0}); // Intrinsic transparency

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: All transparent pixels map to index 0
    EXPECT_EQ(index_tile.at(0).index(), 0);
    EXPECT_EQ(index_tile.at(1).index(), 0);
    EXPECT_EQ(index_tile.at(2).index(), 0);
}

TEST(IndexTileFromColorTileTest, DuplicateColorsInTile)
{
    // Arrange: Create a tile with duplicate colors
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_red);
    color_tile.set(1, rgba_red);
    color_tile.set(2, rgba_red);
    color_tile.set(3, rgba_green);
    color_tile.set(4, rgba_green);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: All duplicates should map to the same index
    EXPECT_EQ(index_tile.at(0).index(), 1);
    EXPECT_EQ(index_tile.at(1).index(), 1);
    EXPECT_EQ(index_tile.at(2).index(), 1);
    EXPECT_EQ(index_tile.at(3).index(), 2);
    EXPECT_EQ(index_tile.at(4).index(), 2);
}

TEST(IndexTileFromColorTileTest, RoundTrip_IndexToColorToIndex)
{
    // Arrange: Start with an index tile, convert to color, then back to index
    PixelTile<IndexPixel> original_index_tile{};
    original_index_tile.set(0, IndexPixel{0}); // Transparent
    original_index_tile.set(1, IndexPixel{1}); // Red
    original_index_tile.set(2, IndexPixel{2}); // Green
    original_index_tile.set(3, IndexPixel{3}); // Blue
    original_index_tile.set(4, IndexPixel{1}); // Red (duplicate)

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Convert index -> color -> index
    auto color_tile = color_tile_from_index_tile(original_index_tile, palette);
    auto result_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Round trip should produce identical indices
    EXPECT_EQ(result_tile.at(0).index(), 0);
    EXPECT_EQ(result_tile.at(1).index(), 1);
    EXPECT_EQ(result_tile.at(2).index(), 2);
    EXPECT_EQ(result_tile.at(3).index(), 3);
    EXPECT_EQ(result_tile.at(4).index(), 1);
}

TEST(IndexTileFromColorTileTest, FullyPopulatedTile)
{
    // Arrange: Create a tile with a pattern using all 64 pixels
    PixelTile<Rgba32> color_tile{};
    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2
    palette.add(rgba_blue);    // Index 3

    // Fill tile with repeating pattern
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        switch (i % 4) {
        case 0:
            color_tile.set(i, rgba_magenta);
            break;
        case 1:
            color_tile.set(i, rgba_red);
            break;
        case 2:
            color_tile.set(i, rgba_green);
            break;
        case 3:
            color_tile.set(i, rgba_blue);
            break;
        }
    }

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Verify the pattern is correctly mapped
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(index_tile.at(i).index(), i % 4);
    }
}

TEST(IndexTileFromColorTileTest, DuplicateColorsInPalette_UsesFirstOccurrence)
{
    // Arrange: Create a palette with duplicate colors
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red); // First occurrence at index 1
    palette.add(rgba_green);
    palette.add(rgba_red); // Duplicate at index 3

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Should use first occurrence (index 1, not 3)
    EXPECT_EQ(index_tile.at(0).index(), 1);
}

TEST(IndexTileFromColorTileTest, ExtrinsicTransparencyTreatedAsTransparent)
{
    // Arrange: Create a tile where extrinsic transparency color appears
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_magenta); // Extrinsic transparent
    color_tile.set(1, rgba_red);     // Opaque
    color_tile.set(2, rgba_magenta); // Extrinsic transparent

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Extrinsic transparent pixels should be treated as transparent (index 0)
    EXPECT_EQ(index_tile.at(0).index(), 0);
    EXPECT_EQ(index_tile.at(1).index(), 1);
    EXPECT_EQ(index_tile.at(2).index(), 0);
}

TEST(IndexTileFromColorTileTest, EmptyPalette_AllTransparentTile)
{
    // Arrange: Create a tile with only transparent pixels and an empty palette
    PixelTile<Rgba32> color_tile{}; // All transparent by default

    Palette<Rgba32> palette{}; // Empty palette

    // Act: Convert color tile to index tile
    // Note: Empty palette is valid when tile has only transparent pixels
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: All pixels should be index 0
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(index_tile.at(i).index(), 0);
    }
}

TEST(IndexTileFromColorTileTest, SingleNonTransparentPixel)
{
    // Arrange: Create a tile with only one non-transparent pixel
    PixelTile<Rgba32> color_tile{};
    color_tile.set(30, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Convert color tile to index tile
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Single pixel should be correctly mapped
    EXPECT_EQ(index_tile.at(30).index(), 3);
}
