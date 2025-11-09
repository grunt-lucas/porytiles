# Code

```c++
#pragma once

#include <iterator>
#include <tuple>

#include "fmt/format.h"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/normalized_pal.hpp"
#include "porytiles2/domain/model/supports_transparency.hpp"
#include "porytiles2/domain/model/tile.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief A tile with normalized pixel data and flip state information.
 *
 * @details
 * NormalizedTile extends the base Tile class to store IndexPixel data along with horizontal and vertical flip states.
 * It also maintains an internal NormalizedPal.
 *
 * @tparam ColorType The type of color objects used in the tile's NormalizedPal.
 */
template <typename ColorType>
    requires SupportsTransparency<ColorType>
class NormalizedTile final : public Tile<IndexPixel> {
  public:
    /**
     * @brief Constructs a NormalizedTile with the specified flip states and extrinsic transparency color.
     *
     * @param h_flip Whether the tile should be horizontally flipped
     * @param v_flip Whether the tile should be vertically flipped
     * @param extrinsic The color that represents transparency in this NormalizedTile
     */
    NormalizedTile(bool h_flip, bool v_flip, const ColorType &extrinsic)
        : h_flip_{h_flip}, v_flip_{v_flip}, pal_{extrinsic}
    {
    }

    /**
     * @brief Returns the horizontal flip state of the tile.
     *
     * @return True if the tile is horizontally flipped, false otherwise
     */
    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    /**
     * @brief Returns the vertical flip state of the tile.
     *
     * @return True if the tile is vertically flipped, false otherwise
     */
    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

    /**
     * @brief Returns a const reference to the NormalizedPal.
     *
     * @return A const reference to the tile's palette
     */
    [[nodiscard]] const NormalizedPal<ColorType> &palette() const
    {
        return pal_;
    }

    /**
     * @brief Returns a mutable reference to the NormalizedPal.
     *
     * @return A mutable reference to the tile's palette
     */
    [[nodiscard]] NormalizedPal<ColorType> &palette()
    {
        return pal_;
    }

    /**
     * @brief Returns the resolved color at the specified linear index.
     *
     * @details
     * Resolves the IndexPixel at the given position to the actual color from the palette. IndexPixel value 0 maps to
     * the extrinsic transparency color, while values 1+ map to palette colors with a 1-based indexing (IndexPixel 1 =
     * first palette color).
     *
     * @param i The linear index (0-63 for an 8x8 tile)
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &color_at(std::size_t i) const
    {
        return resolve_index_pixel(at(i));
    }

    /**
     * @brief Returns the resolved color at the specified row and column.
     *
     * @details
     * Resolves the IndexPixel at the given position to the actual color from the palette. IndexPixel value 0 maps to
     * the extrinsic transparency color, while values 1+ map to palette colors with a 1-based indexing (IndexPixel 1 =
     * first palette color).
     *
     * @param row The row index (0-7)
     * @param col The column index (0-7)
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &color_at(std::size_t row, std::size_t col) const
    {
        return resolve_index_pixel(at(row, col));
    }

    /**
     * @brief Compares two NormalizedTile objects for ordering.
     *
     * @details
     * Comparison is performed on the tile pixel data (via base class) and the palette. Flip states are not included in
     * the comparison since tiles with the same pixel pattern and palette should be considered equivalent regardless of
     * how they were flipped during normalization.
     *
     * @param other The other NormalizedTile to compare against
     * @return Ordering relationship between this tile and other
     */
    auto operator<=>(const NormalizedTile &other) const
    {
        // First compare the tile pixel data
        const auto base_cmp = static_cast<const Tile &>(*this) <=> static_cast<const Tile &>(other);
        if (base_cmp != 0) {
            return base_cmp;
        }

        // Then compare palettes (flip states are not compared)
        return pal_.colors() <=> other.pal_.colors();
    }

    bool operator==(const NormalizedTile &other) const = default;

  private:
    /**
     * @brief Resolves an IndexPixel to its corresponding color in the palette.
     *
     * @details
     * IndexPixel value 0 maps to the extrinsic transparency color, while values 1+ map to palette colors with 1-based
     * indexing (IndexPixel 1 = first palette color).
     *
     * @param pixel The IndexPixel to resolve
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &resolve_index_pixel(const IndexPixel &pixel) const
    {
        const auto index = pixel.index();

        if (index == 0) {
            return pal_.extrinsic_transparency();
        }

        const auto palette_index = index - 1;
        if (palette_index >= pal_.size()) {
            panic(fmt::format("IndexPixel value {} out of bounds for palette of size {}", index, pal_.size()));
        }

        auto it = std::next(pal_.colors().begin(), palette_index);
        return *it;
    }

    bool h_flip_;
    bool v_flip_;
    NormalizedPal<ColorType> pal_;
};

} // namespace porytiles2

#pragma once

#include <set>

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/supports_transparency.hpp"

namespace porytiles2 {

/**
 * @brief A palette that stores color types in ascending order.
 *
 * @tparam ColorType The type of color objects to store in the palette
 */
template <typename ColorType>
    requires SupportsTransparency<ColorType>
class NormalizedPal {
  public:
    /**
     * @brief Constructs a NormalizedPal with the specified extrinsic transparency color.
     *
     * @details
     * The extrinsic transparency color is used to identify which color in the source data should be treated as
     * transparent when building this particular NormalizedPal.
     *
     * @param extrinsic The color that represents transparency for this particular NormalizedPal
     */
    explicit NormalizedPal(const ColorType &extrinsic) : extrinsic_transparency_{extrinsic} {}

    /**
     * @brief Inserts a color into the palette.
     *
     * @details
     * Colors are automatically stored in sorted order using std::set.
     *
     * @param color The color to insert into the palette
     */
    void insert(const ColorType &color)
    {
        colors_.insert(color);
    }

    /**
     * @brief Returns the number of colors in the palette.
     *
     * @return The number of unique colors stored in the palette
     */
    [[nodiscard]] std::size_t size() const
    {
        return colors_.size();
    }

    [[nodiscard]] const ColorType &extrinsic_transparency() const
    {
        return extrinsic_transparency_;
    }

    /**
     * @brief Returns a const reference to the underlying color set.
     *
     * @details
     * The colors are stored in ascending order as determined by ColorType's comparison operator.
     *
     * @return A const reference to the set containing all colors
     */
    [[nodiscard]] const std::set<ColorType> &colors() const
    {
        return colors_;
    }

  private:
    ColorType extrinsic_transparency_;
    std::set<ColorType> colors_;
};

} // namespace porytiles2

#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <unordered_map>

#include "fmt/format.h"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Builds a normalized palette from the unique colors in a tile.
 *
 * @param tile The source tile
 * @param extrinsic_transparency The extrinsic transparency color
 * @tparam PixelType The pixel type of the input tile
 * @return ChainableResult containing the palette or an error if too many colors
 */
template <typename PixelType>
    requires SupportsTransparency<PixelType>
[[nodiscard]] ChainableResult<NormalizedPal<PixelType>>
build_normalized_palette(const Tile<PixelType> &tile, const PixelType &extrinsic_transparency)
{
    std::set<PixelType> unique_colors{};

    // Collect all unique non-transparent colors
    for (std::size_t i = 0; i < tile_size; ++i) {
        const PixelType pixel = tile.at(i);
        if (!pixel.is_transparent(extrinsic_transparency)) {
            unique_colors.insert(pixel);
        }
    }

    // Check color count limit (15 non-transparent + 1 transparent = 16 total)
    if (unique_colors.size() > 15) {
        return FormattableError{fmt::format(
            "tile had {} unique colors, but maximum allowed is 15 (plus transparency)", unique_colors.size())};
    }

    NormalizedPal<PixelType> palette{extrinsic_transparency};

    // Insert all non-transparent colors
    for (const auto &color : unique_colors) {
        palette.insert(color);
    }

    return palette;
}

/**
 * @brief Converts RGBA pixels to index pixels using the given palette.
 *
 * @param rgba_tile The source RGBA tile
 * @param palette The normalized palette to use for conversion
 * @param h_flip Whether to apply horizontal flip during conversion
 * @param v_flip Whether to apply vertical flip during conversion
 * @param extrinsic_transparency The extrinsic transparency color
 * @return The converted tile with IndexPixel data
 */
[[nodiscard]] Tile<IndexPixel> convert_to_indexed(
    const RgbaTile &rgba_tile,
    const NormalizedPal<Rgba32> &palette,
    bool h_flip,
    bool v_flip,
    const Rgba32 &extrinsic_transparency)
{
    // Create a mapping from color to palette index for fast lookup
    // Transparency color should always be at index 0
    std::unordered_map<Rgba32, unsigned int> color_to_index;

    // Assign transparency color to index 0
    color_to_index[extrinsic_transparency] = 0;

    // Assign all other colors to subsequent indices
    unsigned int index = 1;
    for (const auto &color : palette.colors()) {
        if (color.is_transparent(extrinsic_transparency)) {
            panic(fmt::format("palette contains transparent color {} at index {}", color.to_jasc_str(), index));
        }
        color_to_index[color] = index++;
    }

    // Apply flip to the input tile, then process in normal order
    const auto flipped_tile = rgba_tile.flip(h_flip, v_flip);

    Tile<IndexPixel> indexed_tile;

    for (std::size_t i = 0; i < tile_size; ++i) {
        const Rgba32 src_pixel = flipped_tile.at(i);

        // Determine the palette index for this pixel
        unsigned int palette_index = 0; // Transparent color is always at index 0
        // If not transparent, get the index into the palette
        if (!src_pixel.is_transparent(extrinsic_transparency)) {
            auto it = color_to_index.find(src_pixel);
            if (it == color_to_index.end()) {
                // Debug: This should never happen
                panic("it == color_to_index.end()");
            }
            palette_index = it->second;
        }

        indexed_tile.set(i, IndexPixel{palette_index});
    }

    return indexed_tile;
}

/**
 * @brief Creates a candidate NormalizedTile with the specified flip states.
 *
 * @param rgba_tile The source RGBA tile
 * @param h_flip Whether to apply horizontal flip
 * @param v_flip Whether to apply vertical flip
 * @param extrinsic_transparency The extrinsic transparency color
 * @return ChainableResult containing the candidate tile or an error
 */
[[nodiscard]] ChainableResult<NormalizedTile<Rgba32>>
create_candidate(const RgbaTile &rgba_tile, bool h_flip, bool v_flip, const Rgba32 &extrinsic_transparency)
{
    // First build the normalized palette
    PT_TRY_ASSIGN_CHAIN_ERR(
        palette,
        build_normalized_palette<Rgba32>(rgba_tile, extrinsic_transparency),
        "failed to build normalized palette",
        NormalizedTile<Rgba32>);

    // Convert to indexed tile with the specified flip
    auto indexed_tile = convert_to_indexed(rgba_tile, palette, h_flip, v_flip, extrinsic_transparency);

    // Create the normalized tile
    NormalizedTile normalized_tile{h_flip, v_flip, extrinsic_transparency};

    // Copy the indexed pixel data
    for (std::size_t i = 0; i < tile_size; ++i) {
        normalized_tile.set(i, indexed_tile.at(i));
    }

    // Set the palette
    normalized_tile.palette() = palette;

    return normalized_tile;
}

/**
 * @brief Converts IndexPixels in a NormalizedTile back to RGBA colors using the palette.
 *
 * @details
 * This helper function performs the core conversion from IndexPixels to RGBA colors
 * without applying any flip transformations.
 *
 * @param normalized_tile The normalized tile containing IndexPixels and palette
 * @return RgbaTile with RGBA colors corresponding to the IndexPixels
 */
[[nodiscard]] RgbaTile convert_index_pixels_to_rgba(const NormalizedTile<Rgba32> &normalized_tile)
{
    RgbaTile rgba_tile;

    // Convert each IndexPixel back to its corresponding RGBA color
    for (std::size_t i = 0; i < tile_size; ++i) {
        const Rgba32 color = normalized_tile.color_at(i);
        rgba_tile.set(i, color);
    }

    return rgba_tile;
}

/**
 * @brief Applies flip transformations to an RgbaTile and returns a new RgbaTile.
 *
 * @details
 * Helper function to apply horizontal and/or vertical flips to an RgbaTile.
 * This handles the type conversion from Tile<Rgba32> to RgbaTile.
 *
 * @param rgba_tile The tile to flip
 * @param h_flip Whether to apply horizontal flip
 * @param v_flip Whether to apply vertical flip
 * @return New RgbaTile with flips applied
 */
[[nodiscard]] RgbaTile apply_flips_to_rgba_tile(const RgbaTile &rgba_tile, bool h_flip, bool v_flip)
{
    if (!h_flip && !v_flip) {
        return rgba_tile;
    }

    const auto flipped_base = rgba_tile.flip(h_flip, v_flip);
    RgbaTile flipped_rgba_tile;
    for (std::size_t i = 0; i < tile_size; ++i) {
        flipped_rgba_tile.set(i, flipped_base.at(i));
    }
    return flipped_rgba_tile;
}

} // anonymous namespace

ChainableResult<NormalizedTile<Rgba32>>
RgbaTileNormalizer::normalize(const RgbaTile &rgba_tile, const Rgba32 &extrinsic_transparency) const
{
    /*
     * TODO: should we clamp up all non-255 alphas on the incoming tile here? That is, we have an earlier step that
     * warns the user if they have supplied pixels with alpha that wasn't 0 or 255. Should we have that step error so
     * things are guaranteed to be 0 or 255 here, and we can panic otherwise? Or should we clamp them up here instead?
     */

    // Create four candidate tiles with different flip combinations
    PT_TRY_ASSIGN_CHAIN_ERR(
        no_flip,
        create_candidate(rgba_tile, false, false, extrinsic_transparency),
        "failed to create no-flip candidate",
        NormalizedTile<Rgba32>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        h_flip,
        create_candidate(rgba_tile, true, false, extrinsic_transparency),
        "failed to create h-flip candidate",
        NormalizedTile<Rgba32>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        v_flip,
        create_candidate(rgba_tile, false, true, extrinsic_transparency),
        "failed to create v-flip candidate",
        NormalizedTile<Rgba32>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        both_flip,
        create_candidate(rgba_tile, true, true, extrinsic_transparency),
        "failed to create both-flip candidate",
        NormalizedTile<Rgba32>);

    // Find the lexicographically smallest candidate
    std::array candidates = {no_flip, h_flip, v_flip, both_flip};

    // Use traditional min_element with custom comparator
    const auto min_candidate =
        std::ranges::min_element(candidates, [](const NormalizedTile<Rgba32> &a, const NormalizedTile<Rgba32> &b) {
            // Compare the pixel data first (this is what matters for normalization)
            return static_cast<const Tile<IndexPixel> &>(a) < static_cast<const Tile<IndexPixel> &>(b);
        });

    return *min_candidate;
}

ChainableResult<std::vector<NormalizedTile<Rgba32>>> RgbaTileNormalizer::batch_normalize(
    const std::vector<RgbaMetatile> &metatiles, const Rgba32 &extrinsic_transparency) const
{
    // Compute NormalizedTiles from the input metatiles
    std::vector<NormalizedTile<Rgba32>> norm_tiles{};
    for (const auto &metatile : metatiles) {
        // Combine all three layers into a single range
        std::array layers = {
            std::ranges::ref_view{metatile.bottom()},
            std::ranges::ref_view{metatile.middle()},
            std::ranges::ref_view{metatile.top()}};
        auto all_tiles = layers | std::views::join;

        std::size_t counter = 0;
        for (const auto &tile : all_tiles) {
            // Determine layer: 0 = bottom, 1 = middle, 2 = top
            std::size_t layer_index = counter / 4;
            std::size_t tile_index = counter % 4;

            const auto &norm_result = normalize(RgbaTile{tile}, extrinsic_transparency);
            if (!norm_result.has_value()) {
                // Better diagnostics with layer and tile indices
                const std::string layer_name = layer_index == 0 ? "bottom" : layer_index == 1 ? "middle" : "top";
                return ChainableResult<std::vector<NormalizedTile<Rgba32>>>::chain_together(
                    FormattableError{
                        "normalization failed: {} layer, tile {}",
                        FormatParam{layer_name, Style::bold},
                        FormatParam{tile_index, Style::bold}},
                    norm_result);
            }
            norm_tiles.push_back(norm_result.value());
            ++counter;
        }
    }
    return norm_tiles;
}

RgbaTile RgbaTileNormalizer::denormalize(const NormalizedTile<Rgba32> &normalized_tile) const
{
    // Convert IndexPixels to RGBA colors
    RgbaTile rgba_tile = convert_index_pixels_to_rgba(normalized_tile);

    // Apply the reverse of the flips that were applied during normalization
    // If the normalized tile has flips, we need to "undo" them to get back the original
    const bool h_flip = normalized_tile.h_flip();
    const bool v_flip = normalized_tile.v_flip();

    rgba_tile = apply_flips_to_rgba_tile(rgba_tile, h_flip, v_flip);

    return rgba_tile;
}

RgbaTile RgbaTileNormalizer::denormalize_preserving_flips(const NormalizedTile<Rgba32> &normalized_tile) const
{
    // Convert IndexPixels to RGBA colors without applying any flip transformations
    // This preserves the normalized form, allowing inspection of the flipped state
    return convert_index_pixels_to_rgba(normalized_tile);
}

} // namespace porytiles2
```

