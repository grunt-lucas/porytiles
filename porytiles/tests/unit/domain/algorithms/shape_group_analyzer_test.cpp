#include <gtest/gtest.h>

#include "porytiles/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;

namespace {

// Color constants for tests
const Rgba32 transparent = rgba_magenta;
const Rgba32 red{255, 0, 0, Rgba32::alpha_opaque};
const Rgba32 blue{0, 0, 255, Rgba32::alpha_opaque};
const Rgba32 green{0, 255, 0, Rgba32::alpha_opaque};
const Rgba32 yellow{255, 255, 0, Rgba32::alpha_opaque};
const Rgba32 white{255, 255, 255, Rgba32::alpha_opaque};
const Rgba32 black{0, 0, 0, Rgba32::alpha_opaque};

/// @brief Creates a simple pixel tile with a 2-color L-shape pattern.
///
/// @details
/// The tile has color1 on a vertical stripe (column 0) and color2 on a horizontal stripe (row 7, cols 1-3).
/// All other pixels are transparent.
PixelTile<Rgba32> make_l_tile(const Rgba32 &color1, const Rgba32 &color2)
{
    PixelTile<Rgba32> tile{transparent};
    for (std::size_t row = 0; row < 8; ++row) {
        tile.set(row, 0, color1);
    }
    for (std::size_t col = 1; col < 4; ++col) {
        tile.set(7, col, color2);
    }
    return tile;
}

PixelTile<Rgba32> make_block_tile(const Rgba32 &color, std::size_t size = 4)
{
    PixelTile<Rgba32> tile{transparent};
    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t col = 0; col < size; ++col) {
            tile.set(row, col, color);
        }
    }
    return tile;
}

} // namespace

TEST(ShapeGroupAnalyzerTests, TwoTilesSameShapeDifferentColors)
{
    auto tile1 = make_l_tile(red, blue);
    auto tile2 = make_l_tile(green, yellow);

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};
    auto groups = analyze_shape_groups(tiles, transparent);

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups[0].members.size(), 2);
    EXPECT_EQ(groups[0].members[0].tile_index, 0);
    EXPECT_EQ(groups[0].members[1].tile_index, 1);
}

TEST(ShapeGroupAnalyzerTests, TwoTilesDifferentShapes)
{
    auto tile1 = make_l_tile(red, blue);
    auto tile2 = make_block_tile(green);

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};
    auto groups = analyze_shape_groups(tiles, transparent);

    EXPECT_TRUE(groups.empty());
}

TEST(ShapeGroupAnalyzerTests, TwoTilesSameShapeSameColors)
{
    auto tile1 = make_l_tile(red, blue);
    auto tile2 = make_l_tile(red, blue);

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};
    auto groups = analyze_shape_groups(tiles, transparent);

    // Same colors means exact duplicates, not sharing candidates
    EXPECT_TRUE(groups.empty());
}

TEST(ShapeGroupAnalyzerTests, SingleTile)
{
    auto tile1 = make_l_tile(red, blue);

    std::vector<PixelTile<Rgba32>> tiles = {tile1};
    auto groups = analyze_shape_groups(tiles, transparent);

    EXPECT_TRUE(groups.empty());
}

TEST(ShapeGroupAnalyzerTests, FlippedVariantsSameShape)
{
    auto tile1 = make_l_tile(red, blue);
    // Create a horizontally flipped version with different colors
    auto tile2 = tile1.flip(true, false);
    // Change the colors in tile2 so it's not an exact duplicate after canonicalization
    PixelTile<Rgba32> tile2_recolored{transparent};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            auto pixel = tile2.at(row, col);
            if (pixel == red) {
                tile2_recolored.set(row, col, green);
            }
            else if (pixel == blue) {
                tile2_recolored.set(row, col, yellow);
            }
            else {
                tile2_recolored.set(row, col, pixel);
            }
        }
    }

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2_recolored};
    auto groups = analyze_shape_groups(tiles, transparent);

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups[0].members.size(), 2);
}

TEST(ShapeGroupAnalyzerTests, TransparentTileSkipped)
{
    PixelTile<Rgba32> transparent_tile{transparent};
    auto tile1 = make_l_tile(red, blue);

    std::vector<PixelTile<Rgba32>> tiles = {transparent_tile, tile1};
    auto groups = analyze_shape_groups(tiles, transparent);

    EXPECT_TRUE(groups.empty());
}

TEST(ShapeGroupAnalyzerTests, ThreeTilesSameShapeDifferentColors)
{
    auto tile1 = make_l_tile(red, blue);
    auto tile2 = make_l_tile(green, yellow);
    auto tile3 = make_l_tile(white, black);

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2, tile3};
    auto groups = analyze_shape_groups(tiles, transparent);

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups[0].members.size(), 3);
}

TEST(ShapeGroupAnalyzerTests, MultipleDistinctShapeGroups)
{
    // L-shape group with 2 color variants
    auto l_tile1 = make_l_tile(red, blue);
    auto l_tile2 = make_l_tile(green, yellow);
    // Block group with 2 color variants
    auto block_tile1 = make_block_tile(red);
    auto block_tile2 = make_block_tile(blue);

    std::vector<PixelTile<Rgba32>> tiles = {l_tile1, l_tile2, block_tile1, block_tile2};
    auto groups = analyze_shape_groups(tiles, transparent);

    ASSERT_EQ(groups.size(), 2);
    // Each group should have exactly 2 members
    EXPECT_EQ(groups[0].members.size(), 2);
    EXPECT_EQ(groups[1].members.size(), 2);
}

TEST(ShapeTileFromPixelTileTests, ConvertsCorrectly)
{
    auto tile = make_l_tile(red, blue);
    auto shape_tile = shape_tile_from_pixel_tile(tile, [](const Rgba32 &c) { return c.is_transparent(rgba_magenta); });

    // The L-tile has 2 colors -> 2 mask entries
    EXPECT_EQ(shape_tile.colors().size(), 2);
    EXPECT_FALSE(shape_tile.is_transparent());
}

TEST(ShapeTileFromPixelTileTests, TransparentTileIsTransparent)
{
    PixelTile<Rgba32> tile{transparent};
    auto shape_tile = shape_tile_from_pixel_tile(tile, [](const Rgba32 &c) { return c.is_transparent(rgba_magenta); });

    EXPECT_TRUE(shape_tile.is_transparent());
    EXPECT_TRUE(shape_tile.colors().empty());
}
