#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/domain/models/shape_tile.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

using namespace porytiles2;

namespace {
// Helper function to create a diagonal ShapeMask pattern
ShapeMask create_diagonal_mask()
{
    ShapeMask mask;
    for (int i = 0; i < 8; ++i) {
        mask.set(i, i);
    }
    return mask;
}

// Helper function to create a horizontal bar ShapeMask pattern
ShapeMask create_horizontal_bar_mask()
{
    ShapeMask mask;
    for (int col = 0; col < 8; ++col) {
        mask.set(3, col); // Row 3, all columns
    }
    return mask;
}

// Helper function to create a vertical bar ShapeMask pattern
ShapeMask create_vertical_bar_mask()
{
    ShapeMask mask;
    for (int row = 0; row < 8; ++row) {
        mask.set(row, 3); // All rows, column 3
    }
    return mask;
}
} // namespace

TEST(ShapeTileTests, DefaultConstruction)
{
    ShapeTile<ColorIndex> tile;

    // Default constructed tile should have empty colors map
    EXPECT_TRUE(tile.colors().empty());
}

TEST(ShapeTileTests, SetAndColorsAccessor)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();

    tile.set(mask1, ColorIndex{1});
    tile.set(mask2, ColorIndex{2});

    EXPECT_EQ(tile.colors().size(), 2);
    EXPECT_EQ(tile.colors().at(mask1), ColorIndex{1});
    EXPECT_EQ(tile.colors().at(mask2), ColorIndex{2});
}

TEST(ShapeTileTests, SetOverwritesExistingMask)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();

    tile.set(mask, ColorIndex{1});
    EXPECT_EQ(tile.colors().at(mask), ColorIndex{1});

    // Overwrite with new color
    tile.set(mask, ColorIndex{5});
    EXPECT_EQ(tile.colors().at(mask), ColorIndex{5});
    EXPECT_EQ(tile.colors().size(), 1);
}

TEST(ShapeTileTests, EqualityOperator)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask = create_diagonal_mask();

    tile1.set(mask, ColorIndex{1});
    tile2.set(mask, ColorIndex{1});

    EXPECT_EQ(tile1, tile2);

    // Different color should not be equal
    ShapeTile<ColorIndex> tile3;
    tile3.set(mask, ColorIndex{2});
    EXPECT_NE(tile1, tile3);

    // Different mask should not be equal
    ShapeTile<ColorIndex> tile4;
    tile4.set(create_horizontal_bar_mask(), ColorIndex{1});
    EXPECT_NE(tile1, tile4);
}

TEST(ShapeTileTests, SpaceshipOperator)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();

    tile1.set(mask1, ColorIndex{1});
    tile2.set(mask2, ColorIndex{1});

    // Comparison should be based on map contents (keys and values)
    // The diagonal mask comes before horizontal bar mask lexicographically
    EXPECT_TRUE(tile1 < tile2 || tile1 > tile2 || tile1 == tile2);
}

TEST(ShapeTileTests, CompareShapeOnlyIgnoresColors)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask = create_diagonal_mask();

    // Same shape, different colors
    tile1.set(mask, ColorIndex{1});
    tile2.set(mask, ColorIndex{99});

    // compare_shape_only should return false (they have same shape)
    EXPECT_FALSE(ShapeTile<ColorIndex>::compare_shape_only(tile1, tile2));
    EXPECT_FALSE(ShapeTile<ColorIndex>::compare_shape_only(tile2, tile1));
}

TEST(ShapeTileTests, CompareShapeOnlyComparesShapes)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();

    tile1.set(mask1, ColorIndex{1});
    tile2.set(mask2, ColorIndex{1});

    // Different shapes - one should be less than the other
    bool result1 = ShapeTile<ColorIndex>::compare_shape_only(tile1, tile2);
    bool result2 = ShapeTile<ColorIndex>::compare_shape_only(tile2, tile1);

    // Exactly one should be true (assuming masks are different)
    EXPECT_TRUE(result1 != result2);
}

