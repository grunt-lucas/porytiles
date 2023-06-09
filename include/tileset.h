#ifndef TSCREATE_TILESET_H
#define TSCREATE_TILESET_H

#include "tile.h"
#include "palette.h"
#include "rgb_tiled_png.h"

#include <png.hpp>
#include <vector>
#include <unordered_set>

namespace tscreate {
extern const int NUM_BG_PALS;

class Tileset {
    std::vector<IndexedTile> tiles;
    std::unordered_set<IndexedTile> tilesIndex;
    std::vector<Palette> palettes;
    int maxPalettes;

public:
    explicit Tileset(int maxPalettes);

    [[nodiscard]] int getMaxPalettes() const { return maxPalettes; }

    void buildPalettes(const RgbTiledPng& masterTiles);
};
} // namespace tscreate

#endif // TSCREATE_TILESET_H