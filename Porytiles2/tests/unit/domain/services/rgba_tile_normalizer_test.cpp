#include <gtest/gtest.h>

#include <iostream>

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"

using namespace porytiles2;

class RgbaTileNormalizerTest : public ::testing::Test {
  protected:
    RgbaTileNormalizer normalizer_;
};

TEST_F(RgbaTileNormalizerTest, ShouldNormalizeSingleColorTile)
{
    RgbaTile tile{};

    // Fill with a single non-transparent color
    const Rgba32 red{255, 0, 0, 255};
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        tile.set(i, red);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_FALSE(normalized.h_flip());
    EXPECT_FALSE(normalized.v_flip());
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // All pixels should be index 1 (red), since 0 is transparency
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(1, normalized.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldNormalizeTransparentTile)
{
    RgbaTile tile{};

    // Fill with transparent pixels (default constructor creates transparent pixels)
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        tile.set(i, transparent);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_FALSE(normalized.h_flip());
    EXPECT_FALSE(normalized.v_flip());
    EXPECT_EQ(0, normalized.palette().size()); // no colors (transparency is separate)

    // All pixels should be index 0 (transparency)
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(0, normalized.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldChooseCorrectNormalFormWithFlips)
{
    RgbaTile tile{};

    // Create a pattern that has a clear normal form
    // Fill mostly with transparency, put red in top-left corner
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        tile.set(i, transparent);
    }
    tile.set(0, 0, red); // Top-left corner

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // Verify that exactly one pixel is red (index 1) and the rest are transparent (index 0)
    int red_count = 0;
    for (std::size_t i = 0; i < NormalizedTile<Rgba32>::tile_size; ++i) {
        if (normalized.at(i).index() == 1) {
            red_count++;
        }
    }
    EXPECT_EQ(1, red_count); // Exactly one red pixel

    /*
     * Normal form would be the hflipped and vflipped tile, i.e., the one with red at the very end of the pixel array.
     * The lexicographically "smallest" pixel array is one which maximizes the number of zero (transparent) pixels that
     * come before the one red pixel. In this case, that means the red pixel should be at the end of the pixel array,
     * i.e., tile position 7,7
     */
    EXPECT_TRUE(normalized.h_flip());
    EXPECT_TRUE(normalized.v_flip());
    EXPECT_EQ(normalized.color_at(NormalizedTile<Rgba32>::tile_size - 1), red);
    EXPECT_EQ(normalized.color_at(7, 7), red);
}

TEST_F(RgbaTileNormalizerTest, ShouldChooseFlippedNormalForm)
{
    RgbaTile tile{};

    // Create a pattern where the flipped version is lexicographically smaller
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        tile.set(i, transparent);
    }
    tile.set(7, 7, red); // Bottom-right corner

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // Verify that exactly one pixel is red (index 1) and the rest are transparent (index 0)
    int red_count = 0;
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        if (normalized.at(i).index() == 1) {
            red_count++;
        }
    }
    EXPECT_EQ(1, red_count); // Exactly one red pixel

    /*
     * Normal form would be the original tile in this case.
     */
    EXPECT_FALSE(normalized.h_flip());
    EXPECT_FALSE(normalized.v_flip());
    EXPECT_EQ(normalized.color_at(Tile<Rgba32>::tile_size - 1), red);
    EXPECT_EQ(normalized.color_at(7, 7), red);
}

TEST_F(RgbaTileNormalizerTest, ShouldHandleMaximumColors)
{
    RgbaTile tile{};

    // Fill with exactly 15 unique non-transparent colors
    // Start from i=1 to avoid (0,0,0) which matches default transparency color
    for (std::size_t i = 1; i <= 15; ++i) {
        const Rgba32 color{static_cast<std::uint8_t>(i * 16), 0, 0, 255};
        tile.set(i - 1, color);
    }

    // Fill remaining pixels with transparency
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 15; i < RgbaTile::tile_size; ++i) {
        tile.set(i, transparent);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(15, normalized.palette().size()); // 15 colors (transparency is separate)
}

TEST_F(RgbaTileNormalizerTest, ShouldFailWithTooManyColors)
{
    RgbaTile tile{};

    // Fill with 16 unique non-transparent colors (exceeds limit)
    // Start from i=1 and use a different increment to avoid transparency color
    for (std::size_t i = 1; i <= 16; ++i) {
        const Rgba32 color{static_cast<std::uint8_t>(i * 15), 0, 0, 255};
        tile.set(i - 1, color);
    }

    auto result = normalizer_.normalize(tile);

    EXPECT_FALSE(result.has_value());
    // Should contain error about too many colors
}

