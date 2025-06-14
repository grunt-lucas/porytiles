#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/colors/rgba32.hpp>
#include <porytiles2/tiles/rgba_tile.hpp>

using namespace porytiles;

TEST(RgbaTileTests, IsTransparentShouldUseAlphaCorrectly) {
    RgbaTile tile{};

    tile.Set(12, Rgba32{22, 90, 144});
    ASSERT_FALSE(tile.IsTransparent(kRgbaMagenta));

    tile.Set(12, Rgba32{22, 90, 144, Rgba32::kAlphaTransparent});
    ASSERT_TRUE(tile.IsTransparent(kRgbaMagenta));
}
