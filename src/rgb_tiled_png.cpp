#include "rgb_tiled_png.h"

#include "tsexception.h"

#include <stdexcept>

namespace tscreate {
RgbTiledPng::RgbTiledPng(const png::image<png::rgb_pixel>& png) {
    if (png.get_width() % TILE_DIMENSION != 0) {
        throw TsException("PNG width must be divisible by 8, was: " + std::to_string(png.get_width()));
    }
    if (png.get_height() % TILE_DIMENSION != 0) {
        throw TsException("PNG height must be divisible by 8, was: " + std::to_string(png.get_height()));
    }
    width = png.get_width() / TILE_DIMENSION;
    height = png.get_height() / TILE_DIMENSION;

    for (long tileY = 0; tileY < height; tileY++) {
        for (long tileX = 0; tileX < width; tileX++) {
            long pixelYStart = tileY * TILE_DIMENSION;
            long pixelXStart = tileX * TILE_DIMENSION;
            RgbTile tile{{0, 0, 0}};
            for (long y = 0; y < TILE_DIMENSION; y++) {
                for (long x = 0; x < TILE_DIMENSION; x++) {
                    png[pixelYStart + y][pixelXStart + x];
                }
            }
        }
    }
}

void RgbTiledPng::addTile(const RgbTile& tile) {
    if (tiles.size() >= width * height) {
        throw std::runtime_error{"internal: TiledPng::addTile tried to add beyond image dimensions"};
    }
    tiles.push_back(tile);
}

const RgbTile& RgbTiledPng::tileAt(int row, int col) const {
    return tiles.at(row * width + col);
}

const RgbTile& RgbTiledPng::tileAt(int index) const {
    return tiles.at(index);
}
} // namespace tscreate
