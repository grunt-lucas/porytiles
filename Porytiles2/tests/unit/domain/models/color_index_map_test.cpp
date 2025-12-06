#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_index.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(ColorIndexMapTests, DefaultConstructedMapShouldBeEmpty)
{
    ColorIndexMap<Rgba32> map{};

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

    // Each color should have an index
    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    auto green_index = map.index_at_color(Rgba32{0, 255, 0});
    auto blue_index = map.index_at_color(Rgba32{0, 0, 255});

    ASSERT_TRUE(red_index.has_value());
    ASSERT_TRUE(green_index.has_value());
    ASSERT_TRUE(blue_index.has_value());

    // Indices should be sequential (0, 1, 2) in some order
    EXPECT_LE(red_index.value().index(), 2);
    EXPECT_LE(green_index.value().index(), 2);
    EXPECT_LE(blue_index.value().index(), 2);

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

    // Each index should have a color
    auto color_at_0 = map.color_at_index(ColorIndex{0});
    auto color_at_1 = map.color_at_index(ColorIndex{1});
    auto color_at_2 = map.color_at_index(ColorIndex{2});

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

    // Should only have index 0, not 99
    auto color_at_99 = map.color_at_index(ColorIndex{99});
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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

    // Should only have red (index 0)
    EXPECT_EQ(1, map.size());

    // Red should be present
    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    ASSERT_TRUE(red_index.has_value());
    EXPECT_EQ(0, red_index.value().index());

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

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

    ColorIndexMap<Rgba32> map{tiles, rgba_magenta};

    // Should have indices 0, 1, 2, 3
    EXPECT_TRUE(map.color_at_index(ColorIndex{0}).has_value());
    EXPECT_TRUE(map.color_at_index(ColorIndex{1}).has_value());
    EXPECT_TRUE(map.color_at_index(ColorIndex{2}).has_value());
    EXPECT_FALSE(map.color_at_index(ColorIndex{3}).has_value());
}

// ============================================================================
// Constructor equivalence tests
// ============================================================================

TEST(ColorIndexMapTests, VectorCtorShouldMatchIncrementalAddTile)
{
    // Create multiple tiles with various colors
    std::vector<PixelTile<Rgba32>> tiles;

    PixelTile<Rgba32> tile1{};
    tile1.set(0, Rgba32{255, 0, 0});   // red
    tile1.set(1, Rgba32{0, 255, 0});   // green
    tile1.set(2, Rgba32{255, 0, 255}); // magenta (transparent)

    PixelTile<Rgba32> tile2{};
    tile2.set(0, Rgba32{0, 0, 255});   // blue
    tile2.set(1, Rgba32{255, 0, 0});   // red (duplicate from tile1)
    tile2.set(2, Rgba32{255, 255, 0}); // yellow

    PixelTile<Rgba32> tile3{};
    tile3.set(0, Rgba32{0, 255, 255});                        // cyan
    tile3.set(1, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent

    tiles.push_back(tile1);
    tiles.push_back(tile2);
    tiles.push_back(tile3);

    // Construct using vector constructor
    ColorIndexMap<Rgba32> ctor_map{tiles, rgba_magenta};

    // Construct incrementally using add_tile
    ColorIndexMap<Rgba32> incremental_map{};
    for (const auto &tile : tiles) {
        incremental_map.add_tile(tile, rgba_magenta);
    }

    // Both maps should have the same size
    EXPECT_EQ(ctor_map.size(), incremental_map.size());

    // Expected colors: red, green, blue, yellow, cyan (5 total)
    // Transparent colors (magenta, alpha=0) should be filtered out
    EXPECT_EQ(5, ctor_map.size());

    // Verify all colors have consistent mappings in both maps
    const std::vector<Rgba32> expected_colors = {
        Rgba32{255, 0, 0},   // red
        Rgba32{0, 255, 0},   // green
        Rgba32{0, 0, 255},   // blue
        Rgba32{255, 255, 0}, // yellow
        Rgba32{0, 255, 255}  // cyan
    };

    for (const auto &color : expected_colors) {
        auto ctor_index = ctor_map.index_at_color(color);
        auto incr_index = incremental_map.index_at_color(color);

        ASSERT_TRUE(ctor_index.has_value()) << "ctor_map missing color";
        ASSERT_TRUE(incr_index.has_value()) << "incremental_map missing color";

        // Both should map to the same index
        EXPECT_EQ(ctor_index.value(), incr_index.value());

        // Reverse lookup should also match
        auto ctor_color = ctor_map.color_at_index(ctor_index.value());
        auto incr_color = incremental_map.color_at_index(incr_index.value());

        ASSERT_TRUE(ctor_color.has_value());
        ASSERT_TRUE(incr_color.has_value());
        EXPECT_EQ(ctor_color.value(), incr_color.value());
    }
}

// ============================================================================
// add_tile tests
// ============================================================================

TEST(ColorIndexMapTests, AddTileShouldAddColorsFromTile)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green

    map.add_tile(tile, rgba_magenta);

    EXPECT_EQ(2, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddTileShouldDeduplicateColors)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile1{};
    tile1.set(0, Rgba32{255, 0, 0}); // red

    PixelTile<Rgba32> tile2{};
    tile2.set(0, Rgba32{255, 0, 0}); // red (duplicate)
    tile2.set(1, Rgba32{0, 255, 0}); // green

    map.add_tile(tile1, rgba_magenta);
    map.add_tile(tile2, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and green, no duplicates
}

TEST(ColorIndexMapTests, AddTileShouldFilterTransparentColors)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0});                          // red (non-transparent)
    tile.set(1, Rgba32{255, 0, 255});                        // magenta (extrinsically transparent)
    tile.set(2, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // black with alpha=0 (intrinsically transparent)

    map.add_tile(tile, rgba_magenta);

    EXPECT_EQ(1, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_FALSE(map.index_at_color(Rgba32{255, 0, 255}).has_value());
}

TEST(ColorIndexMapTests, AddTileShouldAssignSequentialIndices)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile1{};
    tile1.set(0, Rgba32{255, 0, 0}); // red

    PixelTile<Rgba32> tile2{};
    tile2.set(0, Rgba32{0, 255, 0}); // green

    map.add_tile(tile1, rgba_magenta);
    map.add_tile(tile2, rgba_magenta);

    // First tile gets index 0, second gets index 1
    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    auto green_index = map.index_at_color(Rgba32{0, 255, 0});

    ASSERT_TRUE(red_index.has_value());
    ASSERT_TRUE(green_index.has_value());
    EXPECT_EQ(0, red_index.value().index());
    EXPECT_EQ(1, green_index.value().index());
}

