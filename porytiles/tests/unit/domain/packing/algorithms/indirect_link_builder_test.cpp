#include <gtest/gtest.h>

#include "porytiles/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/packing/algorithms/indirect_link_builder.hpp"

using namespace porytiles;

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

Palette<Rgba32, palette::max_size> make_palette(const Rgba32 &slot0, const std::vector<Rgba32> &colors)
{
    Palette<Rgba32, palette::max_size> palette{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};
    palette.set(0, slot0);
    for (std::size_t i = 0; i < colors.size() && (i + 1) < palette::max_size; ++i) {
        palette.set(i + 1, colors.at(i));
    }
    return palette;
}

} // namespace

TEST(IndirectLinkBuilderTests, SamePaletteNoLinks)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // Both tiles assigned to palette 0 by the packer
    std::map<std::size_t, std::size_t> tile_palette_assignments = {{0, 0}, {1, 0}};

    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> base_palettes{};
    base_palettes.at(0) = make_palette(transparent, {red, blue});

    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    auto links = build_indirect_links(shape_groups, tile_palette_assignments, base_palettes, prefilled_palettes);
    EXPECT_TRUE(links.empty());
}

TEST(IndirectLinkBuilderTests, DifferentPalettesGenerateLinks)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // tile1 assigned to palette 0, tile2 assigned to palette 1
    std::map<std::size_t, std::size_t> tile_palette_assignments = {{0, 0}, {1, 1}};

    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> base_palettes{};
    base_palettes.at(0) = make_palette(transparent, {red});
    base_palettes.at(1) = make_palette(transparent, {blue});

    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    auto links = build_indirect_links(shape_groups, tile_palette_assignments, base_palettes, prefilled_palettes);

    ASSERT_EQ(links.size(), 1);
    EXPECT_EQ(links.at(0).source_palette, 1);
    EXPECT_EQ(links.at(0).source_color, blue);
    EXPECT_EQ(links.at(0).ref_palette, 0);
    EXPECT_EQ(links.at(0).ref_color, red);
}

TEST(IndirectLinkBuilderTests, PrefilledSlotPicksBetterRef)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // tile1 assigned to palette 0, tile2 assigned to palette 1
    std::map<std::size_t, std::size_t> tile_palette_assignments = {{0, 0}, {1, 1}};

    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> base_palettes{};
    base_palettes.at(0) = make_palette(transparent, {red});
    base_palettes.at(1) = make_palette(transparent, {blue});

    // Prefilled palette 1 has a locked color at slot 1
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    prefilled_palettes.at(1) = make_palette(transparent, {white}); // white locked at slot 1

    auto links = build_indirect_links(shape_groups, tile_palette_assignments, base_palettes, prefilled_palettes);

    // With palette 1 having a prefilled slot 1, choosing palette 0's member as reference would generate a link
    // targeting palette 1 (which has the conflict). The conflict-minimization heuristic should instead pick palette 1's
    // member (blue) as reference and generate a link for red in palette 0 (no prefilled, 0 conflicts).
    ASSERT_EQ(links.size(), 1);
    EXPECT_EQ(links.at(0).source_palette, 0);
    EXPECT_EQ(links.at(0).source_color, red);
    EXPECT_EQ(links.at(0).ref_palette, 1);
    EXPECT_EQ(links.at(0).ref_color, blue);
}

TEST(IndirectLinkBuilderTests, EmptyShapeGroupsReturnsEmpty)
{
    std::vector<ShapeGroup<Rgba32>> empty_groups;
    std::map<std::size_t, std::size_t> tile_palette_assignments;
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> base_palettes{};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    auto links = build_indirect_links(empty_groups, tile_palette_assignments, base_palettes, prefilled_palettes);
    EXPECT_TRUE(links.empty());
}
