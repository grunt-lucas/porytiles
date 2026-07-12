#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <unordered_set>

#include "porytiles/domain/models/color_set.hpp"
#include "porytiles/domain/packing/models/packable_tile.hpp"

using namespace porytiles;

TEST(PackableTileTests, PrimaryTileIdEquality)
{
    PackableTile::PrimaryTileId a{5, 3};
    PackableTile::PrimaryTileId b{5, 3};
    PackableTile::PrimaryTileId c{5, 4};
    PackableTile::PrimaryTileId d{6, 3};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

TEST(PackableTileTests, PrimaryTileIdComparison)
{
    PackableTile::PrimaryTileId a{1, 2};
    PackableTile::PrimaryTileId b{1, 3};
    PackableTile::PrimaryTileId c{2, 1};

    EXPECT_LT(a, b);
    EXPECT_LT(a, c);
    EXPECT_LT(b, c);
}

TEST(PackableTileTests, PrimaryTileIdHash)
{
    std::unordered_set<PackableTile::PrimaryTileId> set;
    set.insert(PackableTile::PrimaryTileId{0, 1});
    set.insert(PackableTile::PrimaryTileId{0, 2});
    set.insert(PackableTile::PrimaryTileId{0, 1});

    EXPECT_EQ(set.size(), 2);
}

TEST(PackableTileTests, PrimaryTileIdToString)
{
    PackableTile::Id id = PackableTile::PrimaryTileId{7, 2};
    auto str = to_string(id);

    EXPECT_EQ(str, "Primary(tile=7, palette=2)");
}

TEST(PackableTileTests, PrimaryTileIdConstruction)
{
    ColorSet cs{};
    cs.set(0);
    cs.set(1);

    PackableTile tile{PackableTile::PrimaryTileId{10, 5}, cs};

    EXPECT_TRUE(std::holds_alternative<PackableTile::PrimaryTileId>(tile.id()));
    auto primary_id = std::get<PackableTile::PrimaryTileId>(tile.id());
    EXPECT_EQ(primary_id.tile_index, 10);
    EXPECT_EQ(primary_id.palette_index, 5);
    EXPECT_EQ(tile.color_count(), 2);
}

TEST(PackableTileTests, PrimaryTileIdVariantConstruction)
{
    ColorSet cs{};
    cs.set(3);

    PackableTile::Id id = PackableTile::PrimaryTileId{1, 4};
    PackableTile tile{id, cs};

    EXPECT_TRUE(std::holds_alternative<PackableTile::PrimaryTileId>(tile.id()));
}
