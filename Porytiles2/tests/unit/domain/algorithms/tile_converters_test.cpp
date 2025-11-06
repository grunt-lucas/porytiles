#include <gtest/gtest.h>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/domain/models/shape_tile.hpp"

using namespace porytiles2;

// ==================================================
// from_pixel_tile tests
// ==================================================

TEST(TileConvertersTests, FromPixelTileSimpleConversion)
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
    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

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

TEST(TileConvertersTests, FromPixelTileWithMultipleColors)
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
    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

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

TEST(TileConvertersTests, FromPixelTileSkipsTransparentPixels)
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
    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have one ShapeMask (transparent pixels should be skipped)
    EXPECT_EQ(shape_tile.colors().size(), 1);

    // Verify the mask only has non-transparent pixels
    const auto &[mask, index] = *shape_tile.colors().begin();
    ShapeMask expected;
    expected.set(1, 0);
    expected.set(1, 1);
    EXPECT_EQ(mask, expected);
}

TEST(TileConvertersTests, FromPixelTileAllTransparentProducesEmptyShapeTile)
{
    // Create a fully transparent PixelTile
    PixelTile<Rgba32> pixel_tile; // Default constructor creates all transparent pixels (alpha=0)

    // Create ColorIndexMap (will be empty)
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    // Should have no ShapeMasks
    EXPECT_TRUE(shape_tile.colors().empty());
    EXPECT_TRUE(shape_tile.is_transparent());
}

TEST(TileConvertersTests, FromPixelTilePanicsWhenPixelNotInMap)
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
    EXPECT_DEATH({ (void)from_pixel_tile(pixel_tile, color_map, rgba_magenta); }, "Pixel not found in ColorIndexMap");
}

TEST(TileConvertersTests, FromPixelTileMixedTransparencyTypes)
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
    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

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

// ==================================================
// from_shape_tile tests
// ==================================================

TEST(TileConvertersTests, FromShapeTileSimpleConversion)
{
    // Create a ShapeTile with one mask and one color
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    mask.set(1, 1);
    mask.set(2, 2);
    shape_tile.set(mask, ColorIndex{0});

    // Create ColorIndexMap
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // Check the pixels that should be set
    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(1, 1), rgba_red);
    EXPECT_EQ(result.at(2, 2), rgba_red);

    // Check some pixels that should be transparent
    EXPECT_EQ(result.at(0, 1), Rgba32{});
    EXPECT_EQ(result.at(1, 0), Rgba32{});
}

TEST(TileConvertersTests, FromShapeTileWithMultipleColors)
{
    // Create a ShapeTile with multiple masks and colors
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask red_mask;
    red_mask.set(0, 0);
    red_mask.set(0, 1);

    ShapeMask blue_mask;
    blue_mask.set(1, 0);
    blue_mask.set(1, 1);

    ShapeMask green_mask;
    green_mask.set(2, 0);

    // Create ColorIndexMap
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(2, 0, rgba_green);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Get indices
    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);
    auto green_index = *color_map.index_at_color(rgba_green);

    // Set up ShapeTile
    shape_tile.set(red_mask, red_index);
    shape_tile.set(blue_mask, blue_index);
    shape_tile.set(green_mask, green_index);

    // Convert to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // Verify red pixels
    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(0, 1), rgba_red);

    // Verify blue pixels
    EXPECT_EQ(result.at(1, 0), rgba_blue);
    EXPECT_EQ(result.at(1, 1), rgba_blue);

    // Verify green pixel
    EXPECT_EQ(result.at(2, 0), rgba_green);

    // Verify some transparent pixels
    EXPECT_EQ(result.at(3, 0), Rgba32{});
    EXPECT_EQ(result.at(0, 7), Rgba32{});
}

TEST(TileConvertersTests, FromShapeTileEmptyProducesTransparentTile)
{
    // Create an empty ShapeTile
    ShapeTile<ColorIndex> shape_tile;

    // Create ColorIndexMap (can be empty)
    std::vector<PixelTile<Rgba32>> tiles;
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // All pixels should be transparent
    EXPECT_TRUE(result.is_transparent(rgba_magenta));
}

TEST(TileConvertersTests, FromShapeTilePanicsWhenIndexNotInMap)
{
    // Create a ShapeTile with a ColorIndex that won't be in the map
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    shape_tile.set(mask, ColorIndex{99}); // Index 99 won't exist

    // Create ColorIndexMap with only one color
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Should panic when trying to convert
    EXPECT_DEATH({ (void)from_shape_tile<Rgba32>(shape_tile, color_map); }, "ColorIndex not found in ColorIndexMap");
}

TEST(TileConvertersTests, FromShapeTilePanicsOnOverlappingMasks)
{
    // Create a ShapeTile with overlapping masks
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask mask1;
    mask1.set(0, 0);
    mask1.set(1, 1);

    ShapeMask mask2;
    mask2.set(1, 1); // Overlaps with mask1
    mask2.set(2, 2);

    // Create ColorIndexMap
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);

    shape_tile.set(mask1, red_index);
    shape_tile.set(mask2, blue_index);

    // Should panic due to overlapping masks
    EXPECT_DEATH({ (void)from_shape_tile<Rgba32>(shape_tile, color_map); }, "Overlapping masks detected in ShapeTile");
}

