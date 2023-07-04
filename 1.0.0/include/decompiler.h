#ifndef PORYTILES_DECOMPILER_H
#define PORYTILES_DECOMPILER_H

#include <cstdint>
#include <array>
#include <vector>

#include "constants.h"
#include "rgba32.h"
#include "bgr15.h"

namespace porytiles {
struct RGBATile {
    std::array<RGBA32, TILE_SIZE> pixels;
};

struct Decompiled {
    std::vector<RGBATile> tiles;
};
}
#endif