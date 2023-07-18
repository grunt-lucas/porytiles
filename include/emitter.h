#ifndef PORYTILES_EMITTER_H
#define PORYTILES_EMITTER_H

#include <iostream>
#include <png.hpp>

#include "types.h"
#include "config.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

extern const std::size_t TILES_PNG_WIDTH_IN_TILES;

/**
 * TODO : fill in doc comment
 */
void emitPalette(const Config& config, std::ostream& out, const GBAPalette& palette);

/**
 * TODO : fill in doc comment
 */
void emitZeroedPalette(const Config& config, std::ostream& out);

/**
 * TODO : fill in doc comment
 */
void emitTilesPng(const Config& config, png::image<png::index_pixel>& out, const CompiledTileset& tileset);

/**
 * TODO : fill in doc comment
 */
void emitMetatilesBin(const Config& config, std::ostream& out, const CompiledTileset& tileset);

}

#endif