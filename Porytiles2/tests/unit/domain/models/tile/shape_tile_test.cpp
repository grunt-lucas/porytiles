#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tile/shape_mask.hpp"
#include "porytiles2/domain/models/tile/shape_tile.hpp"

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
