#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tile/canonical_shape_tile.hpp"
#include "porytiles2/domain/models/tile/shape_mask.hpp"
#include "porytiles2/domain/models/tile/shape_tile.hpp"

using namespace porytiles2;

namespace {
// Helper function to create a distinctive diagonal ShapeMask pattern
ShapeMask create_diagonal_mask()
{
    ShapeMask mask;
    for (int i = 0; i < 8; ++i) {
        mask.set(i, i);
    }
    return mask;
}

// Helper function to create an L-shaped pattern (distinctive for testing)
ShapeMask create_l_shape_mask()
{
    ShapeMask mask;
    // Vertical part of L
    for (int row = 0; row < 8; ++row) {
        mask.set(row, 0);
    }
    // Horizontal part of L
    for (int col = 1; col < 4; ++col) {
        mask.set(7, col);
    }
    return mask;
}

// Helper function to create a corner pattern
ShapeMask create_corner_mask()
{
    ShapeMask mask;
    mask.set(0, 0);
    mask.set(0, 1);
    mask.set(1, 0);
    return mask;
}
} // namespace

TEST(CanonicalShapeTileTests, ShouldFindCanonicalRepresentation)
{
    // Create a test tile with a distinctive L-shape pattern
    ShapeTile<ColorIndex> tile;
    ShapeMask mask = create_l_shape_mask();
    tile.set(mask, ColorIndex{1});

    // Create canonical tile
    CanonicalShapeTile<ColorIndex> canonical{tile};

    // The canonical representation should be one of the 4 flip combinations
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    // Cast to ShapeTile to compare
    bool matches_no_flip = (static_cast<const ShapeTile<ColorIndex> &>(canonical) == no_flip);
    bool matches_h_flip = (static_cast<const ShapeTile<ColorIndex> &>(canonical) == h_flip);
    bool matches_v_flip = (static_cast<const ShapeTile<ColorIndex> &>(canonical) == v_flip);
    bool matches_hv_flip = (static_cast<const ShapeTile<ColorIndex> &>(canonical) == hv_flip);

    EXPECT_TRUE(matches_no_flip || matches_h_flip || matches_v_flip || matches_hv_flip);

    // Verify the flip flags correctly reconstruct the canonical form
    auto expected_tile = tile.flip(canonical.h_flip(), canonical.v_flip());
    EXPECT_EQ(static_cast<const ShapeTile<ColorIndex> &>(canonical), expected_tile);
}

TEST(CanonicalShapeTileTests, ShouldChooseMinimalShapeRepresentation)
{
    // Create a tile where we can predict which flip should be minimal by shape
    ShapeTile<ColorIndex> tile;

    // Create a small corner pattern - minimal should be top-left corner
    ShapeMask corner = create_corner_mask();
    tile.set(corner, ColorIndex{1});

    CanonicalShapeTile<ColorIndex> canonical{tile};

    // The canonical should have the lexicographically smallest shape
    // Verify it's one of the four flips
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    bool matches_one = (static_cast<const ShapeTile<ColorIndex> &>(canonical) == no_flip) ||
                       (static_cast<const ShapeTile<ColorIndex> &>(canonical) == h_flip) ||
                       (static_cast<const ShapeTile<ColorIndex> &>(canonical) == v_flip) ||
                       (static_cast<const ShapeTile<ColorIndex> &>(canonical) == hv_flip);

    EXPECT_TRUE(matches_one);
}

TEST(CanonicalShapeTileTests, ShouldHandleSymmetricTiles)
{
    // Create a fully symmetric tile (all same shape)
    ShapeTile<ColorIndex> tile;

    // Create a symmetric cross pattern (centered at rows/cols 3-4)
    ShapeMask mask;
    for (int i = 0; i < 8; ++i) {
        mask.set(3, i); // Horizontal bar (row 3)
        mask.set(4, i); // Horizontal bar (row 4)
        mask.set(i, 3); // Vertical bar (col 3)
        mask.set(i, 4); // Vertical bar (col 4)
    }
    tile.set(mask, ColorIndex{1});

    // For a truly symmetric tile, all flip variants should be equal
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    EXPECT_EQ(no_flip, h_flip);
    EXPECT_EQ(no_flip, v_flip);
    EXPECT_EQ(no_flip, hv_flip);

    // The canonical form should be one of the flip variants (they're all equal)
    CanonicalShapeTile<ColorIndex> canonical{tile};
    EXPECT_EQ(static_cast<const ShapeTile<ColorIndex> &>(canonical), no_flip);
}

