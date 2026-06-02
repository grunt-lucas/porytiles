#include <gtest/gtest.h>

#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/color_index.hpp"
#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;

TEST(ColorIndexMapTests, DefaultIsEmpty)
{
    ColorIndexMap<Rgba32> map{};

    EXPECT_TRUE(map.empty());
}

TEST(ColorIndexMapTests, AddTileBuildsBidirectionalMapping)
{
    // Create a tile with 3 unique non-transparent colors
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue
    tile.set(3, Rgba32{255, 0, 0}); // red (duplicate)
    // Rest remain default (transparent)

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

    EXPECT_EQ(3, map.size());
}

TEST(ColorIndexMapTests, ForwardLookupMapsColorToIndex)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

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

    EXPECT_NE(red_index.value(), green_index.value());
    EXPECT_NE(red_index.value(), blue_index.value());
    EXPECT_NE(green_index.value(), blue_index.value());
}

TEST(ColorIndexMapTests, ReverseLookupMapsIndexToColor)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

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

    EXPECT_NE(color_at_0.value(), color_at_1.value());
    EXPECT_NE(color_at_0.value(), color_at_2.value());
    EXPECT_NE(color_at_1.value(), color_at_2.value());
}

TEST(ColorIndexMapTests, BidirectionalLookupConsistent)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

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

TEST(ColorIndexMapTests, LookupNonexistentColorReturnsNullopt)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

    // Yellow was never added
    auto yellow_index = map.index_at_color(Rgba32{255, 255, 0});
    EXPECT_FALSE(yellow_index.has_value());
}

TEST(ColorIndexMapTests, LookupNonexistentIndexReturnsNullopt)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

    // Should only have index 0, not 99
    auto color_at_99 = map.color_at_index(ColorIndex{99});
    EXPECT_FALSE(color_at_99.has_value());
}

TEST(ColorIndexMapTests, TransparentColorsFiltered)
{
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0});                          // red (non-transparent)
    tile.set(1, Rgba32{255, 0, 255});                        // magenta (extrinsically transparent)
    tile.set(2, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // black with alpha=0 (intrinsically transparent)

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

    EXPECT_EQ(1, map.size());

    auto red_index = map.index_at_color(Rgba32{255, 0, 0});
    ASSERT_TRUE(red_index.has_value());
    EXPECT_EQ(0, red_index.value().index());

    // Magenta and transparent black should not be present
    EXPECT_FALSE(map.index_at_color(Rgba32{255, 0, 255}).has_value());
    EXPECT_FALSE(map.index_at_color(Rgba32{0, 0, 0, Rgba32::alpha_transparent}).has_value());
}

TEST(ColorIndexMapTests, DeduplicationAcrossMultipleTiles)
{
    // Tile 1 with red and green
    PixelTile<Rgba32> tile1{};
    tile1.set(0, Rgba32{255, 0, 0}); // red
    tile1.set(1, Rgba32{0, 255, 0}); // green

    // Tile 2 with green and blue (green is duplicate)
    PixelTile<Rgba32> tile2{};
    tile2.set(0, Rgba32{0, 255, 0}); // green (duplicate)
    tile2.set(1, Rgba32{0, 0, 255}); // blue

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile1, rgba_magenta);
    map.add_tile(tile2, rgba_magenta);

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
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue

    ColorIndexMap<Rgba32> map{};
    map.add_tile(tile, rgba_magenta);

    // Should have indices 0, 1, 2, 3
    EXPECT_TRUE(map.color_at_index(ColorIndex{0}).has_value());
    EXPECT_TRUE(map.color_at_index(ColorIndex{1}).has_value());
    EXPECT_TRUE(map.color_at_index(ColorIndex{2}).has_value());
    EXPECT_FALSE(map.color_at_index(ColorIndex{3}).has_value());
}

TEST(ColorIndexMapTests, AddTileAddsColors)
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

TEST(ColorIndexMapTests, AddTileDeduplicatesColors)
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

TEST(ColorIndexMapTests, AddTileFiltersTransparent)
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

