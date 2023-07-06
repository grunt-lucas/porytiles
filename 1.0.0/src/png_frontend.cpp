#include "png_frontend.h"

#include <png.hpp>

#include "types.h"

namespace porytiles {
DecompiledTileset importTilesFrom(const png::image<png::rgba_pixel>& png) {
    DecompiledTileset decompiledTiles;

    std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
    std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

    for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / pngWidthInTiles;
        size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
            tile.pixels[pixelIndex].red = png[pixelRow][pixelCol].red;
            tile.pixels[pixelIndex].green = png[pixelRow][pixelCol].green;
            tile.pixels[pixelIndex].blue = png[pixelRow][pixelCol].blue;
            tile.pixels[pixelIndex].alpha = png[pixelRow][pixelCol].alpha;
        }
        decompiledTiles.tiles.push_back(tile);
    }
    return decompiledTiles;
}
}
