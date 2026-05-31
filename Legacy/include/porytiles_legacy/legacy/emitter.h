#ifndef PORYTILES_EMITTER_H
#define PORYTILES_EMITTER_H

#include <iostream>
#include <png.hpp>

#include "./porytiles_context.h"
#include "./types.h"

namespace porytiles_legacy {

extern const std::size_t TILES_PNG_WIDTH_IN_TILES;

void emitPalette(PorytilesContext &ctx, std::ostream &out, const GBAPalette &palette);

void emitZeroedPalette(PorytilesContext &ctx, std::ostream &out);

void emitTilesPng(PorytilesContext &ctx, png::image<png::index_pixel> &out, const CompiledTileset &tileset);

void emitMetatilesBin(PorytilesContext &ctx, std::ostream &out, const CompiledTileset &tileset);

void emitAnim(PorytilesContext &ctx, std::vector<png::image<png::index_pixel>> &outFrames,
              const CompiledAnimation &animation, const std::vector<GBAPalette> &palettes);

void emitAttributes(const PorytilesContext &ctx, std::ostream &out,
                    const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap,
                    const CompiledTileset &tileset);

void emitDecompiled(PorytilesContext &ctx, DecompilerMode mode, png::image<png::rgba_pixel> &bottom,
                    png::image<png::rgba_pixel> &middle, png::image<png::rgba_pixel> &top, std::ostream &outCsv,
                    const DecompiledTileset &tileset, const std::unordered_map<std::size_t, Attributes> &attributesMap,
                    const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap);
} // namespace porytiles_legacy

#endif // PORYTILES_EMITTER_H
