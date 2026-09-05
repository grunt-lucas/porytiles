#include "gtest/gtest.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;

namespace {

constexpr Rgba32 stray{234, 21, 97};
constexpr Rgba32 near_stray{236, 20, 99};
constexpr Rgba32 transparent_stray{234, 21, 97, Rgba32::alpha_transparent};

PixelTile<Rgba32> solid_tile(const Rgba32 &color)
{
    return PixelTile<Rgba32>{color};
}

Metatile<Rgba32> solid_metatile(const Rgba32 &color)
{
    Metatile<Rgba32> metatile{};
    for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
        metatile.set_bottom(i, solid_tile(color));
        metatile.set_middle(i, solid_tile(color));
        metatile.set_top(i, solid_tile(color));
    }
    return metatile;
}

} // namespace

TEST(ColorSearchTests, RgbWithinToleranceIgnoresAlpha)
{
    EXPECT_TRUE(rgb_within_tolerance(stray, transparent_stray, 0));
    EXPECT_TRUE(rgb_within_tolerance(stray, near_stray, 2));
    EXPECT_FALSE(rgb_within_tolerance(stray, near_stray, 1));
    EXPECT_TRUE(rgb_within_tolerance(rgba_black, rgba_white, 255));
}

TEST(ColorSearchTests, RgbGbaEquivalentUsesFiveBitBuckets)
{
    // 0 through 7 downconvert to 0; 8 starts the next bucket.
    EXPECT_TRUE(rgb_gba_equivalent(Rgba32{0, 8, 16}, Rgba32{7, 15, 23}));
    EXPECT_FALSE(rgb_gba_equivalent(Rgba32{7, 0, 0}, Rgba32{8, 0, 0}));
    EXPECT_FALSE(rgb_gba_equivalent(Rgba32{0, 7, 0}, Rgba32{0, 8, 0}));
    EXPECT_FALSE(rgb_gba_equivalent(Rgba32{0, 0, 7}, Rgba32{0, 0, 8}));
    // Alpha is ignored.
    EXPECT_TRUE(rgb_gba_equivalent(stray, transparent_stray));
    // 248 through 255 all become 31.
    EXPECT_TRUE(rgb_gba_equivalent(rgba_white, Rgba32{248, 248, 248}));
}

TEST(ColorSearchTests, ColorToleranceRules)
{
    EXPECT_TRUE(ColorTolerance::exact().is_exact());
    EXPECT_FALSE(ColorTolerance::per_channel(1).is_exact());
    EXPECT_FALSE(ColorTolerance::gba().is_exact());
    EXPECT_EQ(ColorTolerance::gba().rule(), ColorTolerance::Rule::gba);
    EXPECT_EQ(ColorTolerance::per_channel(5).steps(), 5);
    EXPECT_EQ(ColorTolerance::exact(), ColorTolerance::per_channel(0));

    EXPECT_FALSE(ColorTolerance::exact().similar(stray, near_stray));
    EXPECT_TRUE(ColorTolerance::per_channel(2).similar(stray, near_stray));
    // 234 and 236 share bucket 29, 21 and 20 share bucket 2, 97 and 99 share bucket 12.
    EXPECT_TRUE(ColorTolerance::gba().similar(stray, near_stray));
    // 239 and 240 differ by 1 but sit in buckets 29 and 30.
    EXPECT_FALSE(ColorTolerance::gba().similar(Rgba32{239, 0, 0}, Rgba32{240, 0, 0}));
    EXPECT_TRUE(ColorTolerance::per_channel(1).similar(Rgba32{239, 0, 0}, Rgba32{240, 0, 0}));
}

