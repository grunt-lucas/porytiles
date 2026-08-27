#include <gtest/gtest.h>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/color_set.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/shape_mask.hpp"
#include "porytiles/domain/models/shape_tile.hpp"

using namespace porytiles;

TEST(TileConvertersTests, FromPixelTileSimpleConversion)
{
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 1, rgba_red);
    pixel_tile.set(2, 2, rgba_red);

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    EXPECT_EQ(shape_tile.colors().size(), 1);

    const auto &colors_map = shape_tile.colors();
    ASSERT_EQ(colors_map.size(), 1);

    const auto &[mask, index] = *colors_map.begin();
    EXPECT_EQ(index, ColorIndex{0});

    ShapeMask expected_mask;
    expected_mask.set(0, 0);
    expected_mask.set(1, 1);
    expected_mask.set(2, 2);

    EXPECT_EQ(mask, expected_mask);
}

TEST(TileConvertersTests, FromPixelTileWithMultipleColors)
{
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(0, 1, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(1, 1, rgba_blue);
    pixel_tile.set(2, 0, rgba_green);

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    EXPECT_EQ(shape_tile.colors().size(), 3);
    auto red_index_opt = color_map.index_at_color(rgba_red);
    auto blue_index_opt = color_map.index_at_color(rgba_blue);
    auto green_index_opt = color_map.index_at_color(rgba_green);

    ASSERT_TRUE(red_index_opt.has_value());
    ASSERT_TRUE(blue_index_opt.has_value());
    ASSERT_TRUE(green_index_opt.has_value());

    bool found_red = false, found_blue = false, found_green = false;

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
            expected.set(1, 1);
            EXPECT_EQ(mask, expected);
        }
        else if (index == *green_index_opt) {
            found_green = true;
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
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_magenta);       // Extrinsically transparent
    pixel_tile.set(0, 1, Rgba32{0, 0, 0, 0}); // Intrinsically transparent (alpha=0)
    pixel_tile.set(1, 0, rgba_red);
    pixel_tile.set(1, 1, rgba_red);

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);

    EXPECT_EQ(shape_tile.colors().size(), 1);

    const auto &[mask, index] = *shape_tile.colors().begin();
    ShapeMask expected;
    expected.set(1, 0);
    expected.set(1, 1);
    EXPECT_EQ(mask, expected);
}

TEST(TileConvertersTests, FromPixelTileAllTransparentProducesEmptyShapeTile)
{
    PixelTile<Rgba32> pixel_tile;

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);
    EXPECT_TRUE(shape_tile.colors().empty());
    EXPECT_TRUE(shape_tile.is_transparent());
}

TEST(TileConvertersTests, FromPixelTilePanicsWhenPixelNotInMap)
{
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 1, rgba_yellow);

    PixelTile<Rgba32> map_tile;
    map_tile.set(0, 0, rgba_red);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(map_tile, rgba_magenta);
    EXPECT_DEATH({ (void)from_pixel_tile(pixel_tile, color_map, rgba_magenta); }, "Pixel not found in ColorIndexMap");
}

TEST(TileConvertersTests, FromPixelTileMixedTransparencyTypes)
{
    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(0, 1, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(1, 1, rgba_magenta);       // Extrinsically transparent
    pixel_tile.set(2, 0, Rgba32{0, 0, 0, 0}); // Intrinsically transparent (alpha=0)

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto shape_tile = from_pixel_tile(pixel_tile, color_map, rgba_magenta);
    EXPECT_EQ(shape_tile.colors().size(), 2);

    auto red_index_opt = color_map.index_at_color(rgba_red);
    auto blue_index_opt = color_map.index_at_color(rgba_blue);

    ASSERT_TRUE(red_index_opt.has_value());
    ASSERT_TRUE(blue_index_opt.has_value());

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

TEST(TileConvertersTests, FromShapeTileSimpleConversion)
{
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    mask.set(1, 1);
    mask.set(2, 2);
    shape_tile.set(mask, ColorIndex{0});

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(1, 1), rgba_red);
    EXPECT_EQ(result.at(2, 2), rgba_red);

    EXPECT_EQ(result.at(0, 1), Rgba32{});
    EXPECT_EQ(result.at(1, 0), Rgba32{});
}

TEST(TileConvertersTests, FromShapeTileWithMultipleColors)
{
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask red_mask;
    red_mask.set(0, 0);
    red_mask.set(0, 1);

    ShapeMask blue_mask;
    blue_mask.set(1, 0);
    blue_mask.set(1, 1);

    ShapeMask green_mask;
    green_mask.set(2, 0);

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(2, 0, rgba_green);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);
    auto green_index = *color_map.index_at_color(rgba_green);

    shape_tile.set(red_mask, red_index);
    shape_tile.set(blue_mask, blue_index);
    shape_tile.set(green_mask, green_index);

    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(0, 1), rgba_red);

    EXPECT_EQ(result.at(1, 0), rgba_blue);
    EXPECT_EQ(result.at(1, 1), rgba_blue);

    EXPECT_EQ(result.at(2, 0), rgba_green);
    EXPECT_EQ(result.at(3, 0), Rgba32{});
    EXPECT_EQ(result.at(0, 7), Rgba32{});
}

