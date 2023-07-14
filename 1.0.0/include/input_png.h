#ifndef PORYTILES_INPUT_PNG_H
#define PORYTILES_INPUT_PNG_H

#include <filesystem>
#include <png.hpp>

#include "types.h"

/**
 * Utility functions for reading compiler input from PNG files. Provides utilities for reading a layered or raw
 * tilesheet.
 */

namespace porytiles {

DecompiledTileset importRawTilesFrom(const png::image<png::rgba_pixel>& png);
/**<
 * Build a DecompiledTileset from a single input PNG. This tileset is considered "raw", that is, it has no layering.
 * The importer will simply scan the PNG tiles left-to-right, top-to-bottom and put them into the DecompiledTileset.
 */

}

#endif