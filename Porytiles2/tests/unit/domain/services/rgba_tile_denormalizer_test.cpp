#include <gtest/gtest.h>

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/domain/services/rgba_tile_denormalizer.hpp"
#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"

using namespace porytiles2;

class RgbaTileDenormalizerTest : public ::testing::Test {
  protected:
    RgbaTileDenormalizer denormalizer_;
    RgbaTileNormalizer normalizer_;

    // Helper function to flip an RgbaTile
    [[nodiscard]] RgbaTile flip_rgba_tile(const RgbaTile &tile, bool h_flip, bool v_flip) const
    {
        const auto flipped_base = tile.flip(h_flip, v_flip);
        RgbaTile flipped_rgba_tile;
        for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
            flipped_rgba_tile.set(i, flipped_base.at(i));
        }
        return flipped_rgba_tile;
    }
};

TEST_F(RgbaTileDenormalizerTest, ShouldDenormalizeSingleColorTile)
{
    RgbaTile original_tile{};

    // Fill with a single non-transparent color
    const Rgba32 red{255, 0, 0, 255};
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, red);
    }

    // Normalize the tile first
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());

    // All pixels should be red
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(red, denormalized_tile.at(i));
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldDenormalizeTransparentTile)
{
    RgbaTile original_tile{};

    // Fill with transparent pixels
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, transparent);
    }

    // Normalize the tile first
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());

    // All pixels should be transparent
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(transparent, denormalized_tile.at(i));
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldDenormalizeMultiColorTile)
{
    RgbaTile original_tile{};

    // Create a tile with multiple colors in a pattern
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};
    const Rgba32 blue{0, 0, 255, 255};

    // Fill quarters with different colors
    for (std::size_t i = 0; i < 16; ++i) {
        original_tile.set(i, transparent);
    }
    for (std::size_t i = 16; i < 32; ++i) {
        original_tile.set(i, red);
    }
    for (std::size_t i = 32; i < 48; ++i) {
        original_tile.set(i, green);
    }
    for (std::size_t i = 48; i < 64; ++i) {
        original_tile.set(i, blue);
    }

    // Normalize the tile first
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());
    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile - should get the same result
    auto renormalized_result = normalizer_.normalize(denormalized_tile);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent in their pixel data and flips
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldDenormalizeWithExtrinsicTransparency)
{
    RgbaTile original_tile{};

    const Rgba32 magenta{255, 0, 255, 255}; // Extrinsic transparency
    const Rgba32 red{255, 0, 0, 255};

    // Fill with magenta and red
    for (std::size_t i = 0; i < RgbaTile::tile_size / 2; ++i) {
        original_tile.set(i, magenta);
    }
    for (std::size_t i = RgbaTile::tile_size / 2; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, red);
    }

    // Normalize with magenta as extrinsic transparency
    auto normalized_result = normalizer_.normalize(original_tile, magenta);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());
    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile with the same extrinsic transparency
    auto renormalized_result = normalizer_.normalize(denormalized_tile, magenta);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldBeInverseOfNormalizer)
{
    // Test that denormalize(normalize(tile)) gives back an equivalent tile
    RgbaTile original_tile{};

    // Create a distinctive asymmetric pattern that will test flip handling
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};

    // Fill with mostly transparent, but add some distinctive pattern
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(0, 0, red);   // Top-left
    original_tile.set(1, 0, green); // Second pixel in first row

    // Normalize then denormalize
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());

    const auto &normalized_tile = normalized_result.value();

    // Alternative way to verify: if we normalize the denormalized tile again,
    // we should get the same normalized tile
    auto renormalized_result = normalizer_.normalize(denormalized_tile);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent in their pixel data and flips
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldHandleMaximumColors)
{
    RgbaTile original_tile{};

    // Fill with exactly 15 unique non-transparent colors
    for (std::size_t i = 1; i <= 15; ++i) {
        const Rgba32 color{static_cast<std::uint8_t>(i * 16), 0, 0, 255};
        original_tile.set(i - 1, color);
    }

    // Fill remaining pixels with transparency
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 15; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, transparent);
    }

    // Normalize then denormalize
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto denormalized_tile = denormalizer_.denormalize(normalized_result.value());

    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile
    auto renormalized_result = normalizer_.normalize(denormalized_tile);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldPreserveFlipsInToRgbaPreservingFlips)
{
    RgbaTile original_tile{};

    // Create an asymmetric pattern to test flip preservation
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};

    // Fill with transparent
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(0, 0, red);   // Top-left
    original_tile.set(1, 0, green); // Second pixel in first row

    // Normalize the tile
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Convert to RGBA preserving flips
    const auto rgba_preserving_flips = denormalizer_.to_rgba_preserving_flips(normalized_tile);

    // The tile with preserved flips should match what we get by applying the same flips to the original
    RgbaTile expected_flipped_tile = flip_rgba_tile(original_tile, normalized_tile.h_flip(), normalized_tile.v_flip());

    // Verify each pixel matches
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(expected_flipped_tile.at(i), rgba_preserving_flips.at(i));
    }

    // Verify that the rgba_preserving_flips tile has the colors in the flipped positions
    // Since we know the normalized tile has both flips and the original had red at (0,0) and green at (1,0),
    // we can check specific expected positions in the flipped tile
    if (normalized_tile.h_flip() && normalized_tile.v_flip()) {
        // Both flips: (0,0) -> (7,7), (1,0) -> (6,7)
        EXPECT_EQ(red, rgba_preserving_flips.at(7, 7));
        EXPECT_EQ(green, rgba_preserving_flips.at(6, 7));
        EXPECT_EQ(transparent, rgba_preserving_flips.at(0, 0));
    }
    else if (normalized_tile.h_flip()) {
        // H flip only: (0,0) -> (0,7), (1,0) -> (1,7)
        EXPECT_EQ(red, rgba_preserving_flips.at(0, 7));
        EXPECT_EQ(green, rgba_preserving_flips.at(1, 7));
    }
    else if (normalized_tile.v_flip()) {
        // V flip only: (0,0) -> (7,0), (1,0) -> (6,0)
        EXPECT_EQ(red, rgba_preserving_flips.at(7, 0));
        EXPECT_EQ(green, rgba_preserving_flips.at(6, 0));
    }
    else {
        // No flips: positions should be the same
        EXPECT_EQ(red, rgba_preserving_flips.at(0, 0));
        EXPECT_EQ(green, rgba_preserving_flips.at(1, 0));
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldDifferentiateBetweenDenormalizeAndToRgbaPreservingFlips)
{
    RgbaTile original_tile{};

    // Create a pattern that will definitely be flipped during normalization
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 blue{0, 0, 255, 255};

    // Fill with transparent, put blue at bottom-right
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(7, 7, blue); // Bottom-right corner

    // Normalize the tile
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Get both versions
    const auto denormalized_tile = denormalizer_.denormalize(normalized_tile);
    const auto rgba_preserving_flips = denormalizer_.to_rgba_preserving_flips(normalized_tile);

    // The denormalized tile should match the original
    EXPECT_EQ(blue, denormalized_tile.at(7, 7));
    EXPECT_EQ(transparent, denormalized_tile.at(0, 0));

    // If flips were applied during normalization, the rgba_preserving_flips tile should be different
    if (normalized_tile.h_flip() || normalized_tile.v_flip()) {
        // The tiles should be different in this case
        bool tiles_are_different = false;
        for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
            if (denormalized_tile.at(i) != rgba_preserving_flips.at(i)) {
                tiles_are_different = true;
                break;
            }
        }
        EXPECT_TRUE(tiles_are_different);
    }
}

TEST_F(RgbaTileDenormalizerTest, ShouldWorkWithExtrinsicTransparencyPreservingFlips)
{
    RgbaTile original_tile{};

    const Rgba32 magenta{255, 0, 255, 255}; // Extrinsic transparency
    const Rgba32 yellow{255, 255, 0, 255};

    // Fill with magenta and yellow
    for (std::size_t i = 0; i < RgbaTile::tile_size / 2; ++i) {
        original_tile.set(i, magenta);
    }
    for (std::size_t i = RgbaTile::tile_size / 2; i < RgbaTile::tile_size; ++i) {
        original_tile.set(i, yellow);
    }

    // Normalize with magenta as extrinsic transparency
    auto normalized_result = normalizer_.normalize(original_tile, magenta);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Convert to RGBA preserving flips
    const auto rgba_preserving_flips = denormalizer_.to_rgba_preserving_flips(normalized_tile);

    // Verify correctness by renormalizing with the same extrinsic transparency
    auto renormalized_result = normalizer_.normalize(rgba_preserving_flips, magenta);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}
