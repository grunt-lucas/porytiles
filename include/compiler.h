#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <bitset>
#include <memory>
#include <tuple>

#include "ptcontext.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

struct DecompiledIndex2 {
  bool animated;
  std::size_t animIndex;
  std::size_t frameIndex;
  std::size_t tileIndex;
};

extern std::size_t gRecurseCount;

/**
 * TODO : fill in doc comments
 */
std::unique_ptr<CompiledTileset> compile(PtContext &ctx, const DecompiledTileset &decompiledTileset);

} // namespace porytiles

/*
 * Some of the types we need are extremely verbose and confusing, so here let's define some better names to make the
 * code a bit more readable.
 */
// ColorSets won't account for transparency color, we will handle that at the end
using ColorSet = std::bitset<porytiles::MAX_BG_PALETTES *(porytiles::PAL_SIZE - 1)>;
using DecompiledIndex = std::size_t;
using IndexedNormTile = std::pair<DecompiledIndex, porytiles::NormalizedTile>;
using IndexedNormTileWithColorSet = std::tuple<DecompiledIndex, porytiles::NormalizedTile, ColorSet>;

#endif // PORYTILES_COMPILER_H
