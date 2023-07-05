#include "compiler.h"

#include "constants.h"

namespace porytiles {
PrecompiledTileset precompiledTilesFromPng(const png::image<png::rgb_pixel>& png) {
    PrecompiledTileset precompiledTiles;

    // TODO : this won't work if width/height not divisible by 8, enforce somewhere, ideally just force 128 pix width
    std::size_t width = png.get_width() / TILE_SIDE_LENGTH;
    std::size_t height = png.get_height() / TILE_SIDE_LENGTH;

    for (size_t tileRow = 0; tileRow < height; tileRow++) {
        for (size_t tileCol = 0; tileCol < width; tileCol++) {
            long pixelRowStart = tileRow * TILE_SIDE_LENGTH;
            long pixelColStart = tileCol * TILE_SIDE_LENGTH;
            RGBATile tile;
        }
    }
}
}
