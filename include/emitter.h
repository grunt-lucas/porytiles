#ifndef PORYTILES_EMITTER_H
#define PORYTILES_EMITTER_H

#include <iostream>
#include <png.hpp>

#include "porytiles_context.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

extern const std::size_t TILES_PNG_WIDTH_IN_TILES;

/**
 * TODO : fill in doc comment
 */
void emitPalette(PorytilesContext &ctx, std::ostream &out, const GBAPalette &palette);

/**
 * TODO : fill in doc comment
 */
void emitZeroedPalette(PorytilesContext &ctx, std::ostream &out);

/**
 * TODO : fill in doc comment
 */
void emitTilesPng(PorytilesContext &ctx, png::image<png::index_pixel> &out, const CompiledTileset &tileset);

/**
 * TODO : fill in doc comment
 */
void emitMetatilesBin(PorytilesContext &ctx, std::ostream &out, const CompiledTileset &tileset);

/**
 * TODO : fill in doc comment
 */
void emitAnim(PorytilesContext &ctx, std::vector<png::image<png::index_pixel>> &outFrames,
              const CompiledAnimation &animation, const std::vector<GBAPalette> &palettes);

/**
 * TODO : fill in doc comment
 */
void emitAttributes(PorytilesContext &ctx, std::ostream &out,
                    std::unordered_map<std::uint8_t, std::string> behaviorReverseMap, const CompiledTileset &tileset);

/**
 * TODO : fill in doc comment
 */
void emitDecompiled(PorytilesContext &ctx, DecompilerMode mode, png::image<png::rgba_pixel> &bottom,
                    png::image<png::rgba_pixel> &middle, png::image<png::rgba_pixel> &top, std::ostream &outCsv,
                    const DecompiledTileset &tileset, const std::unordered_map<std::size_t, Attributes> &attributesMap,
                    const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap);

void emitAssignCache(PorytilesContext &ctx, const CompilerMode &mode, std::ostream &out);
} // namespace porytiles

#endif // PORYTILES_EMITTER_H