# Tests
```c++
#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <set>

#include "porytiles2/domain/model/tile/rgba_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/services/rgba_image_tileizer.hpp"
#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"

using namespace porytiles2;

class RgbaTileNormalizerTest : public ::testing::Test {
  protected:
    RgbaTileNormalizer normalizer_{};

    // Helper function to flip an RgbaTile
    [[nodiscard]] RgbaTile flip_rgba_tile(const RgbaTile &tile, bool h_flip, bool v_flip) const
    {
        const auto flipped_base = tile.flip(h_flip, v_flip);
        RgbaTile flipped_rgba_tile;
        for (std::size_t i = 0; i < tile_size; ++i) {
            flipped_rgba_tile.set(i, flipped_base.at(i));
        }
        return flipped_rgba_tile;
    }
};

TEST_F(RgbaTileNormalizerTest, ShouldNormalizeSingleColorTile)
{
    RgbaTile tile{};

    // Fill with a single non-transparent color
    const Rgba32 red{255, 0, 0, 255};
    for (std::size_t i = 0; i < tile_size; ++i) {
        tile.set(i, red);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_FALSE(normalized.h_flip());
    EXPECT_FALSE(normalized.v_flip());
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // All pixels should be index 1 (red), since 0 is transparency
    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(1, normalized.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldNormalizeTransparentTile)
{
    RgbaTile tile{};

    // Fill with transparent pixels (default constructor creates transparent pixels)
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 0; i < tile_size; ++i) {
        tile.set(i, transparent);
    }

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_FALSE(normalized.h_flip());
    EXPECT_FALSE(normalized.v_flip());
    EXPECT_EQ(0, normalized.palette().size()); // no colors (transparency is separate)

    // All pixels should be index 0 (transparency)
    for (std::size_t i = 0; i < tile_size; ++i) {
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

    for (std::size_t i = 0; i < tile_size; ++i) {
        tile.set(i, transparent);
    }
    tile.set(0, 0, red); // Top-left corner

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // Verify that exactly one pixel is red (index 1) and the rest are transparent (index 0)
    int red_count = 0;
    for (std::size_t i = 0; i < tile_size; ++i) {
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
    EXPECT_EQ(normalized.color_at(tile_size - 1), red);
    EXPECT_EQ(normalized.color_at(7, 7), red);
}

TEST_F(RgbaTileNormalizerTest, ShouldChooseFlippedNormalForm)
{
    RgbaTile tile{};

    // Create a pattern where the flipped version is lexicographically smaller
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};

    for (std::size_t i = 0; i < tile_size; ++i) {
        tile.set(i, transparent);
    }
    tile.set(7, 7, red); // Bottom-right corner

    auto result = normalizer_.normalize(tile);

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (transparency is separate)

    // Verify that exactly one pixel is red (index 1) and the rest are transparent (index 0)
    int red_count = 0;
    for (std::size_t i = 0; i < tile_size; ++i) {
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
    EXPECT_EQ(normalized.color_at(tile_size - 1), red);
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
    for (std::size_t i = 15; i < tile_size; ++i) {
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
    for (std::size_t i = 0; i < tile_size / 2; ++i) {
        tile.set(i, magenta);
    }
    for (std::size_t i = tile_size / 2; i < tile_size; ++i) {
        tile.set(i, red);
    }

    auto result = normalizer_.normalize(tile, magenta); // Treat magenta as transparent

    ASSERT_TRUE(result.has_value());

    const auto &normalized = result.value();
    EXPECT_EQ(1, normalized.palette().size()); // red (magenta/transparency is separate)

    // First half should be index 0 (transparent), second half should be index 1 (red)
    for (std::size_t i = 0; i < tile_size / 2; ++i) {
        EXPECT_EQ(0, normalized.at(i).index());
    }
    for (std::size_t i = tile_size / 2; i < tile_size; ++i) {
        EXPECT_EQ(1, normalized.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldForceAllNotFullyTransparentAlphaValuesToOpaque)
{
    // TODO: Uncomment this test once we decide what to do about alpha clamping. See TODO comment in
    // RgbaTileNormalizer::normalize

    // RgbaTile tile{};

    // const Rgba32 transparent{0, 0, 0, 0};
    // const Rgba32 magenta{255, 0, 255};
    // const Rgba32 red1{255, 0, 0, 255};
    // const Rgba32 red2{255, 0, 0, 128};
    // const Rgba32 red3{255, 0, 0, 20};

    // // Fill with reds
    // for (std::size_t i = 0; i < RgbaTile::tile_size / 2; i += 2) {
    //     tile.set(i, red1);
    //     tile.set(i + 1, red2);
    // }
    // for (std::size_t i = RgbaTile::tile_size / 2; i < RgbaTile::tile_size; ++i) {
    //     tile.set(i, red3);
    // }

    // // Make a few pixels truly transparent
    // tile.set(0, transparent);
    // tile.set(1, transparent);
    // tile.set(63, magenta);

    // auto result = normalizer_.normalize(tile, magenta); // Treat magenta as transparent

    // ASSERT_TRUE(result.has_value());

    // const auto &normalized = result.value();

    // /*
    //  * Palette should have size 1. All reds should get regularized. The transparent and magenta pixels will be
    //  treated
    //  * as transparent and don't contribute to the palette size.
    //  */
    // EXPECT_EQ(1, normalized.palette().size());
}

TEST_F(RgbaTileNormalizerTest, DocumentsPanicForPaletteContainingTransparentColor)
{
    // This test documents the defensive panic on (or near, if code has changed) line 84 of rgba_tile_normalizer.cpp.
    //
    // ANALYSIS: The panic appears to be unreachable under the current implementation:
    // - build_normalized_palette() only adds colors where !check_transparency(pixel, extrinsic_transparency)
    // - convert_to_indexed() panics if check_transparency(color, extrinsic_transparency)
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
    for (std::size_t i = 0; i < tile_size; ++i) {
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
    for (std::size_t i = 0; i < tile_size; ++i) {
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

TEST_F(RgbaTileNormalizerTest, ShouldDenormalizeSingleColorTile)
{
    RgbaTile original_tile{};

    // Fill with a single non-transparent color
    const Rgba32 red{255, 0, 0, 255};
    for (std::size_t i = 0; i < tile_size; ++i) {
        original_tile.set(i, red);
    }

    // Normalize the tile first
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());

    // All pixels should be red
    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(red, denormalized_tile.at(i));
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldDenormalizeTransparentTile)
{
    RgbaTile original_tile{};

    // Fill with transparent pixels
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 0; i < tile_size; ++i) {
        original_tile.set(i, transparent);
    }

    // Normalize the tile first
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());

    // All pixels should be transparent
    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(transparent, denormalized_tile.at(i));
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldDenormalizeMultiColorTile)
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
    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());
    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile - should get the same result
    auto renormalized_result = normalizer_.normalize(denormalized_tile);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent in their pixel data and flips
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldDenormalizeWithExtrinsicTransparency)
{
    RgbaTile original_tile{};

    const Rgba32 magenta{255, 0, 255, 255}; // Extrinsic transparency
    const Rgba32 red{255, 0, 0, 255};

    // Fill with magenta and red
    for (std::size_t i = 0; i < tile_size / 2; ++i) {
        original_tile.set(i, magenta);
    }
    for (std::size_t i = tile_size / 2; i < tile_size; ++i) {
        original_tile.set(i, red);
    }

    // Normalize with magenta as extrinsic transparency
    auto normalized_result = normalizer_.normalize(original_tile, magenta);
    ASSERT_TRUE(normalized_result.has_value());

    // Denormalize back to RGBA
    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());
    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile with the same extrinsic transparency
    auto renormalized_result = normalizer_.normalize(denormalized_tile, magenta);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldBeInverseOfNormalizer)
{
    // Test that denormalize(normalize(tile)) gives back an equivalent tile
    RgbaTile original_tile{};

    // Create a distinctive asymmetric pattern that will test flip handling
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};

    // Fill with mostly transparent, but add some distinctive pattern
    for (std::size_t i = 0; i < tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(0, 0, red);   // Top-left
    original_tile.set(1, 0, green); // Second pixel in first row

    // Normalize then denormalize
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());

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

    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldHandleMaximumColorsInDenormalization)
{
    RgbaTile original_tile{};

    // Fill with exactly 15 unique non-transparent colors
    for (std::size_t i = 1; i <= 15; ++i) {
        const Rgba32 color{static_cast<std::uint8_t>(i * 16), 0, 0, 255};
        original_tile.set(i - 1, color);
    }

    // Fill remaining pixels with transparency
    const Rgba32 transparent{0, 0, 0, 0};
    for (std::size_t i = 15; i < tile_size; ++i) {
        original_tile.set(i, transparent);
    }

    // Normalize then denormalize
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto denormalized_tile = normalizer_.denormalize(normalized_result.value());

    const auto &normalized_tile = normalized_result.value();

    // Verify correctness by renormalizing the denormalized tile
    auto renormalized_result = normalizer_.normalize(denormalized_tile);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldPreserveFlipsInDenormalizePreservingFlips)
{
    RgbaTile original_tile{};

    // Create an asymmetric pattern to test flip preservation
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 red{255, 0, 0, 255};
    const Rgba32 green{0, 255, 0, 255};

    // Fill with transparent
    for (std::size_t i = 0; i < tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(0, 0, red);   // Top-left
    original_tile.set(1, 0, green); // Second pixel in first row

    // Normalize the tile
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Convert to RGBA preserving flips
    const auto rgba_preserving_flips = normalizer_.denormalize_preserving_flips(normalized_tile);

    // The tile with preserved flips should match what we get by applying the same flips to the original
    RgbaTile expected_flipped_tile = flip_rgba_tile(original_tile, normalized_tile.h_flip(), normalized_tile.v_flip());

    // Verify each pixel matches
    for (std::size_t i = 0; i < tile_size; ++i) {
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

TEST_F(RgbaTileNormalizerTest, ShouldDifferentiateBetweenDenormalizeAndDenormalizePreservingFlips)
{
    RgbaTile original_tile{};

    // Create a pattern that will definitely be flipped during normalization
    const Rgba32 transparent{0, 0, 0, 0};
    const Rgba32 blue{0, 0, 255, 255};

    // Fill with transparent, put blue at bottom-right
    for (std::size_t i = 0; i < tile_size; ++i) {
        original_tile.set(i, transparent);
    }
    original_tile.set(7, 7, blue); // Bottom-right corner

    // Normalize the tile
    auto normalized_result = normalizer_.normalize(original_tile);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Get both versions
    const auto denormalized_tile = normalizer_.denormalize(normalized_tile);
    const auto rgba_preserving_flips = normalizer_.denormalize_preserving_flips(normalized_tile);

    // The denormalized tile should match the original
    EXPECT_EQ(blue, denormalized_tile.at(7, 7));
    EXPECT_EQ(transparent, denormalized_tile.at(0, 0));

    // If flips were applied during normalization, the rgba_preserving_flips tile should be different
    if (normalized_tile.h_flip() || normalized_tile.v_flip()) {
        // The tiles should be different in this case
        bool tiles_are_different = false;
        for (std::size_t i = 0; i < tile_size; ++i) {
            if (denormalized_tile.at(i) != rgba_preserving_flips.at(i)) {
                tiles_are_different = true;
                break;
            }
        }
        EXPECT_TRUE(tiles_are_different);
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldWorkWithExtrinsicTransparencyPreservingFlips)
{
    RgbaTile original_tile{};

    const Rgba32 magenta{255, 0, 255, 255}; // Extrinsic transparency
    const Rgba32 yellow{255, 255, 0, 255};

    // Fill with magenta and yellow
    for (std::size_t i = 0; i < tile_size / 2; ++i) {
        original_tile.set(i, magenta);
    }
    for (std::size_t i = tile_size / 2; i < tile_size; ++i) {
        original_tile.set(i, yellow);
    }

    // Normalize with magenta as extrinsic transparency
    auto normalized_result = normalizer_.normalize(original_tile, magenta);
    ASSERT_TRUE(normalized_result.has_value());

    const auto &normalized_tile = normalized_result.value();

    // Convert to RGBA preserving flips
    const auto rgba_preserving_flips = normalizer_.denormalize_preserving_flips(normalized_tile);

    // Verify correctness by renormalizing with the same extrinsic transparency
    auto renormalized_result = normalizer_.normalize(rgba_preserving_flips, magenta);
    ASSERT_TRUE(renormalized_result.has_value());

    const auto &renormalized_tile = renormalized_result.value();

    // The normalized tiles should be equivalent
    EXPECT_EQ(normalized_tile.h_flip(), renormalized_tile.h_flip());
    EXPECT_EQ(normalized_tile.v_flip(), renormalized_tile.v_flip());
    EXPECT_EQ(normalized_tile.palette().size(), renormalized_tile.palette().size());

    for (std::size_t i = 0; i < tile_size; ++i) {
        EXPECT_EQ(normalized_tile.at(i).index(), renormalized_tile.at(i).index());
    }
}

TEST_F(RgbaTileNormalizerTest, ShouldHandleIssue0118EdgeCaseCorrectly)
{
    // https://github.com/grunt-lucas/porytiles/issues/118
    const PngRgbaImageLoader loader{};
    const RgbaImageTileizer tileizer{};

    auto input_png_result = loader.load_from_file("Resources/Tests/unit/domain/services/normalization_edge_case.png");
    ASSERT_TRUE(input_png_result.has_value()) << "Failed to load edge case test image";

    const auto tiles_result = tileizer.tileize(*input_png_result.value());
    ASSERT_TRUE(tiles_result.has_value()) << "Failed to tileize edge case test image";

    const auto &tiles = tiles_result.value();

    std::set<NormalizedTile<Rgba32>> normalized_tiles;

    for (const auto &tile : tiles) {
        auto normalized_result = normalizer_.normalize(tile);
        ASSERT_TRUE(normalized_result.has_value()) << "Failed to normalize tile";
        const auto &normalized_tile = normalized_result.value();
        normalized_tiles.insert(normalized_tile);
    }

    ASSERT_EQ(1, normalized_tiles.size()) << "Expected all tiles to normalize to the same result";
}
```

