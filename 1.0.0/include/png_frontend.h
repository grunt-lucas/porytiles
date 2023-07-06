#ifndef PORYTILES_PNG_FRONTEND_H
#define PORYTILES_PNG_FRONTEND_H

#include <filesystem>
#include <png.hpp>

#include "types.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {
/**
 * Preconditions:
 * 1) input png width is divisible by 8
 * 2) input png height is divisible by 8 and not larger than max allowed metatiles
 *
 * These must be enforced at the callsite
 *
 * TODO : enforce these preconditions at callsite
 */
DecompiledTileset importTilesFrom(const png::image<png::rgba_pixel>& png);
}

#endif