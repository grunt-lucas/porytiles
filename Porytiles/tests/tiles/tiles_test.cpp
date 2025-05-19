#include "fmt/format.h"

#include <gtest/gtest.h>

#include "porytiles/tiles/tile.hpp"
#include "porytiles/tiles/tile_metadata.hpp"

using namespace porytiles;

TEST(TileMetadataTests, TileMetadataShouldFetchSuccessfully) {
    const Tile<int> tile{TileType::kFree};
    const auto metadata = tile.metadata<FreeMetadata>();
    ASSERT_EQ(metadata.tile_index(), 0);
}
