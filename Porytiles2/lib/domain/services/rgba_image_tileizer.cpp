#include "porytiles2/domain/services/rgba_image_tileizer.hpp"

#include <cstddef>

#include "fmt/format.h"

#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/rgba_tile.hpp"
#include "porytiles2/domain/models/tile_constants.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::vector<RgbaTile>> RgbaImageTileizer::tileize(const Image<Rgba32> &img) const
{
    // Validate that image dimensions are multiples of tile size
    if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
        return FormattableError{fmt::format(
            "Image dimensions must be a multiple of {}, got {}x{}", tile::side_length_pix, img.width(), img.height())};
    }

    const std::size_t tiles_per_row = img.width() / tile::side_length_pix;
    const std::size_t tiles_per_col = img.height() / tile::side_length_pix;
    const std::size_t total_tiles = tiles_per_row * tiles_per_col;

    std::vector<RgbaTile> tiles;
    tiles.reserve(total_tiles);

    // Process each tile region
    for (std::size_t tile_row = 0; tile_row < tiles_per_col; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < tiles_per_row; ++tile_col) {
            RgbaTile tile;

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

} // namespace porytiles2
