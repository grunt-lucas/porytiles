#ifndef PORYTILES_TILESET_H
#define PORYTILES_TILESET_H

#include "tile.h"
#include "palette.h"
#include "rgb_tiled_png.h"

#include <png.hpp>
#include <vector>
#include <unordered_set>

namespace porytiles {
constexpr int NUM_BG_PALS = 12;

class Tileset {
    std::vector<IndexedTile> tiles;
    std::unordered_set<IndexedTile> tilesIndex;
    std::vector<Palette> palettes;
    int maxPalettes;

public:
    explicit Tileset(int maxPalettes);

    [[nodiscard]] int getMaxPalettes() const { return maxPalettes; }

    void alignSiblings(const RgbTiledPng& masterTiles);

    void buildPalettes(const RgbTiledPng& masterTiles);

    void indexTiles(const RgbTiledPng& masterTiles);
};
} // namespace porytiles

#endif // PORYTILES_TILESET_H