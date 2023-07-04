#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <cstdint>
#include <array>
#include <vector>

#include "bgr15.h"

namespace porytiles {
struct GBATile {
    std::array<std::uint8_t, 32> paletteIndexPairs;

    //auto operator<=>(GBATile const&) const = default;
};

struct GBAPalette {
    std::array<BGR15, 16> colors;

    //auto operator<=>(GBAPalette const&) const = default;
};

struct Assignment {
    std::size_t tileIndex;
    std::size_t paletteIndex;
    bool hFlip;
    bool vFlip;
};

struct Compiled {
    std::vector<GBATile> tiles;
    std::vector<GBAPalette> palettes;
    std::vector<Assignment> assignments;
};

}
#endif