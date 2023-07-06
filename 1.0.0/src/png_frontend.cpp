#include "png_frontend.h"

#include <png.hpp>

#include "types.h"

namespace porytiles {
DecompiledTileset decompiledTilesFrom(const png::image<png::rgb_pixel>& png) {
    /*
     * Preconditions:
     * 1) png width is 128 pixels
     * 2) png height is divisible by 8 and not larger than max allowed metatiles
     *
     * These need to be enforced by the caller
     *
     * TODO : enforce these preconditions at callsite
     */
    DecompiledTileset decompiledTiles;

    std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
    std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

    for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        // size_t row = index / width;
        // size_t col = index % width;
    }

    return decompiledTiles;
}
}
