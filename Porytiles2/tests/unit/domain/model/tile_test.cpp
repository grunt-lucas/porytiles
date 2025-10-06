#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/tile.hpp"

using namespace porytiles2;

TEST(TileTests, AtAndSetShouldWork)
{
    Tile<IndexPixel> tile{};

    EXPECT_EQ(0, tile.at(0).index());
    EXPECT_EQ(0, tile.at(63).index());
    EXPECT_DEATH(std::ignore = tile.at(tile_size), "index 64 out of bounds");
    EXPECT_DEATH(std::ignore = tile.at(tile_side_length, 2), "row index 8 out of bounds");
    EXPECT_DEATH(std::ignore = tile.at(0, tile_side_length), "col index 8 out of bounds");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6).index());

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42).index());
}
