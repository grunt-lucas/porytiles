#ifndef TSCREATE_TILESET_H
#define TSCREATE_TILESET_H

#include "tile.h"
#include "palette.h"

#include <png.hpp>
#include <vector>

namespace tscreate {
extern const int NUM_BG_PALS;

class Tileset {
    std::vector<IndexedTile> tiles;
    std::unordered_set<IndexedTile> tilesIndex;
    std::vector<Palette> palettes;
    int maxPalettes;

public:
    explicit Tileset(int maxPalettes);

    bool addPalette(const tscreate::Palette& palette);
};
}

#endif // TSCREATE_TILESET_H