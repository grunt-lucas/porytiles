#ifndef PORYTILES_IMPORTER_H
#define PORYTILES_IMPORTER_H

#include <filesystem>
#include <png.hpp>

#include "types.h"

/**
 * Utility functions for reading compiler input from PNG files. Provides utilities for reading a layered or raw
 * tilesheet.
 */

namespace porytiles {

/**
 * Build a DecompiledTileset from a single input PNG. This tileset is considered "raw", that is, it has no layering.
 * The importer will simply scan the PNG tiles left-to-right, top-to-bottom and put them into the DecompiledTileset.
 */
DecompiledTileset importRawTilesFromPng(const png::image<png::rgba_pixel>& png);

}

#endif