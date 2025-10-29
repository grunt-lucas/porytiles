#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(ColorIndexMapTests, DefaultConstructedMapShouldBeEmpty)
{
    ColorIndexMap map{};

    EXPECT_TRUE(map.empty());
}

TEST(ColorIndexMapTests, ConstructorShouldBuildBidirectionalMapping)
{
    std::vector<PixelTile<Rgba32>> tiles;

    // Create a tile with 3 unique non-transparent colors
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue
    tile.set(3, Rgba32{255, 0, 0}); // red (duplicate)
    // Rest remain default (transparent)

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Should have 3 unique colors
    EXPECT_EQ(3, map.size());
}

TEST(ColorIndexMapTests, ForwardLookupShouldMapColorToIndex)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Each color should have an index
    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    auto green_index = map.index_at_color(Rgba32{0, 255, 0});
    auto blue_index = map.index_at_color(Rgba32{0, 0, 255});

    ASSERT_TRUE(red_index.has_value());
    ASSERT_TRUE(green_index.has_value());
    ASSERT_TRUE(blue_index.has_value());

    // Indices should be sequential (0, 1, 2) in some order
    EXPECT_LE(red_index.value(), 2);
    EXPECT_LE(green_index.value(), 2);
    EXPECT_LE(blue_index.value(), 2);

    // All indices should be different
    EXPECT_NE(red_index.value(), green_index.value());
    EXPECT_NE(red_index.value(), blue_index.value());
    EXPECT_NE(green_index.value(), blue_index.value());
}

TEST(ColorIndexMapTests, ReverseLookupShouldMapIndexToColor)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Each index should have a color
    auto color_at_0 = map.color_at_index(0);
    auto color_at_1 = map.color_at_index(1);
    auto color_at_2 = map.color_at_index(2);

    ASSERT_TRUE(color_at_0.has_value());
    ASSERT_TRUE(color_at_1.has_value());
    ASSERT_TRUE(color_at_2.has_value());

    // Colors should be one of the three we inserted
    const Rgba32 red{255, 0, 0};
    const Rgba32 green{0, 255, 0};
    const Rgba32 blue{0, 0, 255};

    EXPECT_TRUE(color_at_0.value() == red || color_at_0.value() == green || color_at_0.value() == blue);
    EXPECT_TRUE(color_at_1.value() == red || color_at_1.value() == green || color_at_1.value() == blue);
    EXPECT_TRUE(color_at_2.value() == red || color_at_2.value() == green || color_at_2.value() == blue);

    // All colors should be different
    EXPECT_NE(color_at_0.value(), color_at_1.value());
    EXPECT_NE(color_at_0.value(), color_at_2.value());
    EXPECT_NE(color_at_1.value(), color_at_2.value());
}

TEST(ColorIndexMapTests, BidirectionalLookupShouldBeConsistent)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    const Rgba32 red{255, 0, 0};
    const Rgba32 green{0, 255, 0};
    const Rgba32 blue{0, 0, 255};

    // Forward lookup: color -> index
    auto red_index = map.index_at_color(red);
    auto green_index = map.index_at_color(green);
    auto blue_index = map.index_at_color(blue);

    ASSERT_TRUE(red_index.has_value());
    ASSERT_TRUE(green_index.has_value());
    ASSERT_TRUE(blue_index.has_value());

    // Reverse lookup: index -> color
    auto color_at_red_index = map.color_at_index(red_index.value());
    auto color_at_green_index = map.color_at_index(green_index.value());
    auto color_at_blue_index = map.color_at_index(blue_index.value());

    ASSERT_TRUE(color_at_red_index.has_value());
    ASSERT_TRUE(color_at_green_index.has_value());
    ASSERT_TRUE(color_at_blue_index.has_value());

    // Bidirectional lookup should be consistent
    EXPECT_EQ(red, color_at_red_index.value());
    EXPECT_EQ(green, color_at_green_index.value());
    EXPECT_EQ(blue, color_at_blue_index.value());
}

TEST(ColorIndexMapTests, LookupNonexistentColorShouldReturnNullopt)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Yellow was never added
    auto yellow_index = map.index_at_color(Rgba32{255, 255, 0});
    EXPECT_FALSE(yellow_index.has_value());
}

TEST(ColorIndexMapTests, LookupNonexistentIndexShouldReturnNullopt)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Should only have index 0, not 99
    auto color_at_99 = map.color_at_index(99);
    EXPECT_FALSE(color_at_99.has_value());
}

TEST(ColorIndexMapTests, TransparentColorsShouldBeFiltered)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0});                          // red (non-transparent)
    tile.set(1, Rgba32{255, 0, 255});                        // magenta (extrinsically transparent)
    tile.set(2, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // black with alpha=0 (intrinsically transparent)

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Should only have red (index 0)
    EXPECT_EQ(1, map.size());

    // Red should be present
    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    ASSERT_TRUE(red_index.has_value());
    EXPECT_EQ(0, red_index.value());

    // Magenta and transparent black should not be present
    EXPECT_FALSE(map.index_at_color(Rgba32{255, 0, 255}).has_value());
    EXPECT_FALSE(map.index_at_color(Rgba32{0, 0, 0, Rgba32::alpha_transparent}).has_value());
}

TEST(ColorIndexMapTests, DeduplicationAcrossMultipleTiles)
{
    std::vector<PixelTile<Rgba32>> tiles;

    // Tile 1 with red and green
    PixelTile<Rgba32> tile1{};
    tile1.set(0, Rgba32{255, 0, 0}); // red
    tile1.set(1, Rgba32{0, 255, 0}); // green

    // Tile 2 with green and blue (green is duplicate)
    PixelTile<Rgba32> tile2{};
    tile2.set(0, Rgba32{0, 255, 0}); // green (duplicate)
    tile2.set(1, Rgba32{0, 0, 255}); // blue

    tiles.push_back(tile1);
    tiles.push_back(tile2);

    ColorIndexMap map{tiles, rgba_magenta};

    // Should have 3 unique colors (red, green, blue)
    EXPECT_EQ(3, map.size());

    // Green should have only one index
    const Rgba32 expected_green{0, 255, 0};
    auto green_index = map.index_at_color(expected_green);
    ASSERT_TRUE(green_index.has_value());

    // Looking up that index should return green
    auto color = map.color_at_index(green_index.value());
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(expected_green, color.value());
}

TEST(ColorIndexMapTests, SequentialIndicesStartAtZero)
{
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    tiles.push_back(tile);

    ColorIndexMap map{tiles, rgba_magenta};

    // Should have indices 0, 1, 2, 3
    EXPECT_TRUE(map.color_at_index(0).has_value());
    EXPECT_TRUE(map.color_at_index(1).has_value());
    EXPECT_TRUE(map.color_at_index(2).has_value());
    EXPECT_FALSE(map.color_at_index(3).has_value());
}
