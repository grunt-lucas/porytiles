#include <gtest/gtest.h>

#include "porytiles2/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/algorithms/indirect_link_builder.hpp"

using namespace porytiles2;

namespace {

const Rgba32 transparent = rgba_magenta;
const Rgba32 red{255, 0, 0, Rgba32::alpha_opaque};
const Rgba32 blue{0, 0, 255, Rgba32::alpha_opaque};
const Rgba32 green{0, 255, 0, Rgba32::alpha_opaque};
const Rgba32 yellow{255, 255, 0, Rgba32::alpha_opaque};
const Rgba32 white{255, 255, 255, Rgba32::alpha_opaque};

PixelTile<Rgba32> make_single_color_tile(const Rgba32 &color)
{
    PixelTile<Rgba32> tile{transparent};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            tile.set(row, col, color);
        }
    }
    return tile;
}

Palette<Rgba32, pal::max_size> make_palette(const Rgba32 &slot0, const std::vector<Rgba32> &colors)
{
    Palette<Rgba32, pal::max_size> pal{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};
    pal.set(0, slot0);
    for (std::size_t i = 0; i < colors.size() && (i + 1) < pal::max_size; ++i) {
        pal.set(i + 1, colors.at(i));
    }
    return pal;
}

} // namespace

TEST(IndirectLinkBuilderTests, MembersInSamePalette_ShouldGenerateNoLinks)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // Both tiles assigned to palette 0 by the packer
    std::map<std::size_t, std::size_t> tile_pal_assignments = {{0, 0}, {1, 0}};

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> base_pals{};
    base_pals.at(0) = make_palette(transparent, {red, blue});

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};

    auto links = build_indirect_links(shape_groups, tile_pal_assignments, base_pals, prefilled_pals);
    EXPECT_TRUE(links.empty());
}

TEST(IndirectLinkBuilderTests, MembersInDifferentPalettes_ShouldGenerateLinks)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // tile1 assigned to pal 0, tile2 assigned to pal 1
    std::map<std::size_t, std::size_t> tile_pal_assignments = {{0, 0}, {1, 1}};

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> base_pals{};
    base_pals.at(0) = make_palette(transparent, {red});
    base_pals.at(1) = make_palette(transparent, {blue});

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};

    auto links = build_indirect_links(shape_groups, tile_pal_assignments, base_pals, prefilled_pals);

    // Should generate one Indirect link: blue in pal 1 follows red in pal 0
    ASSERT_EQ(links.size(), 1);
    EXPECT_EQ(links.at(0).source_pal, 1);
    EXPECT_EQ(links.at(0).source_color, blue);
    EXPECT_EQ(links.at(0).ref_pal, 0);
    EXPECT_EQ(links.at(0).ref_color, red);
}

TEST(IndirectLinkBuilderTests, PrefilledSlot_ShouldPickBetterReference)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // tile1 assigned to pal 0, tile2 assigned to pal 1
    std::map<std::size_t, std::size_t> tile_pal_assignments = {{0, 0}, {1, 1}};

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> base_pals{};
    base_pals.at(0) = make_palette(transparent, {red});
    base_pals.at(1) = make_palette(transparent, {blue});

    // Prefilled pal 1 has a locked color at slot 1
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals.at(1) = make_palette(transparent, {white}); // white locked at slot 1

    auto links = build_indirect_links(shape_groups, tile_pal_assignments, base_pals, prefilled_pals);

    /*
     * With pal 1 having a prefilled slot 1, choosing pal 0's member as reference would generate a link
     * targeting pal 1 (which has the conflict). The conflict-minimization heuristic should instead pick pal 1's
     * member (blue) as reference and generate a link for red in pal 0 (no prefilled, 0 conflicts).
     */
    ASSERT_EQ(links.size(), 1);
    EXPECT_EQ(links.at(0).source_pal, 0);
    EXPECT_EQ(links.at(0).source_color, red);
    EXPECT_EQ(links.at(0).ref_pal, 1);
    EXPECT_EQ(links.at(0).ref_color, blue);
}

TEST(IndirectLinkBuilderTests, EmptyShapeGroups_ShouldReturnEmpty)
{
    std::vector<ShapeGroup<Rgba32>> empty_groups;
    std::map<std::size_t, std::size_t> tile_pal_assignments;
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> base_pals{};
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};

    auto links = build_indirect_links(empty_groups, tile_pal_assignments, base_pals, prefilled_pals);
    EXPECT_TRUE(links.empty());
}