TEST_F(RgbaTileNormalizerTest, ShouldResolveColorsCorrectly)
{
    RgbaTile tile{};

    // Create a tile with multiple colors in a pattern
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};
    const Rgba32 blue{0, 0, 255, 255};

    // Fill first quarter with transparent
    for (std::size_t i = 0; i < 16; ++i) {
        tile.set(i, transparent);
    }
    // Fill second quarter with red
    for (std::size_t i = 16; i < 32; ++i) {
        tile.set(i, red);
    }
    // Fill third quarter with green
    for (std::size_t i = 32; i < 48; ++i) {
        tile.set(i, green);
    }
    // Fill fourth quarter with blue
    for (std::size_t i = 48; i < 64; ++i) {
        tile.set(i, blue);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(3, normalized.palette().size()); // red, green, blue (transparency is separate)

    // Verify color_at() returns correct colors
    // Note: colors in palette are sorted, so blue < green < red alphabetically by RGB values
    for (std::size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(transparent, normalized.color_at(i)); // Should resolve to transparent
    }
    for (std::size_t i = 16; i < 32; ++i) {
        EXPECT_EQ(red, normalized.color_at(i)); // Should resolve to red
    }
    for (std::size_t i = 32; i < 48; ++i) {
        EXPECT_EQ(green, normalized.color_at(i)); // Should resolve to green
    }
    for (std::size_t i = 48; i < 64; ++i) {
        EXPECT_EQ(blue, normalized.color_at(i)); // Should resolve to blue
    }

    // Also test row/col access
    EXPECT_EQ(transparent, normalized.color_at(0, 0));
    EXPECT_EQ(transparent, normalized.color_at(1, 7));
    EXPECT_EQ(red, normalized.color_at(2, 0));
    EXPECT_EQ(red, normalized.color_at(3, 7));
    EXPECT_EQ(green, normalized.color_at(4, 0));
    EXPECT_EQ(green, normalized.color_at(5, 7));
    EXPECT_EQ(blue, normalized.color_at(6, 0));
    EXPECT_EQ(blue, normalized.color_at(7, 7));
}

TEST_F(RgbaTileNormalizerTest, ShouldHandleExtrinsicTransparency)
{
    RgbaTile tile{};

    const Rgba32 magenta{255, 0, 255, 255}; // Extrinsic transparency
    const Rgba32 red{255, 0, 0, 255};

    // Fill with magenta and red
    for (std::size_t i = 0; i < RgbaTile::tile_size / 2; ++i) {
        tile.set(i, magenta);
    }
    for (std::size_t i = RgbaTile::tile_size / 2; i < RgbaTile::tile_size; ++i) {
        tile.set(i, red);
    }

    auto result = normalizer_.normalize(tile, magenta); // Treat magenta as transparent

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (magenta/transparency is separate)

    // First half should be index 0 (transparent), second half should be index 1 (red)
    for (std::size_t i = 0; i < RgbaTile::tile_size / 2; ++i) {
        EXPECT_EQ(0, normalized.at(i).index());
    }
    for (std::size_t i = RgbaTile::tile_size / 2; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(1, normalized.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, DocumentsPanicForPaletteContainingTransparentColor)
{
    // This test documents the defensive panic on (or near, if code has changed) line 83 of rgba_tile_normalizer.cpp.
    //
    // ANALYSIS: The panic appears to be unreachable under the current implementation:
    // - build_normalized_palette() only adds colors where !pixel.is_transparent(extrinsic_transparency)
    // - convert_to_indexed() panics if color.is_transparent(extrinsic_transparency)
    // - These are logically opposite conditions
    //
    // CONCLUSION: The panic serves as a defensive assertion against future code changes
    // that might introduce bugs in the palette building logic. It should remain in place
    // as a safeguard, even though it's currently unreachable.
    //
    // This test verifies that edge cases work correctly and documents the invariant.

    // For now, we can test edge cases that should work correctly:

    RgbaTile tile{};

    // Test with a color that has RGB matching extrinsic transparency but different alpha
    constexpr Rgba32 red_opaque{255, 0, 0, 255};
    constexpr Rgba32 red_transparent{255, 0, 0, 0}; // Same RGB, different alpha

    // Fill tile with the opaque version
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        tile.set(i, red_opaque);
    }

    // Normalize with the transparent version as extrinsic transparency
    // This should work correctly - red_opaque should be filtered out as transparent
    auto result = normalizer_.normalize(tile, red_transparent);
    ASSERT_TRUE(result.has_value());

    // The palette should be empty since all pixels match extrinsic transparency (ignoring alpha)
    const auto &normalized = result.value();
    EXPECT_EQ(0, normalized.palette().size());

    // All pixels should be index 0 (transparent)
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(0, normalized.at(i).index());
    }

    // NOTE: The actual panic can only be triggered if there's a bug in the implementation
    // that allows transparent colors to enter the palette. Since the public API prevents
    // this through proper filtering, a direct test of the panic would require:
    // 1. A bug in build_normalized_palette filtering
    // 2. Direct access to convert_to_indexed with a malformed palette
    // 3. A mock/test version that bypasses the normal safeguards
    //
    // The panic serves as a runtime assertion to catch such bugs during development.
}