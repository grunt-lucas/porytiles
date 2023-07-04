#ifndef PORYTILES_CONSTANTS_H
#define PORYTILES_CONSTANTS_H

#include <limits>

namespace porytiles {
constexpr std::size_t TILE_ROWS = 8;
constexpr std::size_t TILE_COLS = 8;
constexpr std::size_t TILE_SIZE = TILE_ROWS * TILE_COLS;
constexpr std::size_t PAL_SIZE = 16;
}
#endif