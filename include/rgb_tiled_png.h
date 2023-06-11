#ifndef PORYTILES_RGB_TILED_PNG_H
#define PORYTILES_RGB_TILED_PNG_H

#include "tile.h"
#include "rgb_color.h"

#include <vector>
#include <cstddef>
#include <string>

namespace porytiles {
class RgbTiledPng {
    size_t width;
    size_t height;
    std::vector<RgbTile> tiles;

public:
    explicit RgbTiledPng(const png::image<png::rgb_pixel>& png);

    [[nodiscard]] size_t size() const { return tiles.size(); }

    [[nodiscard]] size_t getWidth() const { return width; }

    [[nodiscard]] size_t getHeight() const { return height; }

    void addTile(const RgbTile& tile);

    [[nodiscard]] const RgbTile& tileAt(size_t row, size_t col) const;

    [[nodiscard]] const RgbTile& tileAt(size_t index) const;

    [[nodiscard]] std::string tileDebugString(size_t index) const;
};
} // namespace porytiles

#endif // PORYTILES_RGB_TILED_PNG_H