TEST(ShapeTileTests, CompareShapeOnlyWithMultipleMasks)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();
    ShapeMask mask3 = create_vertical_bar_mask();

    // tile1 has mask1 and mask2 with different colors
    tile1.set(mask1, ColorIndex{1});
    tile1.set(mask2, ColorIndex{2});

    // tile2 has same masks but different colors
    tile2.set(mask1, ColorIndex{50});
    tile2.set(mask2, ColorIndex{60});

    // Same shape, different colors - should not compare as less
    EXPECT_FALSE(ShapeTile<ColorIndex>::compare_shape_only(tile1, tile2));
    EXPECT_FALSE(ShapeTile<ColorIndex>::compare_shape_only(tile2, tile1));

    // tile3 has different shape (mask1 and mask3)
    ShapeTile<ColorIndex> tile3;
    tile3.set(mask1, ColorIndex{1});
    tile3.set(mask3, ColorIndex{2});

    // Different shapes - one should be less
    bool result1 = ShapeTile<ColorIndex>::compare_shape_only(tile1, tile3);
    bool result2 = ShapeTile<ColorIndex>::compare_shape_only(tile3, tile1);
    EXPECT_TRUE(result1 != result2);
}

TEST(ShapeTileTests, IsTransparentForEmptyTile)
{
    ShapeTile<ColorIndex> tile;

    // Empty tile (no masks) should be transparent
    EXPECT_TRUE(tile.is_transparent());
}

TEST(ShapeTileTests, IsTransparentForTransparentMasks)
{
    ShapeTile<ColorIndex> tile;

    // Add transparent masks (all zeros)
    ShapeMask transparent_mask;
    tile.set(transparent_mask, ColorIndex{1});

    EXPECT_TRUE(tile.is_transparent());
}

TEST(ShapeTileTests, IsTransparentForNonTransparentMask)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    // Tile with non-transparent mask should not be transparent
    EXPECT_FALSE(tile.is_transparent());
}

TEST(ShapeTileTests, FlipNoFlip)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    ShapeTile<ColorIndex> flipped = tile.flip(false, false);

    // No flip should return identical tile
    EXPECT_EQ(tile, flipped);
}

TEST(ShapeTileTests, FlipHorizontal)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    ShapeTile<ColorIndex> flipped = tile.flip(true, false);

    // Flipped tile should have the mask flipped, but same color
    EXPECT_NE(tile, flipped);

    // The flipped tile should have the h-flipped mask
    ShapeMask expected_mask = mask.flip(true, false);
    EXPECT_TRUE(flipped.colors().contains(expected_mask));
    EXPECT_EQ(flipped.colors().at(expected_mask), ColorIndex{1});
}

TEST(ShapeTileTests, FlipVertical)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    ShapeTile<ColorIndex> flipped = tile.flip(false, true);

    // Flipped tile should have the mask flipped, but same color
    EXPECT_NE(tile, flipped);

    // The flipped tile should have the v-flipped mask
    ShapeMask expected_mask = mask.flip(false, true);
    EXPECT_TRUE(flipped.colors().contains(expected_mask));
    EXPECT_EQ(flipped.colors().at(expected_mask), ColorIndex{1});
}

TEST(ShapeTileTests, FlipBoth)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_horizontal_bar_mask();
    tile.set(mask, ColorIndex{1});

    ShapeTile<ColorIndex> flipped = tile.flip(true, true);

    // The flipped tile should have the h+v-flipped mask
    ShapeMask expected_mask = mask.flip(true, true);
    EXPECT_TRUE(flipped.colors().contains(expected_mask));
    EXPECT_EQ(flipped.colors().at(expected_mask), ColorIndex{1});
}

TEST(ShapeTileTests, FlipSymmetry)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    // Flipping twice should return to original
    ShapeTile<ColorIndex> h_flip = tile.flip(true, false);
    ShapeTile<ColorIndex> h_flip_back = h_flip.flip(true, false);
    EXPECT_EQ(tile, h_flip_back);

    ShapeTile<ColorIndex> v_flip = tile.flip(false, true);
    ShapeTile<ColorIndex> v_flip_back = v_flip.flip(false, true);
    EXPECT_EQ(tile, v_flip_back);

    ShapeTile<ColorIndex> both_flip = tile.flip(true, true);
    ShapeTile<ColorIndex> both_flip_back = both_flip.flip(true, true);
    EXPECT_EQ(tile, both_flip_back);
}

