#ifndef TSCREATE_RGB_TILED_PNG_H
#define TSCREATE_RGB_TILED_PNG_H

#include "tile.h"
#include "rgb_color.h"

#include <vector>

namespace tscreate {
class RgbTiledPng {
    long width;
    long height;
    std::vector<RgbTile> tiles;

public:
    explicit RgbTiledPng(long width, long height) : width{width}, height{height} {
        tiles.reserve(this->width * this->height);
    }

    explicit RgbTiledPng(const png::image<png::rgb_pixel>& png);

    void addTile(const RgbTile& tile);

    [[nodiscard]] const RgbTile& tileAt(int row, int col) const;

    [[nodiscard]] const RgbTile& tileAt(int index) const;
};
} // namespace tscreate

#endif // TSCREATE_RGB_TILED_PNG_H