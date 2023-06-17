#ifndef PORYTILES_RGB_TILED_PNG_H
#define PORYTILES_RGB_TILED_PNG_H

#include "tile.h"
#include "rgb_color.h"

#include <vector>
#include <cstddef>
#include <string>

namespace porytiles {
struct StructureRegion {
    const size_t topRow;
    const size_t bottomRow;
    const size_t leftCol;
    const size_t rightCol;
};

struct LinearRegion {
    // LinearRegion is for primer or sibling regions
    const size_t startIndex;
    const size_t endIndex;
    const size_t size;
};

class RgbTiledPng {
    size_t width;
    size_t height;
    std::vector<RgbTile> tiles;
    std::vector<StructureRegion> structures;
    std::vector<LinearRegion> primers;
    std::vector<LinearRegion> siblings;

public:
    explicit RgbTiledPng(const png::image<png::rgb_pixel>& png);

    [[nodiscard]] size_t size() const { return tiles.size(); }

    [[nodiscard]] size_t getWidth() const { return width; }

    [[nodiscard]] size_t getHeight() const { return height; }

    void pushTile(const RgbTile& tile);

    [[nodiscard]] const RgbTile& tileAt(size_t row, size_t col) const;

    [[nodiscard]] const RgbTile& tileAt(size_t index) const;

    [[nodiscard]] std::pair<size_t, size_t> indexToRowCol(size_t index) const;

    [[nodiscard]] size_t rowColToIndex(size_t row, size_t col) const;

    [[nodiscard]] StructureRegion getStructureStartingAt(size_t index) const;

    [[nodiscard]] const std::vector<LinearRegion>& getPrimers() const;

    [[nodiscard]] const std::vector<LinearRegion>& getSiblings() const;

    [[nodiscard]] const std::vector<StructureRegion>& getStructures() const;

    [[nodiscard]] std::string tileDebugString(size_t index) const;
};
} // namespace porytiles

#endif // PORYTILES_RGB_TILED_PNG_H