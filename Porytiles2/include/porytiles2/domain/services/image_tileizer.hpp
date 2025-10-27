#pragma once

#include <cstddef>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Service for converting images into collections of 8x8 tiles.
 *
 * @details
 * The ImageTileizer service provides functionality to decompose images into individual 8x8 pixel tiles.
 * This is a fundamental operation in tileset compilation, where large images need to be broken down into the
 * tile-based format used by the Game Boy Advance graphics system.
 *
 * The service validates that input images have dimensions that are multiples of the tile size (8 pixels) and
 * processes the image in row-major order, creating tiles from left-to-right, top-to-bottom. Each resulting
 * PixelTile<T> contains the exact pixel data from the corresponding 8x8 region of the source image.
 *
 * @tparam T The pixel type (e.g., Rgba32, IndexPixel)
 */
template <typename T>
class ImageTileizer {
  public:
    /**
     * @brief Converts an image into a vector of 8x8 tiles.
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
     * @param img The source image to tileize
     * @tparam T The pixel type (e.g., Rgba32, IndexPixel)
     * @return A ChainableResult containing either:
     *         - Success: A vector of PixelTile<T> objects in row-major order
     *         - Error: A FormattableError describing why tileization failed (e.g., invalid dimensions)
     */
    [[nodiscard]] ChainableResult<std::vector<PixelTile<T>>> tileize(const Image<T> &img) const
    {
        // Validate that image dimensions are multiples of tile size
        if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
            return FormattableError{fmt::format(
                "image dimensions must be a multiple of {}, got {}x{}",
                tile::side_length_pix,
                img.width(),
                img.height())};
        }

        const std::size_t tiles_per_row = img.width() / tile::side_length_pix;
        const std::size_t tiles_per_col = img.height() / tile::side_length_pix;
        const std::size_t total_tiles = tiles_per_row * tiles_per_col;

        std::vector<PixelTile<T>> tiles;
        tiles.reserve(total_tiles);

        // Process each tile region
        for (std::size_t tile_row = 0; tile_row < tiles_per_col; ++tile_row) {
            for (std::size_t tile_col = 0; tile_col < tiles_per_row; ++tile_col) {
                PixelTile<T> tile;

                // Calculate pixel offsets for this tile
                const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
                const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

                // Copy pixels from source image to tile
                for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
                    for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                        const std::size_t src_row = pixel_row_offset + pixel_row;
                        const std::size_t src_col = pixel_col_offset + pixel_col;

                        tile.set(pixel_row, pixel_col, img.at(src_row, src_col));
                    }
                }

                tiles.push_back(std::move(tile));
            }
        }

        return tiles;
    }
};

} // namespace porytiles2
