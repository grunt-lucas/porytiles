#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <memory>

#include "ptcontext.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

/*
 * Some of the types we need are extremely verbose and confusing, so here let's define some better names to make the
 * code a bit more readable.
 */
// ColorSets won't account for transparency color, we will handle that at the end
using ColorSet = std::bitset<porytiles::MAX_BG_PALETTES *(porytiles::PAL_SIZE - 1)>;
using DecompiledIndex = std::size_t;
using IndexedNormTile = std::pair<DecompiledIndex, porytiles::NormalizedTile>;
using IndexedNormTileWithColorSet = std::tuple<DecompiledIndex, porytiles::NormalizedTile, ColorSet>;

namespace porytiles {

extern std::size_t gRecurseCount;

/**
 * TODO : fill in doc comments
 */
std::unique_ptr<CompiledTileset> compile(PtContext &ctx, const DecompiledTileset &decompiledTileset);

} // namespace porytiles

#endif // PORYTILES_COMPILER_H