TEST(ColorIndexMapTests, AddTileAssignsSequentialIndices)
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

TEST(ColorIndexMapTests, AddPalFixedAddsColors)
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

TEST(ColorIndexMapTests, AddPalFixedSkipsWildcards)
{
    ColorIndexMap<Rgba32> map{};

    Palette<Rgba32, pal::max_size> pal{};
    pal.set(0, Rgba32{255, 0, 255}); // magenta (transparent slot 0)
    pal.set(1, Rgba32{255, 0, 0});   // red
    // slots 2-15 are wildcards

    map.add_pal(pal, rgba_magenta);

    EXPECT_EQ(1, map.size()); // only red
}

TEST(ColorIndexMapTests, AddPalFixedDeduplicates)
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

TEST(ColorIndexMapTests, AddPalDynamicAddsColors)
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

TEST(ColorIndexMapTests, AddPalDynamicSkipsWildcards)
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

TEST(ColorIndexMapTests, AddPalDynamicDeduplicates)
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

TEST(ColorIndexMapTests, AddTileAndPalDeduplicateAcross)
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

TEST(ColorIndexMapTests, AddMethodsMaintainBidirectionalConsistency)
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

TEST(ColorIndexMapTests, AddAnimKeyFrameOnly)
{
    ColorIndexMap<Rgba32> map{};

    // Create an animation with only a key frame
    Animation<Rgba32> anim{"test_anim"};

    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0}); // red
    key_tile.set(1, Rgba32{0, 255, 0}); // green

    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(2, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddAnimRegularFramesOnly)
{
    ColorIndexMap<Rgba32> map{};

    // Create an animation with only regular frames (no key frame)
    Animation<Rgba32> anim{"test_anim"};

    PixelTile<Rgba32> frame0_tile{};
    frame0_tile.set(0, Rgba32{255, 0, 0}); // red

    PixelTile<Rgba32> frame1_tile{};
    frame1_tile.set(0, Rgba32{0, 255, 0}); // green

    AnimFrame<Rgba32> frame0{"0"};
    frame0.add_tile(std::move(frame0_tile));
    anim.put_frame("0", std::move(frame0));

    AnimFrame<Rgba32> frame1{"1"};
    frame1.add_tile(std::move(frame1_tile));
    anim.put_frame("1", std::move(frame1));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(2, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddAnimKeyAndRegularFrames)
{
    ColorIndexMap<Rgba32> map{};

    // Create an animation with key frame and regular frames
    Animation<Rgba32> anim{"test_anim"};

    // Key frame with red
    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0}); // red
    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    // Frame 0 with green
    PixelTile<Rgba32> frame0_tile{};
    frame0_tile.set(0, Rgba32{0, 255, 0}); // green
    AnimFrame<Rgba32> frame0{"0"};
    frame0.add_tile(std::move(frame0_tile));
    anim.put_frame("0", std::move(frame0));

    // Frame 1 with blue
    PixelTile<Rgba32> frame1_tile{};
    frame1_tile.set(0, Rgba32{0, 0, 255}); // blue
    AnimFrame<Rgba32> frame1{"1"};
    frame1.add_tile(std::move(frame1_tile));
    anim.put_frame("1", std::move(frame1));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(3, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value()); // red from key
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value()); // green from frame 0
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 0, 255}).has_value()); // blue from frame 1
}

TEST(ColorIndexMapTests, AddAnimDeduplicatesAcrossFrames)
{
    ColorIndexMap<Rgba32> map{};

    Animation<Rgba32> anim{"test_anim"};

    // Key frame with red and green
    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0}); // red
    key_tile.set(1, Rgba32{0, 255, 0}); // green
    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    // Frame 0 with red (duplicate) and blue
    PixelTile<Rgba32> frame0_tile{};
    frame0_tile.set(0, Rgba32{255, 0, 0}); // red (duplicate)
    frame0_tile.set(1, Rgba32{0, 0, 255}); // blue
    AnimFrame<Rgba32> frame0{"0"};
    frame0.add_tile(std::move(frame0_tile));
    anim.put_frame("0", std::move(frame0));

    // Frame 1 with green (duplicate)
    PixelTile<Rgba32> frame1_tile{};
    frame1_tile.set(0, Rgba32{0, 255, 0}); // green (duplicate)
    AnimFrame<Rgba32> frame1{"1"};
    frame1.add_tile(std::move(frame1_tile));
    anim.put_frame("1", std::move(frame1));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(3, map.size()); // red, green, blue (no duplicates)
}

