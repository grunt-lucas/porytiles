#pragma once

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Domain service for normalizing RGBA tiles.
 *
 * @details
 * The RgbaTileNormalizer converts an RgbaTile to its canonical NormalizedTile form.
 * The normalization process involves:
 * - Constructing a NormalizedPal from the tile's unique colors
 * - Creating candidate NormalizedTiles with different flip combinations
 * - Selecting the lexicographically smallest tile as the normal form
 * - Validating that the tile has at most 16 unique colors (15 + transparency)
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
};

} // namespace porytiles2