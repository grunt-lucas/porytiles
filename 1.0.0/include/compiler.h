#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <cstdint>
#include <array>
#include <vector>
#include <png.hpp>

#include "types.h"

namespace porytiles {
DecompiledTileset decompiledTilesFromPng(const png::image<png::rgb_pixel>& png);
}
#endif