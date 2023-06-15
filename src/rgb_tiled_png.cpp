#include "rgb_tiled_png.h"

#include <png.hpp>
#include <stdexcept>

namespace porytiles {
RgbTiledPng::RgbTiledPng(const png::image<png::rgb_pixel>& png) {
    width = png.get_width() / TILE_DIMENSION;
    height = png.get_height() / TILE_DIMENSION;

    for (size_t tileRow = 0; tileRow < height; tileRow++) {
        for (size_t tileCol = 0; tileCol < width; tileCol++) {
            long pixelRowStart = tileRow * TILE_DIMENSION;
            long pixelColStart = tileCol * TILE_DIMENSION;
            RgbTile tile{{0, 0, 0}};
            for (size_t row = 0; row < TILE_DIMENSION; row++) {
                for (size_t col = 0; col < TILE_DIMENSION; col++) {
                    png::byte red = png[pixelRowStart + row][pixelColStart + col].red;
                    png::byte green = png[pixelRowStart + row][pixelColStart + col].green;
                    png::byte blue = png[pixelRowStart + row][pixelColStart + col].blue;
                    tile.setPixel(row, col, RgbColor{red, green, blue});
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

const RgbTile& RgbTiledPng::tileAt(size_t row, size_t col) const {
    if (row >= height)
        throw std::runtime_error{
                "internal: RgbTiledPng::tileAt row argument out of bounds (" + std::to_string(row) + ")"};
    if (col >= width)
        throw std::runtime_error{
                "internal: RgbTiledPng::tileAt col argument out of bounds (" + std::to_string(col) + ")"};
    return tiles.at(row * width + col);
}

const RgbTile& RgbTiledPng::tileAt(size_t index) const {
    if (index >= width * height) {
        throw std::runtime_error{"internal: TiledPng::tileAt tried to reference index beyond image dimensions: " +
                                 std::to_string(index)};
    }
    return tiles.at(index);
}

std::string RgbTiledPng::tileDebugString(size_t index) const {
    return "tile " + std::to_string(index) + " (col,row) " +
           "(" + std::to_string(index % width) +
           "," + std::to_string(index / width) + ")";
}
} // namespace porytiles
