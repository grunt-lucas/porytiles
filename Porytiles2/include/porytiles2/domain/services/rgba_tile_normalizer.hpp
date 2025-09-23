#pragma once

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Domain service for normalizing and denormalizing RGBA tiles.
 *
 * @details
 * The RgbaTileNormalizer provides comprehensive functionality for converting between RgbaTile and NormalizedTile
 * formats.
 *
 * Normalization process involves:
 * - Constructing a NormalizedPal from the tile's unique colors
 * - Creating candidate NormalizedTiles with different flip combinations
 * - Selecting the lexicographically smallest tile as the normal form
 * - Validating that the tile has at most 16 unique colors (15 + transparency)
 *
 * Denormalization process supports two modes:
 * - Full denormalization that undoes the flips applied during normalization
 * - Partial conversion that preserves the normalized flip state for inspection
 */
class RgbaTileNormalizer {
  public:
    /**
     * @brief Normalizes an RgbaTile to its canonical form.
     *
     * @details
     * This method creates four candidate tiles with different flip combinations, then selects the one that comes first
     * in lexicographic order as the normal form. The resulting tile contains IndexPixel data referencing a normalized
     * palette.
     *
     * @param rgba_tile The RGBA tile to normalize
     * @param extrinsic_transparency Optional extrinsic transparency color
     * @return ChainableResult containing the normalized tile or an error if the tile has too many colors
     */
    [[nodiscard]] ChainableResult<NormalizedTile<Rgba32>>
    normalize(const RgbaTile &rgba_tile, const Rgba32 &extrinsic_transparency = Rgba32{}) const;

    /**
     * @brief Converts a NormalizedTile back to the original RgbaTile format.
     *
     * @details
     * This method performs the complete inverse of the normalization process:
     * 1. Converts IndexPixels back to RGBA colors using the tile's palette
     * 2. Applies the reverse of any flips that were applied during normalization
     *
     * The result should be equivalent to the original tile that was normalized, making this method suitable for
     * round-trip operations.
     *
     * @param normalized_tile The normalized tile to convert back to RGBA format
     * @return RgbaTile representing the original tile before normalization
     */
    [[nodiscard]] RgbaTile denormalize(const NormalizedTile<Rgba32> &normalized_tile) const;

    /**
     * @brief Converts a NormalizedTile to RGBA format while preserving the normalized flip state.
     *
     * @details
     * This method performs only the color conversion step of denormalization:
     * 1. Converts IndexPixels back to RGBA colors using the tile's palette
     * 2. Does NOT undo the flips applied during normalization
     *
     * The result preserves the normalized flip state, making it useful for testing and inspecting the effects of
     * normalization. The returned tile will have the same visual arrangement as the normalized form.
     *
     * @param normalized_tile The normalized tile to convert to RGBA format
     * @return RgbaTile with colors converted but flips preserved
     */
    [[nodiscard]] RgbaTile denormalize_preserving_flips(const NormalizedTile<Rgba32> &normalized_tile) const;
};

} // namespace porytiles2