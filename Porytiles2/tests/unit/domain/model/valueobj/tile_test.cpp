#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/model/valueobj/tile.hpp"

using namespace porytiles2;

TEST(TileTests, AtAndSetShouldWork) {
    Tile<int> tile{};

    EXPECT_EQ(0, tile.at(0));
    EXPECT_EQ(0, tile.at(63));
    EXPECT_DEATH(std::ignore = tile.at(Tile<int>::tile_size), "index 64 out of bounds");
    EXPECT_DEATH(std::ignore = tile.at(Tile<int>::tile_side_length, 2), "row index 8 out of bounds");
    EXPECT_DEATH(std::ignore = tile.at(0, Tile<int>::tile_side_length), "col index 8 out of bounds");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6));

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42));
}
