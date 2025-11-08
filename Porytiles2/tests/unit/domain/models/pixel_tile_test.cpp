#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(PixelTileTests, AtAndSetShouldWork)
{
    PixelTile<IndexPixel> tile{};

    EXPECT_EQ(0, tile.at(0).index());
    EXPECT_EQ(0, tile.at(63).index());
    EXPECT_DEATH(std::ignore = tile.at(tile::size_pix), "index out of bounds: 64");
    EXPECT_DEATH(std::ignore = tile.at(tile::side_length_pix, 2), "row index out of bounds: 8");
    EXPECT_DEATH(std::ignore = tile.at(0, tile::side_length_pix), "col index out of bounds: 8");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6).index());

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42).index());
}

TEST(PixelTileTests, IsTransparentShouldUseExtrinsicCorrectly)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentShouldUseAlphaCorrectly)
{
    // Default-constructed PixelTile<Rgba32> is zeroed, i.e. black and intrinsically transparent
    const PixelTile<Rgba32> tile{};
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentShouldUseMixedTransparencyCorrectly)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    // Set a pixel to non-magenta, but set alpha channel to transparent
    tile.set(12, Rgba32{22, 90, 144, Rgba32::alpha_transparent});

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldWorkWithIntrinsicTransparency)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with same transparent value (index 0 is the only intrinsically transparent value for IndexPixel)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, IndexPixel{0}); // transparent
        tile2.set(i, IndexPixel{0}); // same transparent value
    }

    // Both tiles should be considered equal since all pixels are transparent
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldDetectNonTransparentDifferences)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with same non-transparent value
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, IndexPixel{5});
        tile2.set(i, IndexPixel{5});
    }

    // Change one pixel in tile2 to a different non-transparent value
    tile2.set(10, IndexPixel{6});

    // Tiles should not be equal
    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_FALSE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldWorkWithMixedTransparentAndNonTransparent)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with mix of transparent and non-transparent values
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 2 == 0) {
            tile1.set(i, IndexPixel{0}); // transparent (index 0)
            tile2.set(i, IndexPixel{0}); // same transparent value
        }
        else {
            tile1.set(i, IndexPixel{5}); // non-transparent
            tile2.set(i, IndexPixel{5}); // same non-transparent value
        }
    }

    // Tiles should be equal since transparent pixels match logically and non-transparent match exactly
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldWorkWithExtrinsicTransparency)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill tile1 with extrinsically transparent pixels (magenta)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{255, 0, 255});
    }

    // Fill tile2 with intrinsically transparent pixels (alpha=0)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile2.set(i, Rgba32{0, 0, 0, Rgba32::alpha_transparent});
    }

    // With magenta as extrinsic transparency, both tiles should be considered equal
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldDetectExtrinsicNonTransparentDifferences)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill both tiles with same non-transparent color
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{100, 150, 200});
        tile2.set(i, Rgba32{100, 150, 200});
    }

    // Change one pixel in tile2
    tile2.set(20, Rgba32{100, 150, 201});

    // Tiles should not be equal
    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_FALSE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringTransparencyShouldWorkWithMixedExtrinsicTransparency)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill both tiles with mix of transparent and non-transparent
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 3 == 0) {
            tile1.set(i, Rgba32{255, 0, 255});                        // extrinsically transparent (magenta)
            tile2.set(i, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent
        }
        else if (i % 3 == 1) {
            tile1.set(i, Rgba32{100, 150, 200}); // non-transparent
            tile2.set(i, Rgba32{100, 150, 200}); // same non-transparent
        }
        else {
            tile1.set(i, Rgba32{50, 75, 100, Rgba32::alpha_transparent}); // intrinsically transparent
            tile2.set(i, Rgba32{255, 0, 255});                            // extrinsically transparent (magenta)
        }
    }

    // With magenta as extrinsic transparency, tiles should be equal
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldReturnEmptySetForDefaultTile)
{
    const PixelTile<IndexPixel> tile{};

    const auto colors = tile.unique_nontransparent_colors();

    // Default-constructed tile has all pixels set to IndexPixel{0} (transparent), so should return empty set
    EXPECT_EQ(0, colors.size());
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldReturnSingleColorForUniformTile)
{
    PixelTile<IndexPixel> tile{};

    // Fill entire tile with same non-transparent color
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{7});
    }

    const auto colors = tile.unique_nontransparent_colors();

    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{7}));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldReturnMultipleColorsForVariedTile)
{
    PixelTile<IndexPixel> tile{};

    // Fill tile with multiple non-transparent colors in a pattern
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 4 == 0) {
            tile.set(i, IndexPixel{1});
        }
        else if (i % 4 == 1) {
            tile.set(i, IndexPixel{2});
        }
        else if (i % 4 == 2) {
            tile.set(i, IndexPixel{3});
        }
        else {
            tile.set(i, IndexPixel{4});
        }
    }

    const auto colors = tile.unique_nontransparent_colors();

    // Should have exactly 4 unique non-transparent colors
    EXPECT_EQ(4, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{1}));
    EXPECT_TRUE(colors.contains(IndexPixel{2}));
    EXPECT_TRUE(colors.contains(IndexPixel{3}));
    EXPECT_TRUE(colors.contains(IndexPixel{4}));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldWorkWithRgba32)
{
    PixelTile<Rgba32> tile{};

    // Set some pixels to different non-transparent colors
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue
    tile.set(3, Rgba32{255, 0, 0}); // red again (duplicate)

    // Rest remain default (black with alpha=0, intrinsically transparent)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should have 3 unique non-transparent colors: red, green, blue
    // Default black with alpha=0 is filtered out because it's intrinsically transparent
    EXPECT_EQ(3, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{255, 0, 0}));
    EXPECT_TRUE(colors.contains(Rgba32{0, 255, 0}));
    EXPECT_TRUE(colors.contains(Rgba32{0, 0, 255}));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldExcludeIntrinsicTransparentIndexPixel)
{
    PixelTile<IndexPixel> tile{};

    // Fill tile with mix of transparent (index 0) and non-transparent colors
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 2 == 0) {
            tile.set(i, IndexPixel{0}); // intrinsically transparent
        }
        else {
            tile.set(i, IndexPixel{5}); // non-transparent
        }
    }

    const auto colors = tile.unique_nontransparent_colors();

    // Should only include the non-transparent index 5, not the transparent index 0
    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{5}));
    EXPECT_FALSE(colors.contains(IndexPixel{0}));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldExcludeAllIntrinsicTransparentRgba32Values)
{
    PixelTile<Rgba32> tile{};

    // Set pixels to different RGB values, all with alpha=0 (intrinsically transparent)
    tile.set(0, Rgba32{255, 0, 0, Rgba32::alpha_transparent});     // red, transparent
    tile.set(1, Rgba32{0, 255, 0, Rgba32::alpha_transparent});     // green, transparent
    tile.set(2, Rgba32{0, 0, 255, Rgba32::alpha_transparent});     // blue, transparent
    tile.set(3, Rgba32{255, 0, 0, Rgba32::alpha_transparent});     // red, transparent (duplicate)
    tile.set(4, Rgba32{100, 100, 100, Rgba32::alpha_transparent}); // gray, transparent

    // Rest remain default (black with alpha=0)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should return empty set because all pixels are intrinsically transparent
    EXPECT_EQ(0, colors.size());
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldExcludeExtrinsicTransparentColors)
{
    PixelTile<Rgba32> tile{};

    // Set some pixels to extrinsically transparent color (magenta)
    tile.set(0, Rgba32{255, 0, 255});   // magenta (extrinsically transparent)
    tile.set(1, Rgba32{255, 0, 255});   // magenta (duplicate)
    tile.set(2, Rgba32{100, 150, 200}); // non-transparent color

    // Rest remain default (black with alpha=0, intrinsically transparent)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should only include the non-transparent color
    // Magenta is excluded (extrinsically transparent)
    // Default black is excluded (intrinsically transparent)
    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{100, 150, 200}));
    EXPECT_FALSE(colors.contains(Rgba32{255, 0, 255}));
    EXPECT_FALSE(colors.contains(Rgba32{}));
}

