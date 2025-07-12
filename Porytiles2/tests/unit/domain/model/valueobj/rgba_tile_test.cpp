#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/domain/model/valueobj/rgba32.hpp>
#include <porytiles2/domain/model/valueobj/rgba_tile.hpp>

using namespace porytiles2;

TEST(RgbaTileTests, IsTransparentShouldUseAlphaCorrectly) {
    RgbaTile tile{};

    tile.Set(12, Rgba32{22, 90, 144});
    EXPECT_FALSE(tile.is_transparent(kRgbaMagenta));

    tile.Set(12, Rgba32{22, 90, 144, Rgba32::alpha_transparent});
    EXPECT_TRUE(tile.is_transparent(kRgbaMagenta));
}
