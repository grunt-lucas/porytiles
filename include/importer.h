#ifndef PORYTILES_IMPORTER_H
#define PORYTILES_IMPORTER_H

#include <filesystem>
#include <png.hpp>

#include "ptcontext.h"
#include "types.h"

/**
 * Utility functions for reading input from various file types. Provides utilities for reading a layered or raw
 * tilesheet.
 */

namespace porytiles {

/**
 * Build a DecompiledTileset from a single input PNG. This tileset is considered "raw", that is, it has no layering.
 * The importer will simply scan the PNG tiles left-to-right, top-to-bottom and put them into the DecompiledTileset.
 */
DecompiledTileset importTilesFromPng(PtContext &ctx, const png::image<png::rgba_pixel> &png);

DecompiledTileset importLayeredTilesFromPngs(PtContext &ctx, const png::image<png::rgba_pixel> &bottom,
                                             const png::image<png::rgba_pixel> &middle,
                                             const png::image<png::rgba_pixel> &top);

void importAnimTiles(PtContext &ctx, const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles);

} // namespace porytiles

#endif // PORYTILES_IMPORTER_H
