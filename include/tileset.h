#ifndef TSCREATE_TILESET_H
#define TSCREATE_TILESET_H

#include "tile.h"

#include <png.hpp>
#include <vector>

namespace tscreate {
extern const png::uint_32 NUM_BG_PALS;

class Tileset {
    std::vector<IndexedTile> tiles;
};
}

#endif // TSCREATE_TILESET_H