// ==================================================
// shape_tile_to_pixel_colors tests
// ==================================================

TEST(TileConvertersTests, ShapeTileToPixelColorsSimpleConversion)
{
    // Create a ShapeTile with one mask and one color index
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    mask.set(1, 1);
    mask.set(2, 2);
    shape_tile.set(mask, ColorIndex{0});

    // Create ColorIndexMap
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile<Rgba32>
    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);

    // Should have one mask with the red color
    EXPECT_EQ(result.colors().size(), 1);

    const auto &[result_mask, result_color] = *result.colors().begin();
    EXPECT_EQ(result_mask, mask);
    EXPECT_EQ(result_color, rgba_red);
}

TEST(TileConvertersTests, ShapeTileToPixelColorsWithMultipleColors)
{
    // Create a ShapeTile with multiple masks and color indices
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask red_mask;
    red_mask.set(0, 0);
    red_mask.set(0, 1);

    ShapeMask blue_mask;
    blue_mask.set(1, 0);
    blue_mask.set(1, 1);

    ShapeMask green_mask;
    green_mask.set(2, 0);

    // Create ColorIndexMap
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(2, 0, rgba_green);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Get indices
    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);
    auto green_index = *color_map.index_at_color(rgba_green);

    // Set up ShapeTile
    shape_tile.set(red_mask, red_index);
    shape_tile.set(blue_mask, blue_index);
    shape_tile.set(green_mask, green_index);

    // Convert to ShapeTile<Rgba32>
    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);

    // Should have three masks
    EXPECT_EQ(result.colors().size(), 3);

    // Verify each mask and color
    bool found_red = false, found_blue = false, found_green = false;

    for (const auto &[mask, color] : result.colors()) {
        if (color == rgba_red) {
            found_red = true;
            EXPECT_EQ(mask, red_mask);
        }
        else if (color == rgba_blue) {
            found_blue = true;
            EXPECT_EQ(mask, blue_mask);
        }
        else if (color == rgba_green) {
            found_green = true;
            EXPECT_EQ(mask, green_mask);
        }
    }

    EXPECT_TRUE(found_red);
    EXPECT_TRUE(found_blue);
    EXPECT_TRUE(found_green);
}

TEST(TileConvertersTests, ShapeTileToPixelColorsEmptyProducesEmptyShapeTile)
{
    // Create an empty ShapeTile
    ShapeTile<ColorIndex> shape_tile;

    // Create ColorIndexMap (can be empty)
    std::vector<PixelTile<Rgba32>> tiles;
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile<Rgba32>
    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);

    // Should be empty
    EXPECT_TRUE(result.colors().empty());
    EXPECT_TRUE(result.is_transparent());
}

TEST(TileConvertersTests, ShapeTileToPixelColorsPanicsWhenIndexNotInMap)
{
    // Create a ShapeTile with a ColorIndex that won't be in the map
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    shape_tile.set(mask, ColorIndex{99}); // Index 99 won't exist

    // Create ColorIndexMap with only one color
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    std::vector<PixelTile<Rgba32>> tiles{pixel_tile};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Should panic when trying to convert
    EXPECT_DEATH(
        { (void)shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map); }, "ColorIndex not found in ColorIndexMap");
}

// ==================================================
// Round-trip conversion tests
// ==================================================

TEST(TileConvertersTests, RoundTripPixelToShapeToPixel)
{
    // Create an original PixelTile
    PixelTile<Rgba32> original;
    original.set(0, 0, rgba_red);
    original.set(0, 1, rgba_red);
    original.set(1, 0, rgba_blue);
    original.set(1, 1, rgba_blue);
    original.set(2, 0, rgba_green);

    // Create ColorIndexMap
    std::vector<PixelTile<Rgba32>> tiles{original};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);

    // Convert back to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // Should equal the original (ignoring transparency representation)
    EXPECT_TRUE(result.equals_ignoring_transparency(original, rgba_magenta));
}

TEST(TileConvertersTests, RoundTripWithTransparentPixels)
{
    // Create a PixelTile with some transparent pixels
    PixelTile<Rgba32> original;
    original.set(0, 0, rgba_red);
    original.set(1, 1, rgba_blue);
    original.set(2, 2, rgba_green);
    // Rest are transparent

    // Create ColorIndexMap
    std::vector<PixelTile<Rgba32>> tiles{original};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);

    // Convert back to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // Should equal the original (ignoring transparency representation)
    EXPECT_TRUE(result.equals_ignoring_transparency(original, rgba_magenta));

    // Verify specific pixels
    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(1, 1), rgba_blue);
    EXPECT_EQ(result.at(2, 2), rgba_green);
}

TEST(TileConvertersTests, RoundTripAllTransparent)
{
    // Create a fully transparent PixelTile
    PixelTile<Rgba32> original; // Default is all transparent

    // Create ColorIndexMap (empty)
    std::vector<PixelTile<Rgba32>> tiles{original};
    ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};

    // Convert to ShapeTile
    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);

    // Convert back to PixelTile
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    // Should be transparent
    EXPECT_TRUE(result.is_transparent(rgba_magenta));
}
