#include <gtest/gtest.h>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
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
    EXPECT_EQ(index, ColorIndex{0}); // First non-transparent color gets index 0

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

// ==================================================
// color_tile_from_index_tile tests
// ==================================================

TEST(TileConvertersTests, ColorTileFromIndexTile_BasicConversion)
{
    // Arrange: Create a simple indexed tile and palette
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0}); // Maps to rgba_magenta (extrinsic transparency)
    index_tile.set(0, 1, IndexPixel{1}); // Maps to rgba_red
    index_tile.set(0, 2, IndexPixel{2}); // Maps to rgba_green
    index_tile.set(1, 0, IndexPixel{3}); // Maps to rgba_blue

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    // Act: Convert index tile to color tile (extrinsic transparency)
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify colors are correctly mapped
    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_green);
    EXPECT_EQ(color_tile.at(1, 0), rgba_blue);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_MultipleDifferentColors)
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

    // Act: Convert index tile to color tile (extrinsic transparency)
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify specific positions have correct colors
    EXPECT_EQ(color_tile.at(0), rgba_red);
    EXPECT_EQ(color_tile.at(5), rgba_green);
    EXPECT_EQ(color_tile.at(10), rgba_blue);
    EXPECT_EQ(color_tile.at(63), rgba_yellow);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_DuplicateIndices)
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

    // Act: Convert index tile to color tile (extrinsic transparency)
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify all duplicate indices map to the same color
    EXPECT_EQ(color_tile.at(0, 0), rgba_red);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_red);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
    EXPECT_EQ(color_tile.at(1, 1), rgba_green);
    EXPECT_EQ(color_tile.at(2, 0), rgba_red);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_CorrectColorMappingAllPixels)
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
        index_tile.set(i, IndexPixel{i % 4});
    }

    // Act: Convert index tile to color tile (extrinsic transparency)
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify the pattern is correctly mapped
    const std::array<Rgba32, 4> expected_colors = {rgba_magenta, rgba_red, rgba_green, rgba_blue};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(color_tile.at(i), expected_colors[i % 4]);
    }
}

TEST(TileConvertersTests, ColorTileFromIndexTile_EmptyPalette_Panics)
{
    // Arrange: Create an indexed tile and an empty palette
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});

    Palette<Rgba32> palette{}; // Empty palette

    // Act & Assert: Should panic when palette is empty
    EXPECT_DEATH({ std::ignore = color_tile_from_index_tile(index_tile, palette, rgba_magenta); }, "palette is empty");
}

TEST(TileConvertersTests, ColorTileFromIndexTile_OutOfBoundsIndex_Panics)
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
    EXPECT_DEATH(
        { std::ignore = color_tile_from_index_tile(index_tile, palette, rgba_magenta); },
        "index 5 out of palette bounds");
}

TEST(TileConvertersTests, ColorTileFromIndexTile_FullyPopulatedPalette)
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
        index_tile.set(i, IndexPixel{i});
    }

    // Act: Convert index tile to color tile
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify all 16 colors are correctly mapped
    EXPECT_EQ(color_tile.at(0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1), rgba_red);
    EXPECT_EQ(color_tile.at(2), rgba_green);
    EXPECT_EQ(color_tile.at(3), rgba_blue);
    EXPECT_EQ(color_tile.at(4), rgba_yellow);

    // Check a generated color
    EXPECT_EQ(color_tile.at(5), (Rgba32{50, 75, 100}));
}

TEST(TileConvertersTests, ColorTileFromIndexTile_TransparencyAtIndex0)
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
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify transparency color is correctly mapped
    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_magenta);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_SingleColorPalette)
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
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: All pixels should map to the extrinsic transparency color, NOT the pal slot 0 color
    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1, 1), rgba_magenta);
    EXPECT_EQ(color_tile.at(2, 2), rgba_magenta);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_BoundaryIndexValues)
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
    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    // Assert: Verify boundary values are handled correctly
    EXPECT_EQ(color_tile.at(0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1), (Rgba32{240, 240, 240}));
    EXPECT_EQ(color_tile.at(2), (Rgba32{128, 128, 128}));
}

// ==================================================
// index_tile_from_color_tile tests (Extrinsic Transparency)
// ==================================================

TEST(TileConvertersTests, IndexTileFromColorTile_BasicConversion)
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

TEST(TileConvertersTests, IndexTileFromColorTile_AllColorsFound)
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

TEST(TileConvertersTests, IndexTileFromColorTile_ColorNotInPalette_Panics)
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

TEST(TileConvertersTests, IndexTileFromColorTile_AllTransparent_IntrinsicAndExtrinsic)
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

TEST(TileConvertersTests, IndexTileFromColorTile_DuplicateColorsInTile)
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

TEST(TileConvertersTests, IndexTileFromColorTile_RoundTrip_IndexToColorToIndex)
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
    auto color_tile = color_tile_from_index_tile(original_index_tile, palette, rgba_magenta);
    auto result_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    // Assert: Round trip should produce identical indices
    EXPECT_EQ(result_tile.at(0).index(), 0);
    EXPECT_EQ(result_tile.at(1).index(), 1);
    EXPECT_EQ(result_tile.at(2).index(), 2);
    EXPECT_EQ(result_tile.at(3).index(), 3);
    EXPECT_EQ(result_tile.at(4).index(), 1);
}

TEST(TileConvertersTests, IndexTileFromColorTile_FullyPopulatedTile)
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

TEST(TileConvertersTests, IndexTileFromColorTile_DuplicateColorsInPalette_UsesFirstOccurrence)
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

TEST(TileConvertersTests, IndexTileFromColorTile_ExtrinsicTransparencyTreatedAsTransparent)
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

TEST(TileConvertersTests, IndexTileFromColorTile_EmptyPalette_AllTransparentTile)
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

TEST(TileConvertersTests, IndexTileFromColorTile_SingleNonTransparentPixel)
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