TEST(ColorSearchTests, ParseColorTolerance)
{
    EXPECT_EQ(parse_color_tolerance("gba").value(), ColorTolerance::gba());
    EXPECT_EQ(parse_color_tolerance("0").value(), ColorTolerance::exact());
    EXPECT_EQ(parse_color_tolerance("255").value(), ColorTolerance::per_channel(255));

    for (const auto *bad : {"", "GBA", "256", "-1", "8x", "1.5"}) {
        const auto parsed = parse_color_tolerance(bad);
        ASSERT_FALSE(parsed.has_value()) << bad;
        EXPECT_NE(parsed.error().find("must be an integer in 0-255 or 'gba'"), std::string::npos);
    }
}

TEST(ColorSearchTests, MatcherSkipsAlphaZeroPixels)
{
    const ColorMatcher exact{stray, ColorTolerance::exact()};
    EXPECT_TRUE(exact.matches(stray));
    EXPECT_FALSE(exact.matches(transparent_stray));
    EXPECT_FALSE(exact.matches(near_stray));

    const ColorMatcher loose{stray, ColorTolerance::per_channel(2)};
    EXPECT_TRUE(loose.matches(near_stray));
    EXPECT_FALSE(loose.matches(transparent_stray));

    const ColorMatcher gba{stray, ColorTolerance::gba()};
    EXPECT_TRUE(gba.matches(near_stray));
    EXPECT_FALSE(gba.matches(transparent_stray));
}

TEST(ColorSearchTests, FindColorInMetatilesReportsLayerAndMetatileLocalCoords)
{
    std::vector<Metatile<Rgba32>> metatiles{solid_metatile(rgba_white), solid_metatile(rgba_white)};

    // Southeast subtile of the middle layer, subtile-local (2, 3), which is metatile-local (10, 11).
    auto tile = solid_tile(rgba_white);
    tile.set(2, 3, stray);
    metatiles.at(1).set_middle(static_cast<std::size_t>(metatile::Subtile::southeast), tile);
    // Northwest subtile of the top layer of the same metatile, at (0, 0).
    auto top_tile = solid_tile(rgba_white);
    top_tile.set(0, 0, stray);
    metatiles.at(1).set_top(static_cast<std::size_t>(metatile::Subtile::northwest), top_tile);

    const auto matches = find_color_in_metatiles(metatiles, ColorMatcher{stray, ColorTolerance::exact()});

    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(matches.at(0).metatile_index, 1);
    EXPECT_EQ(matches.at(0).layer, metatile::Layer::middle);
    EXPECT_EQ(matches.at(0).pixel_coords, (std::set<std::pair<std::size_t, std::size_t>>{{10, 11}}));
    EXPECT_EQ(matches.at(1).metatile_index, 1);
    EXPECT_EQ(matches.at(1).layer, metatile::Layer::top);
    EXPECT_EQ(matches.at(1).pixel_coords, (std::set<std::pair<std::size_t, std::size_t>>{{0, 0}}));
}

TEST(ColorSearchTests, FindColorInMetatilesNoMatches)
{
    const std::vector<Metatile<Rgba32>> metatiles{solid_metatile(rgba_white)};
    EXPECT_TRUE(find_color_in_metatiles(metatiles, ColorMatcher{stray, ColorTolerance::exact()}).empty());
}

TEST(ColorSearchTests, FindColorInMetatilesWithTolerance)
{
    std::vector<Metatile<Rgba32>> metatiles{solid_metatile(rgba_white)};
    auto tile = solid_tile(rgba_white);
    tile.set(7, 7, near_stray);
    metatiles.at(0).set_bottom(static_cast<std::size_t>(metatile::Subtile::northeast), tile);

    EXPECT_TRUE(find_color_in_metatiles(metatiles, ColorMatcher{stray, ColorTolerance::exact()}).empty());

    const auto matches = find_color_in_metatiles(metatiles, ColorMatcher{stray, ColorTolerance::per_channel(2)});
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches.at(0).layer, metatile::Layer::bottom);
    EXPECT_EQ(matches.at(0).pixel_coords, (std::set<std::pair<std::size_t, std::size_t>>{{7, 15}}));
}