TEST(TileConvertersTests, FromShapeTileEmptyProducesTransparentTile)
{
    ShapeTile<ColorIndex> shape_tile;
    ColorIndexMap<Rgba32> color_map{};

    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);
    EXPECT_TRUE(result.is_transparent(rgba_magenta));
}

TEST(TileConvertersTests, FromShapeTilePanicsWhenIndexNotInMap)
{
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    shape_tile.set(mask, ColorIndex{99});

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);
    EXPECT_DEATH({ (void)from_shape_tile<Rgba32>(shape_tile, color_map); }, "ColorIndex not found in ColorIndexMap");
}

TEST(TileConvertersTests, FromShapeTilePanicsOnOverlappingMasks)
{
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask mask1;
    mask1.set(0, 0);
    mask1.set(1, 1);

    ShapeMask mask2;
    mask2.set(1, 1);
    mask2.set(2, 2);

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);

    shape_tile.set(mask1, red_index);
    shape_tile.set(mask2, blue_index);

    EXPECT_DEATH({ (void)from_shape_tile<Rgba32>(shape_tile, color_map); }, "Overlapping masks detected in ShapeTile");
}

TEST(TileConvertersTests, ShapeTileToPixelColorsSimpleConversion)
{
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    mask.set(1, 1);
    mask.set(2, 2);
    shape_tile.set(mask, ColorIndex{0});

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);

    EXPECT_EQ(result.colors().size(), 1);

    const auto &[result_mask, result_color] = *result.colors().begin();
    EXPECT_EQ(result_mask, mask);
    EXPECT_EQ(result_color, rgba_red);
}

TEST(TileConvertersTests, ShapeTileToPixelColorsWithMultipleColors)
{
    ShapeTile<ColorIndex> shape_tile;

    ShapeMask red_mask;
    red_mask.set(0, 0);
    red_mask.set(0, 1);

    ShapeMask blue_mask;
    blue_mask.set(1, 0);
    blue_mask.set(1, 1);

    ShapeMask green_mask;
    green_mask.set(2, 0);

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    pixel_tile.set(1, 0, rgba_blue);
    pixel_tile.set(2, 0, rgba_green);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    auto red_index = *color_map.index_at_color(rgba_red);
    auto blue_index = *color_map.index_at_color(rgba_blue);
    auto green_index = *color_map.index_at_color(rgba_green);

    shape_tile.set(red_mask, red_index);
    shape_tile.set(blue_mask, blue_index);
    shape_tile.set(green_mask, green_index);

    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);

    EXPECT_EQ(result.colors().size(), 3);

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
    ShapeTile<ColorIndex> shape_tile;
    ColorIndexMap<Rgba32> color_map{};

    auto result = shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map);
    EXPECT_TRUE(result.colors().empty());
    EXPECT_TRUE(result.is_transparent());
}

TEST(TileConvertersTests, ShapeTileToPixelColorsPanicsWhenIndexNotInMap)
{
    ShapeTile<ColorIndex> shape_tile;
    ShapeMask mask;
    mask.set(0, 0);
    shape_tile.set(mask, ColorIndex{99});

    PixelTile<Rgba32> pixel_tile;
    pixel_tile.set(0, 0, rgba_red);
    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(pixel_tile, rgba_magenta);

    EXPECT_DEATH(
        { (void)shape_tile_to_pixel_colors<Rgba32>(shape_tile, color_map); }, "ColorIndex not found in ColorIndexMap");
}

TEST(TileConvertersTests, RoundTripPixelToShapeToPixel)
{
    PixelTile<Rgba32> original;
    original.set(0, 0, rgba_red);
    original.set(0, 1, rgba_red);
    original.set(1, 0, rgba_blue);
    original.set(1, 1, rgba_blue);
    original.set(2, 0, rgba_green);

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(original, rgba_magenta);

    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    EXPECT_TRUE(result.equals_ignoring_transparency(original, rgba_magenta));
}

TEST(TileConvertersTests, RoundTripWithTransparentPixels)
{
    PixelTile<Rgba32> original;
    original.set(0, 0, rgba_red);
    original.set(1, 1, rgba_blue);
    original.set(2, 2, rgba_green);

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(original, rgba_magenta);

    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);

    EXPECT_TRUE(result.equals_ignoring_transparency(original, rgba_magenta));
    EXPECT_EQ(result.at(0, 0), rgba_red);
    EXPECT_EQ(result.at(1, 1), rgba_blue);
    EXPECT_EQ(result.at(2, 2), rgba_green);
}