TEST(ColorIndexMapTests, AddAnimFiltersTransparent)
{
    ColorIndexMap<Rgba32> map{};

    Animation<Rgba32> anim{"test_anim"};

    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0});                          // red (non-transparent)
    key_tile.set(1, Rgba32{255, 0, 255});                        // magenta (extrinsically transparent)
    key_tile.set(2, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // black with alpha=0 (intrinsically transparent)
    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(1, map.size());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_FALSE(map.index_at_color(Rgba32{255, 0, 255}).has_value());
    EXPECT_FALSE(map.index_at_color(Rgba32{0, 0, 0, Rgba32::alpha_transparent}).has_value());
}

TEST(ColorIndexMapTests, AddAnimAllSubtiles)
{
    ColorIndexMap<Rgba32> map{};

    Animation<Rgba32> anim{"test_anim"};

    // Key frame with two subtiles
    PixelTile<Rgba32> key_tile0{};
    key_tile0.set(0, Rgba32{255, 0, 0}); // red
    PixelTile<Rgba32> key_tile1{};
    key_tile1.set(0, Rgba32{0, 255, 0}); // green

    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile0));
    key_frame.add_tile(std::move(key_tile1));
    anim.key_frame(std::move(key_frame));

    // Frame 0 with two subtiles
    PixelTile<Rgba32> frame0_tile0{};
    frame0_tile0.set(0, Rgba32{0, 0, 255}); // blue
    PixelTile<Rgba32> frame0_tile1{};
    frame0_tile1.set(0, Rgba32{255, 255, 0}); // yellow

    AnimFrame<Rgba32> frame0{"0"};
    frame0.add_tile(std::move(frame0_tile0));
    frame0.add_tile(std::move(frame0_tile1));
    anim.put_frame("0", std::move(frame0));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(4, map.size()); // red, green, blue, yellow
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 0, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 255, 0}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{0, 0, 255}).has_value());
    EXPECT_TRUE(map.index_at_color(Rgba32{255, 255, 0}).has_value());
}

TEST(ColorIndexMapTests, AddAnimEmptyNoColors)
{
    ColorIndexMap<Rgba32> map{};

    Animation<Rgba32> anim{"empty_anim"};

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(0, map.size());
    EXPECT_TRUE(map.empty());
}

TEST(ColorIndexMapTests, AddAnimAndTileDeduplicateAcross)
{
    ColorIndexMap<Rgba32> map{};

    // Add a tile with red
    PixelTile<Rgba32> tile{};
    tile.set(0, Rgba32{255, 0, 0}); // red
    map.add_tile(tile, rgba_magenta);

    // Add an animation with red (duplicate) and green
    Animation<Rgba32> anim{"test_anim"};
    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0}); // red (duplicate)
    key_tile.set(1, Rgba32{0, 255, 0}); // green
    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and green
}

TEST(ColorIndexMapTests, AddAnimAndPalDeduplicateAcross)
{
    ColorIndexMap<Rgba32> map{};

    // Add a palette with red
    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 255}); // transparent
    pal.add(Rgba32{255, 0, 0});   // red
    map.add_pal(pal, rgba_magenta);

    // Add an animation with red (duplicate) and blue
    Animation<Rgba32> anim{"test_anim"};
    PixelTile<Rgba32> key_tile{};
    key_tile.set(0, Rgba32{255, 0, 0}); // red (duplicate)
    key_tile.set(1, Rgba32{0, 0, 255}); // blue
    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(std::move(key_tile));
    anim.key_frame(std::move(key_frame));

    map.add_anim(anim, rgba_magenta);

    EXPECT_EQ(2, map.size()); // red and blue
}
