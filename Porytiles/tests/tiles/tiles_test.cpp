#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles/tiles/rgba_tile.hpp>
#include <porytiles/tiles/tile.hpp>
#include <porytiles/tiles/tile_metadata.hpp>

using namespace porytiles;

TEST(TileTests, TileAtAndSetShouldWork) {
    Tile<int> tile{TileType::kFree};

    ASSERT_EQ(tile.At(0), 0);
    ASSERT_EQ(tile.At(63), 0);
    EXPECT_EXIT(std::ignore = tile.At(kTileSize), ::testing::KilledBySignal(SIGABRT), "Index 64 out of bounds");
    EXPECT_EXIT(std::ignore = tile.At(kTileSideLength, 2), ::testing::KilledBySignal(SIGABRT),
                "Row index 8 out of bounds");
    EXPECT_EXIT(std::ignore = tile.At(0, kTileSideLength), ::testing::KilledBySignal(SIGABRT),
                "Col index 8 out of bounds");

    tile.Set(22, 10);
    tile.Set(5, 2, 31);
    ASSERT_EQ(tile.At(2, 6), 10);
    ASSERT_EQ(tile.At(42), 31);
}

TEST(TileMetadataTests, TileMetadataShouldFetchSuccessfully) {
    const Tile<int> tile{TileType::kFree};
    const auto metadata = tile.metadata<FreeMetadata>();
    ASSERT_EQ(metadata.tile_index(), 0);
}

TEST(TileMetadataTests, TileMetadataShouldAbortOnBadFetchType) {
    const Tile<int> tile{TileType::kFree};
    EXPECT_EXIT(std::ignore = tile.metadata<LayeredMetadata>(), ::testing::KilledBySignal(SIGABRT),
                "Metadata std::variant did not contain expected type");
}
