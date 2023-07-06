#ifndef PORYTILES_PNG_FRONTEND_H
#define PORYTILES_PNG_FRONTEND_H

#include <png.hpp>

#include "types.h"

namespace porytiles {
DecompiledTileset decompiledTilesFrom(const png::image<png::rgb_pixel>& png);
}

#endif