TEST(ColorSearchTests, FindColorInAnimsVisitsKeyFrameThenFramesByName)
{
    auto marked = solid_tile(rgba_white);
    marked.set(1, 2, stray);
    marked.set(1, 3, stray);

    Animation<Rgba32> flower{"flower"};
    flower.key_frame(AnimFrame<Rgba32>{"key", {solid_tile(rgba_white), marked}});
    flower.put_frame("01", AnimFrame<Rgba32>{"01", {marked}});
    flower.put_frame("00", AnimFrame<Rgba32>{"00", {solid_tile(rgba_white)}});
    Animation<Rgba32> water{"water"};
    water.put_frame("00", AnimFrame<Rgba32>{"00", {marked}});

    std::map<std::string, Animation<Rgba32>> anims{};
    anims.emplace("water", water);
    anims.emplace("flower", flower);

    const auto matches = find_color_in_anims(anims, ColorMatcher{stray, ColorTolerance::exact()});

    ASSERT_EQ(matches.size(), 3);
    EXPECT_EQ(matches.at(0).anim_name, "flower");
    EXPECT_EQ(matches.at(0).frame_name, "key");
    EXPECT_EQ(matches.at(0).tile_index, 1);
    EXPECT_EQ(matches.at(0).pixel_indexes, (std::vector<std::size_t>{10, 11}));
    EXPECT_EQ(matches.at(1).anim_name, "flower");
    EXPECT_EQ(matches.at(1).frame_name, "01");
    EXPECT_EQ(matches.at(1).tile_index, 0);
    EXPECT_EQ(matches.at(2).anim_name, "water");
    EXPECT_EQ(matches.at(2).frame_name, "00");
}

TEST(ColorSearchTests, CountTilesetColorsSkipsTransparentAndIncludesAnims)
{
    constexpr Rgba32 extrinsic = rgba_magenta;
    std::vector<Metatile<Rgba32>> metatiles{solid_metatile(extrinsic)};
    auto tile = solid_tile(rgba_white);
    tile.set(0, 0, stray);
    tile.set(0, 1, Rgba32{});
    metatiles.at(0).set_bottom(0, tile);

    Animation<Rgba32> flower{"flower"};
    auto frame_tile = solid_tile(rgba_blue);
    frame_tile.set(5, 5, stray);
    flower.key_frame(AnimFrame<Rgba32>{"key", {frame_tile}});
    flower.put_frame("00", AnimFrame<Rgba32>{"00", {solid_tile(extrinsic)}});
    std::map<std::string, Animation<Rgba32>> anims{};
    anims.emplace("flower", flower);

    const auto summary = count_tileset_colors(metatiles, anims, extrinsic);

    // Bottom tile 0: 62 white, 1 stray, 1 alpha 0. Key frame tile: 63 blue, 1 stray.
    EXPECT_EQ(summary.counts.size(), 3);
    EXPECT_EQ(summary.counts.at(rgba_white), 62);
    EXPECT_EQ(summary.counts.at(stray), 2);
    EXPECT_EQ(summary.counts.at(rgba_blue), 63);
    EXPECT_EQ(summary.opaque_pixels, 127);
    // 12 tiles * 64 = 768 metatile pixels, minus 63 opaque in tile 0, plus the 64-pixel extrinsic frame tile.
    EXPECT_EQ(summary.transparent_pixels, 768 - 63 + 64);
}

TEST(ColorSearchTests, SortColorCountsDescendingBreaksTiesByColor)
{
    const std::map<Rgba32, unsigned int> counts{{rgba_white, 5}, {rgba_black, 5}, {stray, 1}, {rgba_blue, 9}};

    const auto sorted = sort_color_counts_descending(counts);

    ASSERT_EQ(sorted.size(), 4);
    EXPECT_EQ(sorted.at(0).first, rgba_blue);
    EXPECT_EQ(sorted.at(1).first, rgba_black);
    EXPECT_EQ(sorted.at(2).first, rgba_white);
    EXPECT_EQ(sorted.at(3).first, stray);
}

