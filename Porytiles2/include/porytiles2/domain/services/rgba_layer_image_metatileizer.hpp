#pragma once

#include <tuple>
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

    /**
     * @brief Converts a vector of metatiles back into three separate RGBA layer images.
     *
     * @details
     * This method performs the inverse of metatileize, reconstructing the original three layer images from a collection
     * of metatiles. The process involves:
     * 1. Validating that the metatiles count corresponds to valid image dimensions
     * 2. Extracting tiles from each metatile and organizing them by layer
     * 3. Reconstructing the full images by combining tiles back into pixel data
     *
     * @param metatiles The vector of RgbaMetatile objects to convert back to images
     * @param metatiles_per_row The number of metatiles per row (width in metatiles)
     * @param metatiles_per_col The number of metatiles per column (height in metatiles)
     * @return A ChainableResult containing either:
     *         - Success: A tuple of three Image<Rgba32> objects (bottom, middle, top)
     *         - Error: A BasicError describing why demetatileization failed
     */
    [[nodiscard]] ChainableResult<std::tuple<Image<Rgba32>, Image<Rgba32>, Image<Rgba32>>> demetatileize(
        const std::vector<RgbaMetatile> &metatiles, std::size_t metatiles_per_row, std::size_t metatiles_per_col) const;

  private:
    RgbaImageTileizer tileizer_;
};

} // namespace porytiles2
