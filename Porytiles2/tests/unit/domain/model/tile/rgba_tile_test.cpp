#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tile/rgba_tile.hpp"
#include "porytiles2/domain/model/tile/tile_constants.hpp"

using namespace porytiles2;

TEST(RgbaTileTests, IsTransparentShouldUseExtrinsicCorrectly)
{
    RgbaTile tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(RgbaTileTests, IsTransparentShouldUseAlphaCorrectly)
{
    // Default-constructed RgbaTile is zeroed, i.e. black and intrinsically transparent
    const RgbaTile tile{};
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(RgbaTileTests, IsTransparentShouldUseMixedTransparencyCorrectly)
{
    RgbaTile tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    // Set a pixel to non-magenta, but set alpha channel to transparent
    tile.set(12, Rgba32{22, 90, 144, Rgba32::alpha_transparent});

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}