// ============================================================================
// add_pal tests (fixed-size palette)
// ============================================================================

TEST(ColorIndexMapTests, AddPalFixedSizeShouldAddColorsFromPalette)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32, pal::max_size> pal{};
    pal.set(0, Rgba32{255, 0, 255}); // magenta (transparent slot 0)
    pal.set(1, Rgba32{255, 0, 0});   // red
    pal.set(2, Rgba32{0, 255, 0});   // green

    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and green (magenta is transparent)
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddPalFixedSizeShouldSkipWildcards)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32, pal::max_size> pal{};
    pal.set(0, Rgba32{255, 0, 255}); // magenta (transparent slot 0)
    pal.set(1, Rgba32{255, 0, 0});   // red
    // slots 2-15 are wildcards

    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(1, map.size()); // only red
}

TEST(ColorIndexMapTests, AddPalFixedSizeShouldDeduplicateColors)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32, pal::max_size> pal1{};
    pal1.set(0, Rgba32{255, 0, 255}); // transparent
    pal1.set(1, Rgba32{255, 0, 0});   // red

    Palette<Rgba32, pal::max_size> pal2{};
    pal2.set(0, Rgba32{255, 0, 255}); // transparent
    pal2.set(1, Rgba32{255, 0, 0});   // red (duplicate)
    pal2.set(2, Rgba32{0, 0, 255});   // blue

    map.add_pal(pal1, rgba_magenta);
    map.add_pal(pal2, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and blue
}

// ============================================================================
// add_pal tests (dynamic palette)
// ============================================================================

TEST(ColorIndexMapTests, AddPalDynamicShouldAddColorsFromPalette)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 255}); // magenta (transparent slot 0)
    pal.add(Rgba32{255, 0, 0});   // red
    pal.add(Rgba32{0, 255, 0});   // green

    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and green (magenta is transparent)
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddPalDynamicShouldSkipWildcards)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 255}); // magenta (transparent)
    pal.add(Rgba32{255, 0, 0});   // red
    pal.add_wildcard();           // wildcard
    pal.add(Rgba32{0, 0, 255});   // blue

    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and blue
}

TEST(ColorIndexMapTests, AddPalDynamicShouldDeduplicateColors)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32> pal1{};
    pal1.add(Rgba32{255, 0, 255}); // transparent
    pal1.add(Rgba32{255, 0, 0});   // red

    Palette<Rgba32> pal2{};
    pal2.add(Rgba32{255, 0, 255}); // transparent
    pal2.add(Rgba32{255, 0, 0});   // red (duplicate)
    pal2.add(Rgba32{0, 0, 255});   // blue

    map.add_pal(pal1, rgba_magenta);
    map.add_pal(pal2, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and blue
}

// ============================================================================
// Combined add tests
// ============================================================================

TEST(ColorIndexMapTests, AddTileAndAddPalShouldDeduplicateAcrossBoth)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green

    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 255}); // transparent
    pal.add(Rgba32{255, 0, 0});   // red (duplicate from tile)
    pal.add(Rgba32{0, 0, 255});   // blue

    map.add_tile(tile, rgba_magenta);
    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(3, map.size()); // red, green, blue
}

TEST(ColorIndexMapTests, AddMethodsShouldMaintainBidirectionalConsistency)
{
    ColorIndexMap<Rgba32> map{};

    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red

    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 255}); // transparent
    pal.add(Rgba32{0, 255, 0});   // green

    map.add_tile(tile, rgba_magenta);
    map.add_pal(pal, rgba_magenta);

    const Rgba32 red{255, 0, 0};
    const Rgba32 green{0, 255, 0};

    // Forward lookup
    auto red_index = map.index_at_color(red);
    auto green_index = map.index_at_color(green);

    ASSERT_TRUE(red_index.has_value());
    ASSERT_TRUE(green_index.has_value());

    // Reverse lookup
    auto color_at_red_index = map.color_at_index(red_index.value());
    auto color_at_green_index = map.color_at_index(green_index.value());

    ASSERT_TRUE(color_at_red_index.has_value());
    ASSERT_TRUE(color_at_green_index.has_value());

    // Consistency check
    EXPECT_EQ(red, color_at_red_index.value());
    EXPECT_EQ(green, color_at_green_index.value());
}
