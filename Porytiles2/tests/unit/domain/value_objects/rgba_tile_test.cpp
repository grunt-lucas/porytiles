#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/domain/value_objects/Rgba32.hpp>
#include <porytiles2/domain/value_objects/RgbaTile.hpp>

using namespace porytiles;

TEST(RgbaTileTests, IsTransparentShouldUseAlphaCorrectly) {
    RgbaTile tile{};

    tile.Set(12, Rgba32{22, 90, 144});
    EXPECT_FALSE(tile.IsTransparent(kRgbaMagenta));

    tile.Set(12, Rgba32{22, 90, 144, Rgba32::kAlphaTransparent});
    EXPECT_TRUE(tile.IsTransparent(kRgbaMagenta));
}
