#pragma once

#include <vector>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Service for converting RGBA images into collections of 8x8 tiles.
 *
 * @details
 * The RgbaImageTileizer service provides functionality to decompose RGBA images into individual 8x8 pixel tiles.
 * This is a fundamental operation in tileset compilation, where large images need to be broken down into the
 * tile-based format used by the Game Boy Advance graphics system.
 *
 * The service validates that input images have dimensions that are multiples of the tile size (8 pixels) and
 * processes the image in row-major order, creating tiles from left-to-right, top-to-bottom. Each resulting
 * RgbaTile contains the exact pixel data from the corresponding 8x8 region of the source image.
 */
class RgbaImageTileizer {
  public:
    /**
     * @brief Converts an RGBA image into a vector of 8x8 tiles.
     *
     * @details
     * This method decomposes the input image into individual 8x8 pixel tiles, processing the image in row-major order
     * (left-to-right, top-to-bottom). The resulting tiles contain the exact pixel data from their corresponding regions
     * in the source image.
     *
     * The method validates that the image dimensions are multiples of 8 pixels, as partial tiles are not supported in
     * the Game Boy Advance graphics system. If the validation fails, an error is returned with details about the
     * invalid dimensions.
     *
     * For an image of width W and height H pixels:
     * - Number of tiles per row: W / 8
     * - Number of tile rows: H / 8
     * - Total tiles: (W / 8) * (H / 8)
     * - Tile ordering: tiles[row * tiles_per_row + col] where row and col are tile coordinates
     *
     * @param img The source RGBA image to tileize
     * @return A ChainableResult containing either:
     *         - Success: A vector of RgbaTile objects in row-major order
     *         - Error: A FormattableError describing why tileization failed (e.g., invalid dimensions)
     */
    [[nodiscard]] ChainableResult<std::vector<RgbaTile>> tileize(const Image<Rgba32> &img) const;
};

} // namespace porytiles2