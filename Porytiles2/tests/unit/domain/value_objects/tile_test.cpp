#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/domain/value_objects/Tile.hpp>

using namespace porytiles;

TEST(TileTests, AtAndSetShouldWork) {
    Tile<int> tile{};

    EXPECT_EQ(0, tile.At(0));
    EXPECT_EQ(0, tile.At(63));
    EXPECT_EXIT(std::ignore = tile.At(kTileSize), ::testing::KilledBySignal(SIGABRT), "Index 64 out of bounds");
    EXPECT_EXIT(std::ignore = tile.At(kTileSideLength, 2), ::testing::KilledBySignal(SIGABRT),
                "Row index 8 out of bounds");
    EXPECT_EXIT(std::ignore = tile.At(0, kTileSideLength), ::testing::KilledBySignal(SIGABRT),
                "Col index 8 out of bounds");

    // Set value using index, fetch using row/col
    tile.Set(22, 10);
    EXPECT_EQ(10, tile.At(2, 6));

    // Set value using row/col, fetch using index
    tile.Set(5, 2, 31);
    EXPECT_EQ(31, tile.At(42));
}
