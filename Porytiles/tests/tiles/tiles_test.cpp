#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include "porytiles/tiles/tile.hpp"
#include "porytiles/tiles/tile_metadata.hpp"

using namespace porytiles;

TEST(TileMetadataTests, TileMetadataShouldFetchSuccessfully) {
    const Tile<int> tile{TileType::kFree};
    const auto metadata = tile.metadata<FreeMetadata>();
    ASSERT_EQ(metadata.tile_index(), 0);
}

TEST(TileMetadataTests, TileMetadataShouldAbortOnBadFetch) {
    const Tile<int> tile{TileType::kFree};
    EXPECT_EXIT(std::ignore = tile.metadata<LayeredMetadata>(), ::testing::KilledBySignal(SIGABRT), ".*");
}
