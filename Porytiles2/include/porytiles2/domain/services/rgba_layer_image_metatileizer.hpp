#pragma once

#include <vector>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/domain/services/rgba_image_tileizer.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service for converting RGBA layer images into collections of metatiles.
 *
 * @details
 * The RgbaLayerImageMetatileizer service provides functionality to convert three RGBA layer images (bottom, middle,
 * top) into a collection of 16x16 pixel metatiles. Each metatile contains a 2x2 arrangement of 8x8 tiles from each
 * layer.
 *
 * This service handles the validation of input dimensions, tileization of each layer, and the construction of metatiles
 * by combining corresponding tiles from each layer.
 */
class RgbaLayerImageMetatileizer {
  public:
    /**
     * @brief Converts three RGBA layer images into a vector of metatiles.
     *
     * @details
     * This method takes three RGBA images representing the bottom, middle, and top layers and converts them into
     * metatiles. The process involves:
     * 1. Validating that all images have identical dimensions
     * 2. Tileizing each layer into 8x8 tiles
     * 3. Validating that dimensions are multiples of 16 (metatile size)
     * 4. Constructing 16x16 metatiles by combining 2x2 groups of tiles from each layer
     *
     * @param bottom The bottom layer RGBA image
     * @param middle The middle layer RGBA image
     * @param top The top layer RGBA image
     * @return A ChainableResult containing either:
     *         - Success: A vector of RgbaMetatile objects in row-major order
     *         - Error: A BasicError describing why metatileization failed
     */
    [[nodiscard]] ChainableResult<std::vector<RgbaMetatile>>
    metatileize(const Image<Rgba32> &bottom, const Image<Rgba32> &middle, const Image<Rgba32> &top) const;

  private:
    RgbaImageTileizer tileizer_;
};

} // namespace porytiles2
