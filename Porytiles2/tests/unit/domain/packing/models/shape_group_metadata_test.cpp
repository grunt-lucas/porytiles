#include <gtest/gtest.h>

#include "porytiles2/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/shape_group_metadata.hpp"

using namespace porytiles2;

namespace {

const Rgba32 transparent = rgba_magenta;
const Rgba32 red{255, 0, 0, Rgba32::alpha_opaque};
const Rgba32 blue{0, 0, 255, Rgba32::alpha_opaque};
const Rgba32 green{0, 255, 0, Rgba32::alpha_opaque};

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

} // namespace

TEST(ShapeGroupMetadataTests, BuildFromShapeGroupsMapsTileIds)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // Build ID mapping: index 0 -> RegularId{0}, index 1 -> RegularId{1}
    std::vector<PackableTile::Id> index_to_id = {PackableTile::RegularId{0}, PackableTile::RegularId{1}};

    auto metadata = build_shape_group_metadata(shape_groups, index_to_id);

    EXPECT_EQ(metadata.group_members.size(), 1);
    EXPECT_EQ(metadata.group_members[0].size(), 2);
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{0}));
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{1}));
    EXPECT_EQ(metadata.tile_id_to_group.at(PackableTile::RegularId{0}), 0);
    EXPECT_EQ(metadata.tile_id_to_group.at(PackableTile::RegularId{1}), 0);
}

TEST(ShapeGroupMetadataTests, BuildFromShapeGroupsAllRegularIds)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    auto tile3 = make_single_color_tile(green);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2, tile3};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // All entries are RegularId (anim tiles no longer participate in shape groups)
    std::vector<PackableTile::Id> index_to_id = {
        PackableTile::RegularId{0}, PackableTile::RegularId{1}, PackableTile::RegularId{2}};

    auto metadata = build_shape_group_metadata(shape_groups, index_to_id);

    EXPECT_EQ(metadata.group_members.size(), 1);
    EXPECT_EQ(metadata.group_members[0].size(), 3);
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{0}));
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{1}));
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{2}));
}

TEST(ShapeGroupMetadataTests, EmptyShapeGroupsProducesEmpty)
{
    std::vector<ShapeGroup<Rgba32>> empty_groups;
    std::vector<PackableTile::Id> empty_ids;

    auto metadata = build_shape_group_metadata(empty_groups, empty_ids);

    EXPECT_TRUE(metadata.tile_id_to_group.empty());
    EXPECT_TRUE(metadata.group_members.empty());
}

TEST(ShapeGroupMetadataTests, MultipleGroupsMapsCorrectly)
{
    // Create two independent shape groups
    auto tile_r = make_single_color_tile(red);
    auto tile_b = make_single_color_tile(blue);

    // Create a different shape: two-color tile
    PixelTile<Rgba32> tile_rg{transparent};
    PixelTile<Rgba32> tile_bg{transparent};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            tile_rg.set(row, col, red);
            tile_bg.set(row, col, blue);
        }
    }
    for (std::size_t row = 4; row < 8; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            tile_rg.set(row, col, green);
            tile_bg.set(row, col, green);
        }
    }

    std::vector<PixelTile<Rgba32>> tiles = {tile_r, tile_b, tile_rg, tile_bg};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    // Should have at least 1 group (the single-color tiles), possibly 2
    ASSERT_GE(shape_groups.size(), 1);

    std::vector<PackableTile::Id> index_to_id = {
        PackableTile::RegularId{0},
        PackableTile::RegularId{1},
        PackableTile::RegularId{2},
        PackableTile::RegularId{3},
    };

    auto metadata = build_shape_group_metadata(shape_groups, index_to_id);

    // Each tile in a group should have a valid group index
    for (const auto &[id, group_idx] : metadata.tile_id_to_group) {
        EXPECT_LT(group_idx, metadata.group_members.size());
    }

    for (const auto &members : metadata.group_members) {
        EXPECT_GE(members.size(), 2);
    }
}

TEST(ShapeGroupMetadataTests, PrimaryTileIdsFilteredFromMetadata)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    auto tile3 = make_single_color_tile(green);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2, tile3};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // Index 0 and 1 are RegularId, index 2 is PrimaryTileId (should be filtered)
    std::vector<PackableTile::Id> index_to_id = {
        PackableTile::RegularId{0}, PackableTile::RegularId{1}, PackableTile::PrimaryTileId{0, 3}};

    auto metadata = build_shape_group_metadata(shape_groups, index_to_id);

    EXPECT_EQ(metadata.group_members.size(), 1);
    EXPECT_EQ(metadata.group_members.at(0).size(), 2);
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{0}));
    EXPECT_TRUE(metadata.tile_id_to_group.contains(PackableTile::RegularId{1}));
    EXPECT_FALSE(metadata.tile_id_to_group.contains(PackableTile::PrimaryTileId{0, 3}));
}

TEST(ShapeGroupMetadataTests, AllPrimaryTileIdsProducesEmptyGroup)
{
    auto tile1 = make_single_color_tile(red);
    auto tile2 = make_single_color_tile(blue);
    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    auto shape_groups = analyze_shape_groups(tiles, transparent);
    ASSERT_EQ(shape_groups.size(), 1);

    // Both members are PrimaryTileId — group should be skipped entirely
    std::vector<PackableTile::Id> index_to_id = {PackableTile::PrimaryTileId{0, 1}, PackableTile::PrimaryTileId{1, 2}};

    auto metadata = build_shape_group_metadata(shape_groups, index_to_id);

    EXPECT_TRUE(metadata.group_members.empty());
    EXPECT_TRUE(metadata.tile_id_to_group.empty());
}
