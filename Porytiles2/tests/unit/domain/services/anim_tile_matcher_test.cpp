#include "porytiles2/domain/services/anim_tile_matcher.hpp"

#include <optional>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles2/domain/models/anim_frame.hpp"
#include "porytiles2/domain/models/anim_params.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

namespace {

const Rgba32 transparent{0, 0, 0, 0};
const Rgba32 red{255, 0, 0, 255};
const Rgba32 green{0, 255, 0, 255};
const Rgba32 blue{0, 0, 255, 255};

PixelTile<Rgba32> make_solid_tile(const Rgba32 &color)
{
    PixelTile<Rgba32> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, color);
    }
    return tile;
}

PixelTile<Rgba32> make_two_color_tile(const Rgba32 &c1, const Rgba32 &c2)
{
    PixelTile<Rgba32> tile;
    for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
            tile.set(row, col, row < 4 ? c1 : c2);
        }
    }
    return tile;
}

PixelTile<Rgba32> make_asymmetric_tile()
{
    PixelTile<Rgba32> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, transparent);
    }
    // Place a single non-transparent pixel in the top-left corner only.
    // This tile is not symmetric under any flip, so canonical form is unique.
    tile.set(0, 0, red);
    tile.set(0, 1, green);
    tile.set(1, 0, blue);
    return tile;
}

Animation<Rgba32>
make_animation(const std::string &name, const std::vector<PixelTile<Rgba32>> &key_frame_tiles, std::size_t tile_count)
{
    AnimParams params;
    params.tile_count(tile_count);

    AnimFrame<Rgba32> key_frame{"key"};
    for (const auto &tile : key_frame_tiles) {
        key_frame.add_tile(tile);
    }

    // Also need at least one regular frame for has_frames() to be true
    AnimFrame<Rgba32> frame0{"0"};
    for (const auto &tile : key_frame_tiles) {
        frame0.add_tile(tile);
    }

    Animation<Rgba32> anim{name, params};
    anim.key_frame(std::move(key_frame));
    anim.put_frame("0", std::move(frame0));
    return anim;
}

} // namespace

TEST(AnimTileMatcherTests, BasicKeyFrameMatch)
{
    AnimTileMatcher matcher;
    auto tile = make_solid_tile(red);
    auto anim = make_animation("flower", {tile}, 1);

    matcher.register_animation("flower", anim, 5, transparent);

    auto match = matcher.find_match(CanonicalPixelTile{tile, transparent}, transparent);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->anim_name, "flower");
    EXPECT_EQ(match->tile_index, 5u);
    EXPECT_EQ(match->keyframe_tile_idx, 0u);
    EXPECT_FALSE(match->h_flip);
    EXPECT_FALSE(match->v_flip);
    EXPECT_FALSE(match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, NoMatchForUnregisteredTile)
{
    AnimTileMatcher matcher;
    auto red_tile = make_solid_tile(red);
    auto green_tile = make_solid_tile(green);
    auto anim = make_animation("flower", {red_tile}, 1);

    matcher.register_animation("flower", anim, 5, transparent);

    auto match = matcher.find_match(CanonicalPixelTile{green_tile, transparent}, transparent);
    EXPECT_FALSE(match.has_value());
}

TEST(AnimTileMatcherTests, FlipMatch)
{
    AnimTileMatcher matcher;
    auto tile = make_asymmetric_tile();
    auto anim = make_animation("flower", {tile}, 1);

    matcher.register_animation("flower", anim, 5, transparent);

    // h-flip the tile and match
    auto h_flipped = tile.flip(true, false);
    auto match = matcher.find_match(CanonicalPixelTile{h_flipped, transparent}, transparent);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->anim_name, "flower");
    EXPECT_EQ(match->tile_index, 5u);
    EXPECT_TRUE(match->h_flip);
    EXPECT_FALSE(match->v_flip);
}