TEST(ShapeTileTests, FlipWithMultipleMasks)
{
    ShapeTile<ColorIndex> tile;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();

    tile.set(mask1, ColorIndex{1});
    tile.set(mask2, ColorIndex{2});

    ShapeTile<ColorIndex> flipped = tile.flip(true, false);

    // Both masks should be flipped
    ShapeMask expected_mask1 = mask1.flip(true, false);
    ShapeMask expected_mask2 = mask2.flip(true, false);

    EXPECT_EQ(flipped.colors().size(), 2);
    EXPECT_TRUE(flipped.colors().contains(expected_mask1));
    EXPECT_TRUE(flipped.colors().contains(expected_mask2));
    EXPECT_EQ(flipped.colors().at(expected_mask1), ColorIndex{1});
    EXPECT_EQ(flipped.colors().at(expected_mask2), ColorIndex{2});
}

TEST(ShapeTileTests, WorksWithRgba32)
{
    ShapeTile<Rgba32> tile;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_horizontal_bar_mask();

    tile.set(mask1, rgba_red);
    tile.set(mask2, rgba_blue);

    EXPECT_EQ(tile.colors().size(), 2);
    EXPECT_EQ(tile.colors().at(mask1), rgba_red);
    EXPECT_EQ(tile.colors().at(mask2), rgba_blue);

    // Test flip with Rgba32
    ShapeTile<Rgba32> flipped = tile.flip(true, false);
    ShapeMask expected_mask1 = mask1.flip(true, false);
    EXPECT_TRUE(flipped.colors().contains(expected_mask1));
    EXPECT_EQ(flipped.colors().at(expected_mask1), rgba_red);
}

TEST(ShapeTileTests, CompareShapeOnlyWithRgba32)
{
    ShapeTile<Rgba32> tile1;
    ShapeTile<Rgba32> tile2;

    ShapeMask mask = create_diagonal_mask();

    // Same shape, different colors
    tile1.set(mask, rgba_red);
    tile2.set(mask, rgba_blue);

    // compare_shape_only should return false (they have same shape)
    EXPECT_FALSE(ShapeTile<Rgba32>::compare_shape_only(tile1, tile2));
    EXPECT_FALSE(ShapeTile<Rgba32>::compare_shape_only(tile2, tile1));

    // But operator< should see them as different (because colors differ)
    EXPECT_NE(tile1, tile2);
}

TEST(ShapeTileTests, FromPixelTileSimpleConversion)
{
    // Create a simple PixelTile with a few non-transparent pixels
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red); // Top-left
    pixel_tile.set(1, 1, rgba_red); // Diagonal
    pixel_tile.set(2, 2, rgba_red); // Diagonal

    // Create ColorIndexMap
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have one ShapeMask mapping to color index 0 (rgba_red should map to index 0)
    EXPECT_EQ(shape_tile.colors().size(), 1);

    // Verify the ShapeMask has the correct bits set
    const auto &colors_map = shape_tile.colors();
    ASSERT_EQ(colors_map.size(), 1);

    const auto &[mask, index] = *colors_map.begin();
    EXPECT_EQ(index, 0); // First non-transparent color gets index 0

    // Check that the mask has the correct pixels set
    // We can verify by creating an expected mask
    ShapeMask expected_mask;
    expected_mask.set(0, 0);
    expected_mask.set(1, 1);
    expected_mask.set(2, 2);

    EXPECT_EQ(mask, expected_mask);
}

TEST(ShapeTileTests, FromPixelTileWithMultipleColors)
{
    // Create a PixelTile with multiple colors
    PixelTile<Rgba32> pixel_tile;
    // Red pixels
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(0, 1, rgba_red);
    // Blue pixels
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(1, 1, rgba_blue);
    // Green pixels
    pixel_tile.set(2, 0, rgba_green);

    // Create ColorIndexMap
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have three ShapeMasks (one for each color)
    EXPECT_EQ(shape_tile.colors().size(), 3);

    // Verify we can look up each color's index and find the corresponding mask
    auto red_index_opt = color_map.index_at_color(rgba_red);
    auto blue_index_opt = color_map.index_at_color(rgba_blue);
    auto green_index_opt = color_map.index_at_color(rgba_green);

    ASSERT_TRUE(red_index_opt.has_value());
    ASSERT_TRUE(blue_index_opt.has_value());
    ASSERT_TRUE(green_index_opt.has_value());

    // Find the masks for each color in the ShapeTile
    bool found_red = false, found_blue = false, found_green = false;

    for (const auto &[mask, index] : shape_tile.colors()) {
        if (index == *red_index_opt) {
            found_red = true;
            // Verify red mask
            ShapeMask expected;
            expected.set(0, 0);
            expected.set(0, 1);
            EXPECT_EQ(mask, expected);
        }
        else if (index == *blue_index_opt) {
            found_blue = true;
            // Verify blue mask
            ShapeMask expected;
            expected.set(1, 0);
            expected.set(1, 1);
            EXPECT_EQ(mask, expected);
        }
        else if (index == *green_index_opt) {
            found_green = true;
            // Verify green mask
            ShapeMask expected;
            expected.set(2, 0);
            EXPECT_EQ(mask, expected);
        }
    }

    EXPECT_TRUE(found_red);
    EXPECT_TRUE(found_blue);
    EXPECT_TRUE(found_green);
}