TEST(PixelTileTests, UniqueNontransparentColorsShouldExcludeBothIntrinsicAndExtrinsicTransparentColors)
{
    PixelTile<Rgba32> tile{};

    // Create a tile with various types of transparency and non-transparent colors
    tile.set(0, Rgba32{255, 0, 255});                          // extrinsically transparent (magenta)
    tile.set(1, Rgba32{100, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent (red with alpha=0)
    tile.set(2, Rgba32{0, 100, 0, Rgba32::alpha_transparent}); // intrinsically transparent (green with alpha=0)
    tile.set(3, Rgba32{200, 200, 200});                        // non-transparent (gray)
    tile.set(4, Rgba32{50, 50, 50});                           // non-transparent (dark gray)
    tile.set(5, Rgba32{255, 0, 255});                          // extrinsically transparent (magenta, duplicate)
    tile.set(
        6, Rgba32{100, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent (red with alpha=0, duplicate)

    // Rest remain default (black with alpha=0)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should only include the 2 non-transparent colors: gray and dark gray
    // Magenta is excluded (extrinsically transparent)
    // Red and green with alpha=0 are excluded (intrinsically transparent)
    // Default black is excluded (intrinsically transparent)
    EXPECT_EQ(2, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{200, 200, 200}));
    EXPECT_TRUE(colors.contains(Rgba32{50, 50, 50}));
    EXPECT_FALSE(colors.contains(Rgba32{255, 0, 255}));
    EXPECT_FALSE(colors.contains(Rgba32{100, 0, 0, Rgba32::alpha_transparent}));
    EXPECT_FALSE(colors.contains(Rgba32{0, 100, 0, Rgba32::alpha_transparent}));
    EXPECT_FALSE(colors.contains(Rgba32{}));
}
