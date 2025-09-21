#include "porytiles2/domain/services/rgba_tile_denormalizer.hpp"

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"

namespace porytiles2 {

namespace {

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

RgbaTile RgbaTileDenormalizer::denormalize(const NormalizedTile<Rgba32> &normalized_tile) const
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

RgbaTile RgbaTileDenormalizer::to_rgba_preserving_flips(const NormalizedTile<Rgba32> &normalized_tile) const
{
    // Convert IndexPixels to RGBA colors without applying any flip transformations
    // This preserves the normalized form, allowing inspection of the flipped state
    return convert_index_pixels_to_rgba(normalized_tile);
}

} // namespace porytiles2