TEST(AnimTileMatcherTests, TransparentTileSkipped)
{
    AnimTileMatcher matcher;
    auto trans_tile = make_solid_tile(transparent);
    auto anim = make_animation("flower", {trans_tile}, 1);

    matcher.register_animation("flower", anim, 5, transparent);

    auto match = matcher.find_match(CanonicalPixelTile{trans_tile, transparent}, transparent);
    EXPECT_FALSE(match.has_value());
}

TEST(AnimTileMatcherTests, CrossTilesetFlag)
{
    AnimTileMatcher matcher;
    auto tile = make_solid_tile(red);
    auto anim = make_animation("primary_flower", {tile}, 1);

    matcher.register_animation("primary_flower", anim, 10, transparent, /*is_cross_tileset=*/true);

    auto match = matcher.find_match(CanonicalPixelTile{tile, transparent}, transparent);
    ASSERT_TRUE(match.has_value());
    EXPECT_TRUE(match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, SecondaryWinsOnCollision)
{
    AnimTileMatcher matcher;
    auto tile = make_solid_tile(red);
    auto secondary_anim = make_animation("sec_flower", {tile}, 1);
    auto primary_anim = make_animation("pri_flower", {tile}, 1);

    // Register secondary first (is_cross_tileset=false), then primary (is_cross_tileset=true)
    matcher.register_animation("sec_flower", secondary_anim, 20, transparent);
    matcher.register_animation("pri_flower", primary_anim, 5, transparent, /*is_cross_tileset=*/true);

    auto match = matcher.find_match(CanonicalPixelTile{tile, transparent}, transparent);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->anim_name, "sec_flower");
    EXPECT_EQ(match->tile_index, 20u);
    EXPECT_FALSE(match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, IsInAnimationRange)
{
    AnimTileMatcher matcher;
    auto t1 = make_solid_tile(red);
    auto t2 = make_two_color_tile(red, green);
    auto t3 = make_two_color_tile(green, blue);
    auto anim = make_animation("flower", {t1, t2, t3}, 3);

    matcher.register_animation("flower", anim, 5, transparent);

    EXPECT_FALSE(matcher.is_in_animation_range(4));
    EXPECT_TRUE(matcher.is_in_animation_range(5));
    EXPECT_TRUE(matcher.is_in_animation_range(6));
    EXPECT_TRUE(matcher.is_in_animation_range(7));
    EXPECT_FALSE(matcher.is_in_animation_range(8));
}

TEST(AnimTileMatcherTests, TileOffsetFor)
{
    AnimTileMatcher matcher;
    auto tile = make_solid_tile(red);
    auto anim = make_animation("flower", {tile}, 1);

    matcher.register_animation("flower", anim, 5, transparent);

    EXPECT_EQ(matcher.tile_offset_for("flower"), 5u);
    EXPECT_FALSE(matcher.tile_offset_for("nonexistent").has_value());
}

TEST(AnimTileMatcherTests, TileCountFor)
{
    AnimTileMatcher matcher;
    auto t1 = make_solid_tile(red);
    auto t2 = make_two_color_tile(red, green);
    auto anim = make_animation("flower", {t1, t2}, 2);

    matcher.register_animation("flower", anim, 5, transparent);

    EXPECT_EQ(matcher.tile_count_for("flower"), 2u);
    EXPECT_FALSE(matcher.tile_count_for("nonexistent").has_value());
}

TEST(AnimTileMatcherTests, CrossTilesetMultiTileWithTransparentSubtiles)
{
    AnimTileMatcher matcher;
    auto opaque_tile = make_solid_tile(red);
    auto trans_tile = make_solid_tile(transparent);
    auto anim = make_animation("pri_water", {opaque_tile, trans_tile, make_solid_tile(green)}, 3);

    matcher.register_animation("pri_water", anim, 10, transparent, /*is_cross_tileset=*/true);

    auto match_opaque = matcher.find_match(CanonicalPixelTile{opaque_tile, transparent}, transparent);
    ASSERT_TRUE(match_opaque.has_value());
    EXPECT_EQ(match_opaque->tile_index, 10u);
    EXPECT_TRUE(match_opaque->is_cross_tileset);

    auto match_trans = matcher.find_match(CanonicalPixelTile{trans_tile, transparent}, transparent);
    EXPECT_FALSE(match_trans.has_value());

    auto match_green = matcher.find_match(CanonicalPixelTile{make_solid_tile(green), transparent}, transparent);
    ASSERT_TRUE(match_green.has_value());
    EXPECT_EQ(match_green->tile_index, 12u);
    EXPECT_TRUE(match_green->is_cross_tileset);
}

TEST(AnimTileMatcherTests, SecondaryThenPrimarySeparateMatches)
{
    AnimTileMatcher matcher;
    auto sec_tile = make_solid_tile(red);
    auto pri_tile = make_solid_tile(green);
    auto sec_anim = make_animation("sec_flower", {sec_tile}, 1);
    auto pri_anim = make_animation("pri_flower", {pri_tile}, 1);

    matcher.register_animation("sec_flower", sec_anim, 20, transparent);
    matcher.register_animation("pri_flower", pri_anim, 5, transparent, /*is_cross_tileset=*/true);

    auto sec_match = matcher.find_match(CanonicalPixelTile{sec_tile, transparent}, transparent);
    ASSERT_TRUE(sec_match.has_value());
    EXPECT_EQ(sec_match->anim_name, "sec_flower");
    EXPECT_FALSE(sec_match->is_cross_tileset);

    auto pri_match = matcher.find_match(CanonicalPixelTile{pri_tile, transparent}, transparent);
    ASSERT_TRUE(pri_match.has_value());
    EXPECT_EQ(pri_match->anim_name, "pri_flower");
    EXPECT_TRUE(pri_match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, SecondaryThenPrimaryMixedCollision)
{
    AnimTileMatcher matcher;
    auto tile_a = make_solid_tile(red);
    auto tile_b = make_solid_tile(green);
    auto tile_c = make_two_color_tile(red, blue);

    auto sec_anim = make_animation("sec_wave", {tile_a, tile_b}, 2);
    auto pri_anim = make_animation("pri_wave", {tile_b, tile_c}, 2);

    matcher.register_animation("sec_wave", sec_anim, 20, transparent);
    matcher.register_animation("pri_wave", pri_anim, 5, transparent, /*is_cross_tileset=*/true);

    auto match_a = matcher.find_match(CanonicalPixelTile{tile_a, transparent}, transparent);
    ASSERT_TRUE(match_a.has_value());
    EXPECT_EQ(match_a->anim_name, "sec_wave");
    EXPECT_FALSE(match_a->is_cross_tileset);

    // tile_b collides: secondary wins (first-registration-wins)
    auto match_b = matcher.find_match(CanonicalPixelTile{tile_b, transparent}, transparent);
    ASSERT_TRUE(match_b.has_value());
    EXPECT_EQ(match_b->anim_name, "sec_wave");
    EXPECT_FALSE(match_b->is_cross_tileset);

    auto match_c = matcher.find_match(CanonicalPixelTile{tile_c, transparent}, transparent);
    ASSERT_TRUE(match_c.has_value());
    EXPECT_EQ(match_c->anim_name, "pri_wave");
    EXPECT_TRUE(match_c->is_cross_tileset);
}

TEST(AnimTileMatcherTests, IsInAnimationRangeWithBothTilesetTypes)
{
    AnimTileMatcher matcher;
    auto sec_tile = make_solid_tile(red);
    auto pri_tile = make_solid_tile(green);
    auto sec_anim = make_animation("sec_flower", {sec_tile}, 1);
    auto pri_anim = make_animation("pri_flower", {pri_tile}, 1);

    matcher.register_animation("sec_flower", sec_anim, 20, transparent);
    matcher.register_animation("pri_flower", pri_anim, 5, transparent, /*is_cross_tileset=*/true);

    EXPECT_TRUE(matcher.is_in_animation_range(20));
    EXPECT_TRUE(matcher.is_in_animation_range(5));
    EXPECT_FALSE(matcher.is_in_animation_range(19));
    EXPECT_FALSE(matcher.is_in_animation_range(21));
    EXPECT_FALSE(matcher.is_in_animation_range(4));
    EXPECT_FALSE(matcher.is_in_animation_range(6));
}

TEST(AnimTileMatcherTests, RegisterAndMatchAcrossDifferentEts)
{
    AnimTileMatcher matcher;

    // Build a tile whose background is rgba_magenta everywhere and has a single opaque marker
    // (register side uses magenta as its ET).
    PixelTile<Rgba32> magenta_tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        magenta_tile.set(i, rgba_magenta);
    }
    magenta_tile.set(2, 3, red);

    // Build a logically-equivalent tile with a cyan background (lookup side uses cyan as its ET).
    PixelTile<Rgba32> cyan_tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        cyan_tile.set(i, rgba_cyan);
    }
    cyan_tile.set(2, 3, red);

    auto anim = make_animation("pri_flower", {magenta_tile}, 1);
    matcher.register_animation("pri_flower", anim, 10, rgba_magenta, /*is_cross_tileset=*/true);

    auto match = matcher.find_match(CanonicalPixelTile{cyan_tile, rgba_cyan}, rgba_cyan);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->anim_name, "pri_flower");
    EXPECT_EQ(match->tile_index, 10u);
    EXPECT_TRUE(match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, RegisterPrimaryUnderDistinctEtPreservesSecondaryMatches)
{
    AnimTileMatcher matcher;

    // Secondary side uses cyan as ET; primary side uses magenta. Both animations share the same opaque
    // pattern, so their canonical forms should collide under the cross-ET comparator.
    PixelTile<Rgba32> sec_tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        sec_tile.set(i, rgba_cyan);
    }
    sec_tile.set(4, 5, green);

    PixelTile<Rgba32> pri_tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        pri_tile.set(i, rgba_magenta);
    }
    pri_tile.set(4, 5, green);

    auto sec_anim = make_animation("sec_wave", {sec_tile}, 1);
    auto pri_anim = make_animation("pri_wave", {pri_tile}, 1);

    // Register secondary first under its own ET, then the cross-tileset primary under its ET.
    matcher.register_animation("sec_wave", sec_anim, 20, rgba_cyan);
    matcher.register_animation("pri_wave", pri_anim, 5, rgba_magenta, /*is_cross_tileset=*/true);

    // Lookup with secondary ET should still find the secondary (first-registration-wins).
    auto match = matcher.find_match(CanonicalPixelTile{sec_tile, rgba_cyan}, rgba_cyan);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->anim_name, "sec_wave");
    EXPECT_FALSE(match->is_cross_tileset);
}

