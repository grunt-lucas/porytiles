#include "porytiles2/domain/services/rgba_image_tileizer.hpp"

#include <cstddef>

#include "fmt/format.h"

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tile.hpp"
#include "porytiles2/templates/error.hpp"

namespace porytiles2 {

ChainableResult<std::vector<RgbaTile>> RgbaImageTileizer::tileize(const Image<Rgba32> &img) const
{
    constexpr std::size_t tile_side_length = RgbaTile::tile_side_length;

    // Validate that image dimensions are multiples of tile size
    if (img.width() % tile_side_length != 0 || img.height() % tile_side_length != 0) {
        return BasicError{fmt::format(
            "Image dimensions must be a multiple of {}, got {}x{}", tile_side_length, img.width(), img.height())};
    }

    const std::size_t tiles_per_row = img.width() / tile_side_length;
    const std::size_t tiles_per_col = img.height() / tile_side_length;
    const std::size_t total_tiles = tiles_per_row * tiles_per_col;

    std::vector<RgbaTile> tiles;
    tiles.reserve(total_tiles);

    // Process each tile region
    for (std::size_t tile_row = 0; tile_row < tiles_per_col; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < tiles_per_row; ++tile_col) {
            RgbaTile tile;

            // Calculate pixel offsets for this tile
            const std::size_t pixel_row_offset = tile_row * tile_side_length;
            const std::size_t pixel_col_offset = tile_col * tile_side_length;

            // Copy pixels from source image to tile
            for (std::size_t pixel_row = 0; pixel_row < tile_side_length; ++pixel_row) {
                for (std::size_t pixel_col = 0; pixel_col < tile_side_length; ++pixel_col) {
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