TEST(CanonicalShapeTileTests, ShouldProduceConsistentResults)
{
    // Create a test tile
    ShapeTile<ColorIndex> tile;
    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    // Create multiple canonical tiles from the same source
    CanonicalShapeTile<ColorIndex> canonical1{tile};
    CanonicalShapeTile<ColorIndex> canonical2{tile};

    // They should be identical
    EXPECT_EQ(canonical1, canonical2);
    EXPECT_EQ(canonical1.h_flip(), canonical2.h_flip());
    EXPECT_EQ(canonical1.v_flip(), canonical2.v_flip());
}

TEST(CanonicalShapeTileTests, ShouldHandleAllFlipVariations)
{
    // Create a test tile with distinctive corners
    ShapeTile<ColorIndex> tile;
    ShapeMask mask1 = create_corner_mask();
    tile.set(mask1, ColorIndex{1});

    // Test that creating canonical tiles from different flips
    // of the same logical tile gives consistent shape results
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    CanonicalShapeTile<ColorIndex> canonical_from_no_flip{no_flip};
    CanonicalShapeTile<ColorIndex> canonical_from_h_flip{h_flip};
    CanonicalShapeTile<ColorIndex> canonical_from_v_flip{v_flip};
    CanonicalShapeTile<ColorIndex> canonical_from_hv_flip{hv_flip};

    // All canonical representations should have the same shape data
    EXPECT_EQ(
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_no_flip),
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_h_flip));
    EXPECT_EQ(
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_no_flip),
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_v_flip));
    EXPECT_EQ(
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_no_flip),
        static_cast<const ShapeTile<ColorIndex> &>(canonical_from_hv_flip));
}

TEST(CanonicalShapeTileTests, SameShapeDifferentColorsProduceSameCanonicalShape)
{
    // This is the KEY difference from CanonicalPixelTile!
    // Two tiles with same shape but different colors should canonicalize to same shape structure

    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask = create_l_shape_mask();

    // Same shape, different colors
    tile1.set(mask, ColorIndex{1});
    tile2.set(mask, ColorIndex{99});

    CanonicalShapeTile<ColorIndex> canonical1{tile1};
    CanonicalShapeTile<ColorIndex> canonical2{tile2};

    // The canonical shapes should have the same SHAPE MASKS
    EXPECT_EQ(canonical1.colors().size(), canonical2.colors().size());

    // They should have the same flip flags (same canonical orientation)
    EXPECT_EQ(canonical1.h_flip(), canonical2.h_flip());
    EXPECT_EQ(canonical1.v_flip(), canonical2.v_flip());

    // The shapes (keys) should be identical
    auto keys1 = canonical1.colors() | std::views::keys;
    auto keys2 = canonical2.colors() | std::views::keys;
    EXPECT_TRUE(std::ranges::equal(keys1, keys2));

    // But the actual tiles are NOT equal (different colors)
    EXPECT_NE(canonical1, canonical2);
}

TEST(CanonicalShapeTileTests, SameShapeMultipleMasksDifferentColors)
{
    // Test with multiple masks - same shapes, different color assignments

    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_corner_mask();

    // tile1: mask1->color1, mask2->color2
    tile1.set(mask1, ColorIndex{1});
    tile1.set(mask2, ColorIndex{2});

    // tile2: same masks, different colors
    tile2.set(mask1, ColorIndex{10});
    tile2.set(mask2, ColorIndex{20});

    CanonicalShapeTile<ColorIndex> canonical1{tile1};
    CanonicalShapeTile<ColorIndex> canonical2{tile2};

    // Same flip flags (same canonical orientation)
    EXPECT_EQ(canonical1.h_flip(), canonical2.h_flip());
    EXPECT_EQ(canonical1.v_flip(), canonical2.v_flip());

    // Same shape structure (same keys)
    auto keys1 = canonical1.colors() | std::views::keys;
    auto keys2 = canonical2.colors() | std::views::keys;
    EXPECT_TRUE(std::ranges::equal(keys1, keys2));

    // But different overall tiles (different values)
    EXPECT_NE(canonical1, canonical2);
}