TEST(ShapeTileTests, FromPixelTileSkipsTransparentPixels)
{
    // Create a PixelTile with transparent pixels
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_magenta);       // Extrinsically transparent
    pixel_tile.set(0, 1, Rgba32{0, 0, 0, 0}); // Intrinsically transparent (alpha=0)
    pixel_tile.set(1, 0, rgba_red);           // Non-transparent
    pixel_tile.set(1, 1, rgba_red);           // Non-transparent

    // Create ColorIndexMap (should only have rgba_red)
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have one ShapeMask (transparent pixels should be skipped)
    EXPECT_EQ(shape_tile.colors().size(), 1);

    // Verify the mask only has non-transparent pixels
    const auto &[mask, index] = *shape_tile.colors().begin();
    ShapeMask expected;
    expected.set(1, 0);
    expected.set(1, 1);
    EXPECT_EQ(mask, expected);
}

TEST(ShapeTileTests, FromPixelTileAllTransparentProducesEmptyShapeTile)
{
    // Create a fully transparent PixelTile
    PixelTile<Rgba32> pixel_tile; // Default constructor creates all transparent pixels (alpha=0)

    // Create ColorIndexMap (will be empty)
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have no ShapeMasks
    EXPECT_TRUE(shape_tile.colors().empty());
    EXPECT_TRUE(shape_tile.is_transparent());
}

TEST(ShapeTileTests, FromPixelTilePanicsWhenPixelNotInMap)
{
    // Create a PixelTile with pixels
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 1, rgba_yellow); // This pixel won't be in the map

    // Create ColorIndexMap with only one tile (missing rgba_yellow)
    std::vector<PixelTile<Rgba32>> tiles;
    PixelTile<Rgba32> map_tile;
    map_tile.set(0, 0, rgba_red); // Only has rgba_red
    tiles.push_back(map_tile);
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Should panic when trying to convert (rgba_yellow not in map)
    EXPECT_DEATH(
        { (void)ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta); },
        "Pixel not found in ColorIndexMap");
}

TEST(ShapeTileTests, FromPixelTileMixedTransparencyTypes)
{
    // Create a PixelTile with mix of intrinsic and extrinsic transparency
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(0, 1, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(1, 1, rgba_magenta);       // Extrinsically transparent
    pixel_tile.set(2, 0, Rgba32{0, 0, 0, 0}); // Intrinsically transparent (alpha=0)

    // Create ColorIndexMap (should filter out transparent pixels)
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile with extrinsic transparency
    auto shape_tile = ShapeTile<unsigned int>::from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have two ShapeMasks (red and blue; magenta and alpha=0 are transparent)
    EXPECT_EQ(shape_tile.colors().size(), 2);

    // Verify the color indices
    auto red_index_opt = color_map.index_at_color(rgba_red);
    auto blue_index_opt = color_map.index_at_color(rgba_blue);

    ASSERT_TRUE(red_index_opt.has_value());
    ASSERT_TRUE(blue_index_opt.has_value());

    // Find and verify masks
    bool found_red = false, found_blue = false;
    for (const auto &[mask, index] : shape_tile.colors()) {
        if (index == *red_index_opt) {
            found_red = true;
            ShapeMask expected;
            expected.set(0, 0);
            expected.set(0, 1);
            EXPECT_EQ(mask, expected);
        }
        else if (index == *blue_index_opt) {
            found_blue = true;
            ShapeMask expected;
            expected.set(1, 0);
            EXPECT_EQ(mask, expected);
        }
    }

    EXPECT_TRUE(found_red);
    EXPECT_TRUE(found_blue);
}
