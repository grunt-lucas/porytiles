#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles/tiles/tile.hpp>

using namespace porytiles;

TEST(TileTests, TileAtAndSetShouldWork) {
    Tile<int> tile{};

    ASSERT_EQ(tile.At(0), 0);
    ASSERT_EQ(tile.At(63), 0);
    ASSERT_EXIT(std::ignore = tile.At(kTileSize), ::testing::KilledBySignal(SIGABRT), "Index 64 out of bounds");
    ASSERT_EXIT(std::ignore = tile.At(kTileSideLength, 2), ::testing::KilledBySignal(SIGABRT),
                "Row index 8 out of bounds");
    ASSERT_EXIT(std::ignore = tile.At(0, kTileSideLength), ::testing::KilledBySignal(SIGABRT),
                "Col index 8 out of bounds");

    // Set value using index, fetch using row/col
    tile.Set(22, 10);
    ASSERT_EQ(tile.At(2, 6), 10);

    // Set value using row/col, fetch using index
    tile.Set(5, 2, 31);
    ASSERT_EQ(tile.At(42), 31);
}