TEST(TileConvertersTests, RoundTripAllTransparent)
{
    PixelTile<Rgba32> original;

    ColorIndexMap<Rgba32> color_map{};
    color_map.add_tile(original, rgba_magenta);

    auto shape_tile = from_pixel_tile(original, color_map, rgba_magenta);
    auto result = from_shape_tile<Rgba32>(shape_tile, color_map);
    EXPECT_TRUE(result.is_transparent(rgba_magenta));
}

TEST(TileConvertersTests, ColorTileFromIndexTile_BasicConversion)
{
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

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_green);
    EXPECT_EQ(color_tile.at(1, 0), rgba_blue);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_MultipleDifferentColors)
{
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

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0), rgba_red);
    EXPECT_EQ(color_tile.at(5), rgba_green);
    EXPECT_EQ(color_tile.at(10), rgba_blue);
    EXPECT_EQ(color_tile.at(63), rgba_yellow);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_DuplicateIndices)
{
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{1});
    index_tile.set(0, 1, IndexPixel{1});
    index_tile.set(0, 2, IndexPixel{1});
    index_tile.set(1, 0, IndexPixel{2});
    index_tile.set(1, 1, IndexPixel{2});
    index_tile.set(2, 0, IndexPixel{1});

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0, 0), rgba_red);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_red);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
    EXPECT_EQ(color_tile.at(1, 1), rgba_green);
    EXPECT_EQ(color_tile.at(2, 0), rgba_red);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_CorrectColorMappingAllPixels)
{
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

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    const std::array<Rgba32, 4> expected_colors = {rgba_magenta, rgba_red, rgba_green, rgba_blue};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(color_tile.at(i), expected_colors[i % 4]);
    }
}

TEST(TileConvertersTests, ColorTileFromIndexTile_EmptyPalette_Panics)
{
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});

    Palette<Rgba32> palette{}; // Empty palette

    EXPECT_DEATH({ std::ignore = color_tile_from_index_tile(index_tile, palette, rgba_magenta); }, "palette is empty");
}

TEST(TileConvertersTests, ColorTileFromIndexTile_OutOfBoundsIndex_Panics)
{
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});
    index_tile.set(0, 1, IndexPixel{5}); // Out of bounds!

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    // Palette size is 3, but we're trying to access index 5

    EXPECT_DEATH(
        { std::ignore = color_tile_from_index_tile(index_tile, palette, rgba_magenta); },
        "index 5 out of palette bounds");
}

TEST(TileConvertersTests, ColorTileFromIndexTile_FullyPopulatedPalette)
{
    Palette<Rgba32> palette{};
    palette.add(rgba_magenta); // Index 0
    palette.add(rgba_red);     // Index 1
    palette.add(rgba_green);   // Index 2
    palette.add(rgba_blue);    // Index 3
    palette.add(rgba_yellow);  // Index 4

    // Fill remaining slots with generated colors
    for (std::size_t i = 5; i < palette::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 10),
                static_cast<std::uint8_t>(i * 15),
                static_cast<std::uint8_t>(i * 20)});
    }

    // Create a tile using all palette indices
    PixelTile<IndexPixel> index_tile{};
    for (std::size_t i = 0; i < palette::max_size; ++i) {
        index_tile.set(i, IndexPixel{i});
    }

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

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

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(0, 1), rgba_red);
    EXPECT_EQ(color_tile.at(0, 2), rgba_magenta);
    EXPECT_EQ(color_tile.at(1, 0), rgba_green);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_SingleColorPalette)
{
    Palette<Rgba32> palette{};
    palette.add(rgba_red);

    // Create a tile where all used pixels have index 0
    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, 0, IndexPixel{0});
    index_tile.set(1, 1, IndexPixel{0});
    index_tile.set(2, 2, IndexPixel{0});

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0, 0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1, 1), rgba_magenta);
    EXPECT_EQ(color_tile.at(2, 2), rgba_magenta);
}

TEST(TileConvertersTests, ColorTileFromIndexTile_BoundaryIndexValues)
{
    Palette<Rgba32> palette{};
    for (std::size_t i = 0; i < palette::max_size; ++i) {
        palette.add(
            Rgba32{
                static_cast<std::uint8_t>(i * 16),
                static_cast<std::uint8_t>(i * 16),
                static_cast<std::uint8_t>(i * 16)});
    }

    PixelTile<IndexPixel> index_tile{};
    index_tile.set(0, IndexPixel{0});                     // Minimum valid index
    index_tile.set(1, IndexPixel{palette::max_size - 1}); // Maximum valid index
    index_tile.set(2, IndexPixel{palette::max_size / 2}); // Middle index

    auto color_tile = color_tile_from_index_tile(index_tile, palette, rgba_magenta);

    EXPECT_EQ(color_tile.at(0), rgba_magenta);
    EXPECT_EQ(color_tile.at(1), (Rgba32{240, 240, 240}));
    EXPECT_EQ(color_tile.at(2), (Rgba32{128, 128, 128}));
}

