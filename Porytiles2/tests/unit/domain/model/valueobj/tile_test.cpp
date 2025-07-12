#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/domain/model/valueobj/tile.hpp>

using namespace porytiles2;

TEST(TileTests, AtAndSetShouldWork) {
    Tile<int> tile{};

    EXPECT_EQ(0, tile.At(0));
    EXPECT_EQ(0, tile.At(63));
    EXPECT_DEATH(std::ignore = tile.At(kTileSize), "Index 64 out of bounds");
    EXPECT_DEATH(std::ignore = tile.At(kTileSideLength, 2), "Row index 8 out of bounds");
    EXPECT_DEATH(std::ignore = tile.At(0, kTileSideLength), "Col index 8 out of bounds");

    // Set value using index, fetch using row/col
    tile.Set(22, 10);
    EXPECT_EQ(10, tile.At(2, 6));

    // Set value using row/col, fetch using index
    tile.Set(5, 2, 31);
    EXPECT_EQ(31, tile.At(42));
}