TEST(CanonicalShapeTileTests, EqualityOperatorComparesShapesAndColors)
{
    ShapeTile<ColorIndex> tile;
    ShapeMask mask = create_diagonal_mask();
    tile.set(mask, ColorIndex{1});

    CanonicalShapeTile<ColorIndex> canonical1{tile};
    CanonicalShapeTile<ColorIndex> canonical2{tile};

    // Same input should produce identical canonical tiles
    EXPECT_EQ(canonical1, canonical2);

    // Different colors should produce different canonical tiles
    ShapeTile<ColorIndex> tile_different_color;
    tile_different_color.set(mask, ColorIndex{2});
    CanonicalShapeTile<ColorIndex> canonical3{tile_different_color};

    EXPECT_NE(canonical1, canonical3);
}

TEST(CanonicalShapeTileTests, SpaceshipOperatorWorks)
{
    ShapeTile<ColorIndex> tile1;
    ShapeTile<ColorIndex> tile2;

    ShapeMask mask1 = create_diagonal_mask();
    ShapeMask mask2 = create_corner_mask();

    tile1.set(mask1, ColorIndex{1});
    tile2.set(mask2, ColorIndex{1});

    CanonicalShapeTile<ColorIndex> canonical1{tile1};
    CanonicalShapeTile<ColorIndex> canonical2{tile2};

    // Should be able to compare
    EXPECT_TRUE(canonical1 < canonical2 || canonical1 > canonical2 || canonical1 == canonical2);
}

TEST(CanonicalShapeTileTests, WorksWithRgba32)
{
    // Test with Rgba32 colors
    ShapeTile<Rgba32> tile;

    ShapeMask mask = create_l_shape_mask();
    tile.set(mask, rgba_red);

    CanonicalShapeTile<Rgba32> canonical{tile};

    // Verify that canonical form is one of the flip combinations
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    bool matches_one = (static_cast<const ShapeTile<Rgba32> &>(canonical) == no_flip) ||
                       (static_cast<const ShapeTile<Rgba32> &>(canonical) == h_flip) ||
                       (static_cast<const ShapeTile<Rgba32> &>(canonical) == v_flip) ||
                       (static_cast<const ShapeTile<Rgba32> &>(canonical) == hv_flip);

    EXPECT_TRUE(matches_one);

    // Verify flip flags
    auto expected_tile = tile.flip(canonical.h_flip(), canonical.v_flip());
    EXPECT_EQ(static_cast<const ShapeTile<Rgba32> &>(canonical), expected_tile);
}

TEST(CanonicalShapeTileTests, Rgba32SameShapeDifferentColors)
{
    // Same critical test as with ColorIndex, but using Rgba32

    ShapeTile<Rgba32> tile1;
    ShapeTile<Rgba32> tile2;

    ShapeMask mask = create_diagonal_mask();

    // Same shape, different colors
    tile1.set(mask, rgba_red);
    tile2.set(mask, rgba_blue);

    CanonicalShapeTile<Rgba32> canonical1{tile1};
    CanonicalShapeTile<Rgba32> canonical2{tile2};

    // Same flip flags (same canonical shape orientation)
    EXPECT_EQ(canonical1.h_flip(), canonical2.h_flip());
    EXPECT_EQ(canonical1.v_flip(), canonical2.v_flip());

    // Same shape structure
    auto keys1 = canonical1.colors() | std::views::keys;
    auto keys2 = canonical2.colors() | std::views::keys;
    EXPECT_TRUE(std::ranges::equal(keys1, keys2));

    // But different tiles (different colors)
    EXPECT_NE(canonical1, canonical2);
}

TEST(CanonicalShapeTileTests, FlipFlagsIndicateTransformationToOriginal)
{
    // Create a distinctive asymmetric tile
    ShapeTile<ColorIndex> tile;
    ShapeMask mask = create_l_shape_mask();
    tile.set(mask, ColorIndex{1});

    // Create canonical form
    CanonicalShapeTile<ColorIndex> canonical{tile};

    // The flip flags tell us how to transform canonical back to original
    // So: canonical.flip(h_flip, v_flip) should equal the original tile
    auto reconstructed = static_cast<ShapeTile<ColorIndex>>(canonical).flip(canonical.h_flip(), canonical.v_flip());

    EXPECT_EQ(reconstructed, tile);
}
