#include "rgb_tiled_png.h"

#include <png.hpp>
#include <stdexcept>

namespace tscreate {
RgbTiledPng::RgbTiledPng(const png::image<png::rgb_pixel>& png) {
    width = png.get_width() / TILE_DIMENSION;
    height = png.get_height() / TILE_DIMENSION;

    for (long tileRow = 0; tileRow < height; tileRow++) {
        for (long tileCol = 0; tileCol < width; tileCol++) {
            long pixelRowStart = tileRow * TILE_DIMENSION;
            long pixelColStart = tileCol * TILE_DIMENSION;
            RgbTile tile{{0, 0, 0}};
            for (long row = 0; row < TILE_DIMENSION; row++) {
                for (long col = 0; col < TILE_DIMENSION; col++) {
                    png::byte red = png[pixelRowStart + row][pixelColStart + col].red;
                    png::byte blue = png[pixelRowStart + row][pixelColStart + col].blue;
                    png::byte green = png[pixelRowStart + row][pixelColStart + col].green;
                    tile.setPixel(row, col, RgbColor{red, blue, green});
                }
            }
            addTile(tile);
        }
    }
}

void RgbTiledPng::addTile(const RgbTile& tile) {
    if (tiles.size() >= width * height) {
        throw std::runtime_error{"internal: TiledPng::addTile tried to add beyond image dimensions"};
    }
    tiles.push_back(tile);
}

const RgbTile& RgbTiledPng::tileAt(long row, long col) const {
    return tiles.at(row * width + col);
}

const RgbTile& RgbTiledPng::tileAt(long index) const {
    return tiles.at(index);
}
} // namespace tscreate
