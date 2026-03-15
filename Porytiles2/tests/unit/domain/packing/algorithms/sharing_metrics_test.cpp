#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/packing/algorithms/sharing_metrics.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"
#include "porytiles2/domain/packing/models/shape_group_metadata.hpp"

using namespace porytiles2;

namespace {

PackableTile make_tile(PackableTile::Id id, std::initializer_list<std::size_t> color_indices)
{
    ColorSet cs{};
    for (auto idx : color_indices) {
        cs.set(idx);
    }
    return PackableTile{std::move(id), cs};
}

} // namespace

TEST(SharingMetricsTests, PaletteContainsSibling_NoGroupMembership_ReturnsFalse)
{
    ShapeGroupMetadata metadata;
    PackedPalette pal{0};

    auto tile = make_tile(PackableTile::RegularId{0}, {1, 2});
    pal.add_tile(tile);

    // Tile not in any group
    EXPECT_FALSE(palette_contains_sibling(PackableTile::RegularId{0}, pal, metadata));
}

TEST(SharingMetricsTests, PaletteContainsSibling_NoSiblingInPalette_ReturnsFalse)
{
    ShapeGroupMetadata metadata;
    metadata.tile_id_to_group[PackableTile::RegularId{0}] = 0;
    metadata.tile_id_to_group[PackableTile::RegularId{1}] = 0;
    metadata.group_members = {{PackableTile::RegularId{0}, PackableTile::RegularId{1}}};

    // Palette only contains tile 0 (no sibling)
    PackedPalette pal{0};
    auto tile0 = make_tile(PackableTile::RegularId{0}, {1, 2});
    pal.add_tile(tile0);

    EXPECT_FALSE(palette_contains_sibling(PackableTile::RegularId{0}, pal, metadata));
}

TEST(SharingMetricsTests, PaletteContainsSibling_SiblingPresent_ReturnsTrue)
{
    ShapeGroupMetadata metadata;
    metadata.tile_id_to_group[PackableTile::RegularId{0}] = 0;
    metadata.tile_id_to_group[PackableTile::RegularId{1}] = 0;
    metadata.group_members = {{PackableTile::RegularId{0}, PackableTile::RegularId{1}}};

    // Palette contains tile 0 (sibling of tile 1)
    PackedPalette pal{0};
    auto tile0 = make_tile(PackableTile::RegularId{0}, {1, 2});
    pal.add_tile(tile0);

    // Checking for tile 1's siblings in the palette
    EXPECT_TRUE(palette_contains_sibling(PackableTile::RegularId{1}, pal, metadata));
}

TEST(SharingMetricsTests, ComputeSharingPenalty_NonGroupTile_ReturnsZero)
{
    ShapeGroupMetadata metadata;
    PackedPalette pal{0};

    auto tile = make_tile(PackableTile::RegularId{0}, {1, 2, 3});
    EXPECT_DOUBLE_EQ(compute_sharing_penalty(tile, pal, metadata), 0.0);
}

TEST(SharingMetricsTests, ComputeSharingPenalty_NoSiblingInPalette_ReturnsZero)
{
    ShapeGroupMetadata metadata;
    metadata.tile_id_to_group[PackableTile::RegularId{0}] = 0;
    metadata.tile_id_to_group[PackableTile::RegularId{1}] = 0;
    metadata.group_members = {{PackableTile::RegularId{0}, PackableTile::RegularId{1}}};

    PackedPalette pal{0};
    auto tile = make_tile(PackableTile::RegularId{0}, {1, 2, 3});

    EXPECT_DOUBLE_EQ(compute_sharing_penalty(tile, pal, metadata), 0.0);
}

TEST(SharingMetricsTests, ComputeSharingPenalty_SiblingPresent_ReturnsPenalty)
{
    ShapeGroupMetadata metadata;
    metadata.tile_id_to_group[PackableTile::RegularId{0}] = 0;
    metadata.tile_id_to_group[PackableTile::RegularId{1}] = 0;
    metadata.group_members = {{PackableTile::RegularId{0}, PackableTile::RegularId{1}}};

    PackedPalette pal{0};
    auto tile0 = make_tile(PackableTile::RegularId{0}, {1, 2});
    pal.add_tile(tile0);

    // Tile 1 has 3 colors, default sharing_weight = 0.5
    auto tile1 = make_tile(PackableTile::RegularId{1}, {4, 5, 6});
    EXPECT_DOUBLE_EQ(compute_sharing_penalty(tile1, pal, metadata), 0.5 * 3.0);
}

TEST(SharingMetricsTests, ComputeSharingPenalty_CustomWeight)
{
    ShapeGroupMetadata metadata;
    metadata.tile_id_to_group[PackableTile::RegularId{0}] = 0;
    metadata.tile_id_to_group[PackableTile::RegularId{1}] = 0;
    metadata.group_members = {{PackableTile::RegularId{0}, PackableTile::RegularId{1}}};

    PackedPalette pal{0};
    auto tile0 = make_tile(PackableTile::RegularId{0}, {1, 2});
    pal.add_tile(tile0);

    auto tile1 = make_tile(PackableTile::RegularId{1}, {4, 5, 6, 7});
    EXPECT_DOUBLE_EQ(compute_sharing_penalty(tile1, pal, metadata, 0.75), 0.75 * 4.0);
}
