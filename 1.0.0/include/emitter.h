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

/**
 * TODO : fill in doc comment
 */
void emitPalette(const Config& config, std::ostream& out, const GBAPalette& palette);

/**
 * TODO : fill in doc comment
 */
void emitTilesPng(const Config& config, const png::image<png::rgba_pixel>& out, const CompiledTileset& tileset);

/**
 * TODO : fill in doc comment
 */
void emitMetatilesBin(const Config& config, std::ostream& out, const CompiledTileset& tileset);

}

#endif