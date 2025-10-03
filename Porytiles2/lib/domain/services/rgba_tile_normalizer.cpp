#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <unordered_map>

#include "fmt/format.h"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

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
    for (std::size_t i = 0; i < Tile<PixelType>::tile_size; ++i) {
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

    for (std::size_t i = 0; i < Tile<IndexPixel>::tile_size; ++i) {
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
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
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
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
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
    for (std::size_t i = 0; i < RgbaTile::tile_size; ++i) {
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