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

struct DecompiledIndex {
  bool animated;
  std::size_t animIndex;
  std::size_t tileIndex;

  DecompiledIndex() : animated{false}, animIndex{0}, tileIndex{0} {}
};

extern std::size_t gPaletteAssignCutoffCounter;

/**
 * TODO : fill in doc comments
 */
std::unique_ptr<CompiledTileset> compile(PtContext &ctx, const DecompiledTileset &decompiledTileset,
                                         const std::vector<RGBATile> &palettePrimers);

} // namespace porytiles

/*
 * Some of the types we need are extremely verbose and confusing, so here let's define some better names to make the
 * code a bit more readable.
 */
// ColorSets won't account for transparency color, we will handle that at the end
using ColorSet = std::bitset<porytiles::MAX_BG_PALETTES *(porytiles::PAL_SIZE - 1)>;
// using DecompiledIndex = std::size_t;
using IndexAndNormTile = std::pair<porytiles::DecompiledIndex, porytiles::NormalizedTile>;
using IndexedNormTileWithColorSet = std::tuple<porytiles::DecompiledIndex, porytiles::NormalizedTile, ColorSet>;

#endif // PORYTILES_COMPILER_H