TEST(TileConvertersTests, IndexTileFromColorTile_BasicConversion)
{
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

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0, 0).index(), 0);
    EXPECT_EQ(index_tile.at(0, 1).index(), 1);
    EXPECT_EQ(index_tile.at(0, 2).index(), 2);
    EXPECT_EQ(index_tile.at(1, 0).index(), 3);
}

TEST(TileConvertersTests, IndexTileFromColorTile_AllColorsFound)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_red);
    color_tile.set(1, rgba_green);
    color_tile.set(2, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0).index(), 1);
    EXPECT_EQ(index_tile.at(1).index(), 2);
    EXPECT_EQ(index_tile.at(2).index(), 3);
}

TEST(TileConvertersTests, IndexTileFromColorTile_ColorNotInPalette_Panics)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_blue); // NOT in palette

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);

    EXPECT_DEATH(
        { std::ignore = index_tile_from_color_tile(color_tile, palette, rgba_magenta); }, "color not found in palette");
}

TEST(TileConvertersTests, IndexTileFromColorTile_AllTransparent_IntrinsicAndExtrinsic)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, Rgba32{});           // Intrinsic transparency (alpha=0)
    color_tile.set(1, rgba_magenta);       // Extrinsic transparency
    color_tile.set(2, Rgba32{0, 0, 0, 0}); // Intrinsic transparency

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0).index(), 0);
    EXPECT_EQ(index_tile.at(1).index(), 0);
    EXPECT_EQ(index_tile.at(2).index(), 0);
}

TEST(TileConvertersTests, IndexTileFromColorTile_DuplicateColorsInTile)
{
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

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0).index(), 1);
    EXPECT_EQ(index_tile.at(1).index(), 1);
    EXPECT_EQ(index_tile.at(2).index(), 1);
    EXPECT_EQ(index_tile.at(3).index(), 2);
    EXPECT_EQ(index_tile.at(4).index(), 2);
}

TEST(TileConvertersTests, IndexTileFromColorTile_RoundTrip_IndexToColorToIndex)
{
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

    auto color_tile = color_tile_from_index_tile(original_index_tile, palette, rgba_magenta);
    auto result_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(result_tile.at(0).index(), 0);
    EXPECT_EQ(result_tile.at(1).index(), 1);
    EXPECT_EQ(result_tile.at(2).index(), 2);
    EXPECT_EQ(result_tile.at(3).index(), 3);
    EXPECT_EQ(result_tile.at(4).index(), 1);
}

TEST(TileConvertersTests, IndexTileFromColorTile_FullyPopulatedTile)
{
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

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(index_tile.at(i).index(), i % 4);
    }
}

TEST(TileConvertersTests, IndexTileFromColorTile_DuplicateColorsInPalette_UsesFirstOccurrence)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_red);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red); // First occurrence at index 1
    palette.add(rgba_green);
    palette.add(rgba_red); // Duplicate at index 3

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0).index(), 1);
}

TEST(TileConvertersTests, IndexTileFromColorTile_ExtrinsicTransparencyTreatedAsTransparent)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(0, rgba_magenta); // Extrinsic transparent
    color_tile.set(1, rgba_red);     // Opaque
    color_tile.set(2, rgba_magenta); // Extrinsic transparent

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(0).index(), 0);
    EXPECT_EQ(index_tile.at(1).index(), 1);
    EXPECT_EQ(index_tile.at(2).index(), 0);
}

TEST(TileConvertersTests, IndexTileFromColorTile_EmptyPalette_AllTransparentTile)
{
    PixelTile<Rgba32> color_tile{}; // All transparent by default

    Palette<Rgba32> palette{}; // Empty palette

    // Note: Empty palette is valid when tile has only transparent pixels
    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        EXPECT_EQ(index_tile.at(i).index(), 0);
    }
}

TEST(TileConvertersTests, IndexTileFromColorTile_SingleNonTransparentPixel)
{
    PixelTile<Rgba32> color_tile{};
    color_tile.set(30, rgba_blue);

    Palette<Rgba32> palette{};
    palette.add(rgba_magenta);
    palette.add(rgba_red);
    palette.add(rgba_green);
    palette.add(rgba_blue);

    auto index_tile = index_tile_from_color_tile(color_tile, palette, rgba_magenta);

    EXPECT_EQ(index_tile.at(30).index(), 3);
}
