#ifndef PORYTILES_CONSTANTS_H
#define PORYTILES_CONSTANTS_H

#include <limits>
#include <cstdint>

namespace porytiles {
// Tiles must be squares
constexpr std::size_t TILE_SIDE_LENGTH = 8;
constexpr std::size_t TILE_NUM_PIX = TILE_SIDE_LENGTH * TILE_SIDE_LENGTH;
constexpr std::size_t PAL_SIZE = 16;
constexpr std::size_t MAX_BG_PALETTES = 16;

constexpr std::uint8_t ALPHA_TRANSPARENT = 0;
constexpr std::uint8_t ALPHA_OPAQUE = 255;
}
#endif