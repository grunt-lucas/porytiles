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
    PixelTile<Rgba32> tile{};    // All transparent by default
    tile.set(7, 3, rgba_yellow); // Linear index: 7*8 + 3 = 59

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
