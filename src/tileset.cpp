#include "tileset.h"

#include <png.hpp>

namespace tscreate {
const png::uint_32 NUM_BG_PALS = 16;

Tileset::Tileset(const png::uint_32 maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
}

bool Tileset::addPalette(const tscreate::Palette& palette) {
    if (palettes.size() >= maxPalettes) {
        return false;
    }
    palettes.push_back(palette);
    return true;
}
}
