#pragma once

#include "porytiles2/domain/model/iso_flip_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Domain service that converts \link RgbaTile RgbaTiles \endlink and IndexPixel \link Tile Tiles \endlink to
 * their canonical IsoFlipTile form, and vice versa.
 */
class IsoFlipTileNormalizer {
  public:
    /**
     * @brief Normalizes an RgbaTile to its IsoFlipTile canonical form.
     *
     * @details
     * This method creates four candidate tiles with different flip combinations, then selects the one that comes first
     * in lexicographic order as the canonical form. The resulting IsoFlipTile contains IndexPixel data referencing an
     * internal OrderedPal.
     *
     * @param rgba_tile The RGBA tile to normalize
     * @param extrinsic_transparency Optional extrinsic transparency color
     * @return ChainableResult containing the canonical tile or an error if the tile has too many colors
     */
    [[nodiscard]] ChainableResult<IsoFlipTile>
    normalize(const RgbaTile &rgba_tile, const Rgba32 &extrinsic_transparency = Rgba32{}) const;

    [[nodiscard]] ChainableResult<std::vector<IsoFlipTile>>
    batch_normalize(const std::vector<RgbaMetatile> &metatiles, const Rgba32 &extrinsic_transparency = Rgba32{}) const;

    /**
     * @brief Converts an IsoFlipTile back to the original RgbaTile format.
     *
     * @param tile The IsoFlipTile to convert back to RGBA format
     * @return RgbaTile representing the original tile before normalization
     */
    [[nodiscard]] RgbaTile denormalize(const IsoFlipTile &tile) const;

    /**
     * @brief Converts an IsoFlipTile to RGBA format while preserving the normalized flip state.
     *
     * @details
     * The result preserves the normalized flip state, making it useful for testing and inspecting the effects of
     * normalization. The returned tile will have the same visual arrangement as the normalized form.
     *
     * @param tile The IsoFlipTile to convert to RGBA format
     * @return RgbaTile with colors converted but flips preserved
     */
    [[nodiscard]] RgbaTile denormalize_preserving_flips(const IsoFlipTile &tile) const;
};

} // namespace porytiles2