TEST(ColorSearchTests, GroupSimilarColorsAnchorsOnMostCommon)
{
    const std::vector<std::pair<Rgba32, unsigned int>> sorted{
        {rgba_white, 100},
        {Rgba32{250, 250, 250}, 10},
        {stray, 8},
        {Rgba32{245, 245, 245}, 3},
        {near_stray, 1},
    };

    const auto groups = group_similar_colors(sorted, ColorTolerance::per_channel(8));

    ASSERT_EQ(groups.size(), 3);
    EXPECT_EQ(groups.at(0).anchor, rgba_white);
    EXPECT_EQ(groups.at(0).members.size(), 2);
    EXPECT_EQ(groups.at(0).total_pixels, 110);
    EXPECT_EQ(groups.at(1).anchor, stray);
    EXPECT_EQ(groups.at(1).members.size(), 2);
    EXPECT_EQ(groups.at(1).members.at(1).first, near_stray);
    EXPECT_EQ(groups.at(1).total_pixels, 9);
    // 245 is within 8 of neither 255 (the anchor) nor 234, so it starts its own group even though it is within 8 of
    // the 250 member: membership is measured against the anchor only.
    EXPECT_EQ(groups.at(2).anchor, (Rgba32{245, 245, 245}));
    EXPECT_EQ(groups.at(2).total_pixels, 3);
}

TEST(ColorSearchTests, GroupSimilarColorsOrdersGroupsByTotal)
{
    const std::vector<std::pair<Rgba32, unsigned int>> sorted{
        {rgba_white, 10},
        {stray, 9},
        {near_stray, 9},
    };

    const auto groups = group_similar_colors(sorted, ColorTolerance::per_channel(2));

    ASSERT_EQ(groups.size(), 2);
    EXPECT_EQ(groups.at(0).anchor, stray);
    EXPECT_EQ(groups.at(0).total_pixels, 18);
    EXPECT_EQ(groups.at(1).anchor, rgba_white);
}

TEST(ColorSearchTests, GroupSimilarColorsGbaRuleIsBucketPartition)
{
    // 248, 250, and 255 all downconvert to 31; 247 downconverts to 30 even though it is within 1 of 248.
    const std::vector<std::pair<Rgba32, unsigned int>> sorted{
        {Rgba32{250, 250, 250}, 50},
        {rgba_white, 40},
        {Rgba32{247, 247, 247}, 30},
        {Rgba32{248, 248, 248}, 20},
    };

    const auto groups = group_similar_colors(sorted, ColorTolerance::gba());

    ASSERT_EQ(groups.size(), 2);
    EXPECT_EQ(groups.at(0).anchor, (Rgba32{250, 250, 250}));
    ASSERT_EQ(groups.at(0).members.size(), 3);
    EXPECT_EQ(groups.at(0).members.at(1).first, rgba_white);
    EXPECT_EQ(groups.at(0).members.at(2).first, (Rgba32{248, 248, 248}));
    EXPECT_EQ(groups.at(0).total_pixels, 110);
    EXPECT_EQ(groups.at(0).anchor.quantize_to_gba(), rgba_white);
    EXPECT_EQ(groups.at(1).anchor, (Rgba32{247, 247, 247}));
    EXPECT_EQ(groups.at(1).members.size(), 1);
}

TEST(ColorSearchTests, GroupSimilarColorsZeroToleranceIsIdentity)
{
    const std::vector<std::pair<Rgba32, unsigned int>> sorted{{rgba_white, 3}, {stray, 2}, {near_stray, 1}};

    const auto groups = group_similar_colors(sorted, ColorTolerance::exact());

    ASSERT_EQ(groups.size(), 3);
    for (const auto &group : groups) {
        EXPECT_EQ(group.members.size(), 1);
    }
}
