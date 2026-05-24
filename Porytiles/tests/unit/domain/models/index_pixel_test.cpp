#include "gtest/gtest.h"

#include "porytiles/domain/models/index_pixel.hpp"

using namespace porytiles;

TEST(IndexPixelTests, DefaultIsTransparent)
{
    const IndexPixel default_pixel{};
    EXPECT_TRUE(default_pixel.is_transparent());
    EXPECT_EQ(default_pixel.index(), 0);
}

TEST(IndexPixelTests, ColorIndexStandardValues)
{
    // For standard 4-bit values (0-15), color_index() should equal index()
    for (std::size_t i = 0; i <= 15; ++i) {
        const IndexPixel pixel{i};
        EXPECT_EQ(pixel.color_index(), i);
        EXPECT_EQ(pixel.color_index(), pixel.index());
    }
}

TEST(IndexPixelTests, ColorIndexTrueColorValues)
{
    // For true-color encoded values, color_index() extracts lower 4 bits
    // Example: 0x23 = palette 2, color 3
    const IndexPixel pixel_0x23{0x23};
    EXPECT_EQ(pixel_0x23.color_index(), 3);

    // Example: 0xF5 = palette 15, color 5
    const IndexPixel pixel_0xF5{0xF5};
    EXPECT_EQ(pixel_0xF5.color_index(), 5);

    // Example: 0x10 = palette 1, color 0 (transparent within palette 1)
    const IndexPixel pixel_0x10{0x10};
    EXPECT_EQ(pixel_0x10.color_index(), 0);

    // Example: 0xFF = palette 15, color 15
    const IndexPixel pixel_0xFF{0xFF};
    EXPECT_EQ(pixel_0xFF.color_index(), 15);
}

TEST(IndexPixelTests, PaletteIndexStandardValues)
{
    // For standard 4-bit values (0-15), palette_index() should be 0
    for (std::size_t i = 0; i <= 15; ++i) {
        const IndexPixel pixel{i};
        EXPECT_EQ(pixel.palette_index(), 0);
    }
}

TEST(IndexPixelTests, PaletteIndexTrueColorValues)
{
    // For true-color encoded values, palette_index() extracts upper 4 bits
    // Example: 0x23 = palette 2, color 3
    const IndexPixel pixel_0x23{0x23};
    EXPECT_EQ(pixel_0x23.palette_index(), 2);

    // Example: 0xF5 = palette 15, color 5
    const IndexPixel pixel_0xF5{0xF5};
    EXPECT_EQ(pixel_0xF5.palette_index(), 15);

    // Example: 0x10 = palette 1, color 0
    const IndexPixel pixel_0x10{0x10};
    EXPECT_EQ(pixel_0x10.palette_index(), 1);

    // Example: 0xFF = palette 15, color 15
    const IndexPixel pixel_0xFF{0xFF};
    EXPECT_EQ(pixel_0xFF.palette_index(), 15);
}

TEST(IndexPixelTests, IsTransparentColorIndex)
{
    // Index 0 is transparent
    const IndexPixel pixel_0{0};
    EXPECT_TRUE(pixel_0.is_transparent());

    // Index 0x10 has color_index() == 0, so it's transparent even though raw index is 16
    const IndexPixel pixel_0x10{0x10};
    EXPECT_TRUE(pixel_0x10.is_transparent());
    EXPECT_EQ(pixel_0x10.index(), 0x10); // raw value is 16

    // Index 0x20 has color_index() == 0, so it's transparent
    const IndexPixel pixel_0x20{0x20};
    EXPECT_TRUE(pixel_0x20.is_transparent());

    // Index 0xF0 has color_index() == 0, so it's transparent
    const IndexPixel pixel_0xF0{0xF0};
    EXPECT_TRUE(pixel_0xF0.is_transparent());
}

TEST(IndexPixelTests, IsTransparentNonZero)
{
    // Index 1 is not transparent
    const IndexPixel pixel_1{1};
    EXPECT_FALSE(pixel_1.is_transparent());

    // Index 0x11 has color_index() == 1, so it's not transparent
    const IndexPixel pixel_0x11{0x11};
    EXPECT_FALSE(pixel_0x11.is_transparent());

    // Index 0x23 has color_index() == 3, so it's not transparent
    const IndexPixel pixel_0x23{0x23};
    EXPECT_FALSE(pixel_0x23.is_transparent());
}

TEST(IndexPixelTests, RoundTrip)
{
    // For any 8-bit value, (palette_index << 4) | color_index should equal index
    for (std::size_t i = 0; i <= 255; ++i) {
        const IndexPixel pixel{i};
        const std::size_t reconstructed = (pixel.palette_index() << 4) | pixel.color_index();
        EXPECT_EQ(reconstructed, pixel.index());
    }
}
