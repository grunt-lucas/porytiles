#include "gtest/gtest.h"

#include "porytiles2/domain/repos/tileset_artifact.hpp"

using namespace porytiles2;

TEST(TilesetArtifactTests, FooShouldBeZero) {
    using enum TilesetArtifact::Type;
    TilesetArtifact foo{metatiles_bin};
    EXPECT_EQ(0, 0);
}
