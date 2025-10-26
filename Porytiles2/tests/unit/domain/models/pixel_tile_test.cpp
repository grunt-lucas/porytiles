#include <gtest/gtest.h>

#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(PixelTileTests, AtAndSetShouldWork)
{
    PixelTile<IndexPixel> tile{};

    EXPECT_EQ(0, tile.at(0).index());
    EXPECT_EQ(0, tile.at(63).index());
    EXPECT_DEATH(std::ignore = tile.at(tile::size_pix), "index out of bounds: 64");
    EXPECT_DEATH(std::ignore = tile.at(tile::side_length_pix, 2), "row index out of bounds: 8");
    EXPECT_DEATH(std::ignore = tile.at(0, tile::side_length_pix), "col index out of bounds: 8");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6).index());

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42).index());
}

TEST(PixelTileTests, IsTransparentShouldUseExtrinsicCorrectly)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentShouldUseAlphaCorrectly)
{
    // Default-constructed PixelTile<Rgba32> is zeroed, i.e. black and intrinsically transparent
    const PixelTile<Rgba32> tile{};
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentShouldUseMixedTransparencyCorrectly)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    // Set a pixel to non-magenta, but set alpha channel to transparent
    tile.set(12, Rgba32{22, 90, 144, Rgba32::alpha_transparent});

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}
