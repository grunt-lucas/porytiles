#pragma once

#include <format>
#include <vector>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

/**
 * @brief Extracts a subset of 8x8 tiles from a tileset image at a specific offset.
 *
 * @details
 * This function extracts `tile_count` tiles starting from `tile_offset`, treating the image as a linear sequence of
 * tiles arranged in rows of `tiles_per_row` width. Tiles are numbered in row-major order starting from the top-left.
 *
 * For example, with tiles_per_row=16 (standard tiles.png format):
 * - tile_offset=0 is at position (0,0)
 * - tile_offset=15 is at position (0,15*8)
 * - tile_offset=16 is at position (8,0) (first tile of second row)
 *
 * @tparam PixelType The pixel type of the image and resulting tiles
 * @param img The source tileset image
 * @param tile_offset The starting tile index (0-based)
 * @param tile_count The number of tiles to extract
 * @param tiles_per_row The number of tiles per row in the source image
 * @pre Image width must be a multiple of 8
 * @pre Image height must be a multiple of 8
 * @pre tile_offset + tile_count must not exceed the total tiles in the image
 * @return Vector of extracted PixelTiles
 */
template <typename PixelType>
[[nodiscard]] std::vector<PixelTile<PixelType>> extract_tiles_from_image(
    const Image<PixelType> &img, std::size_t tile_offset, std::size_t tile_count, std::size_t tiles_per_row = 16)
{
    if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
        panic(
            std::format(
                "image dimensions must be multiples of {}, got {}x{}",
                tile::side_length_pix,
                img.width(),
                img.height()));
    }

    std::vector<PixelTile<PixelType>> result;
    result.reserve(tile_count);

    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_idx = tile_offset + i;
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;

        const std::size_t pixel_x = tile_col * tile::side_length_pix;
        const std::size_t pixel_y = tile_row * tile::side_length_pix;

        PixelTile<PixelType> pixel_tile;
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                pixel_tile.set(row, col, img.at(pixel_y + row, pixel_x + col));
            }
        }

        result.push_back(std::move(pixel_tile));
    }

    return result;
}

/**
 * @brief Extracts all 8x8 tiles from an image in row-major order.
 *
 * @details
 * This function divides the input image into 8x8 pixel tiles and returns them as a vector. Tiles are extracted in
 * row-major order: starting from the top-left corner, proceeding left-to-right across each row, then top-to-bottom
 * across rows.
 *
 * This is a convenience overload that delegates to the offset/count version with tile_offset=0 and tile_count equal to
 * the total number of tiles in the image.
 *
 * @tparam PixelType The pixel type of the image and resulting tiles
 * @param img The source image to extract tiles from
 * @pre Image width must be a multiple of 8
 * @pre Image height must be a multiple of 8
 * @return Vector of extracted PixelTiles in row-major order
 */
template <typename PixelType>
[[nodiscard]] std::vector<PixelTile<PixelType>> extract_tiles_from_image(const Image<PixelType> &img)
{
    if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
        panic(
            std::format(
                "image dimensions must be multiples of {}, got {}x{}",
                tile::side_length_pix,
                img.width(),
                img.height()));
    }

    const std::size_t tiles_per_row = img.width() / tile::side_length_pix;
    const std::size_t total_tiles = img.size_in_tiles();

    return extract_tiles_from_image(img, 0, total_tiles, tiles_per_row);
}

/**
 * @brief Extracts a single tile from an image at a given tile index.
 *
 * @details
 * Convenience wrapper around extract_tiles_from_image for single-tile extraction. Useful for extracting individual
 * tiles for diagnostic visualization.
 *
 * @tparam PixelType The pixel type of the image and resulting tile
 * @param img The source tileset image
 * @param tile_idx The tile index to extract (0-based)
 * @param tiles_per_row The number of tiles per row in the source image (default 16 for standard tiles.png)
 * @return The extracted PixelTile at the specified index
 */
template <typename PixelType>
[[nodiscard]] PixelTile<PixelType> extract_single_tile(
    const Image<PixelType> &img,
    std::size_t tile_idx,
    std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side)
{
    return extract_tiles_from_image(img, tile_idx, 1, tiles_per_row).at(0);
}

} // namespace porytiles
