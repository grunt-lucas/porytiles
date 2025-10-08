#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/tile_constants.hpp"

using namespace porytiles2;

TEST(PixelTileTests, AtAndSetShouldWork)
{
    PixelTile<IndexPixel> tile{};

    EXPECT_EQ(0, tile.at(0).index());
    EXPECT_EQ(0, tile.at(63).index());
    EXPECT_DEATH(std::ignore = tile.at(tile::size_pix), "index out of bounds: 64");
    EXPECT_DEATH(std::ignore = tile.at(tile::side_length_pix, 2), "row index out of bounds: 8");
    EXPECT_DEATH(std::ignore = tile.at(0, tile::side_length_pix), "col index out of bounds: 8");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6).index());

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42).index());
}