TEST(AnimTileMatcherTests, RegisterFullyTransparentPrimaryTileIsSkipped)
{
    AnimTileMatcher matcher;

    // Primary key frame tile is entirely transparent under the primary ET (magenta).
    PixelTile<Rgba32> fully_magenta;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        fully_magenta.set(i, rgba_magenta);
    }
    auto anim = make_animation("pri_empty", {fully_magenta}, 1);

    matcher.register_animation("pri_empty", anim, 10, rgba_magenta, /*is_cross_tileset=*/true);

    // Looking up the tile under any ET must not match — the entry was skipped during registration.
    EXPECT_FALSE(matcher.find_match(CanonicalPixelTile{fully_magenta, rgba_magenta}, rgba_magenta).has_value());
    EXPECT_FALSE(matcher.find_match(CanonicalPixelTile{fully_magenta, rgba_cyan}, rgba_cyan).has_value());
}

TEST(AnimTileMatcherTests, CrossTilesetFlipMatchPreservesPalIndex)
{
    AnimTileMatcher matcher;
    auto tile = make_asymmetric_tile();
    auto anim = make_animation("pri_flower", {tile}, 1);

    matcher.register_animation("pri_flower", anim, 10, transparent, /*is_cross_tileset=*/true);

    auto h_flipped = tile.flip(true, false);
    auto match = matcher.find_match(CanonicalPixelTile{h_flipped, transparent}, transparent);
    ASSERT_TRUE(match.has_value());
    EXPECT_TRUE(match->h_flip);
    EXPECT_FALSE(match->v_flip);
    EXPECT_TRUE(match->is_cross_tileset);
}
