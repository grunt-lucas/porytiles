#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <bitset>
#include <memory>
#include <tuple>

#include "./porytiles_context.h"
#include "./types.h"

namespace porytiles {

struct DecompiledIndex {
    bool animated;
    std::size_t animIndex;
    std::size_t tileIndex;

    DecompiledIndex() : animated{false}, animIndex{0}, tileIndex{0} {}
};

extern std::size_t gPaletteAssignCutoffCounter;

std::unique_ptr<CompiledTileset>
compile(PorytilesContext &ctx, CompilerMode compilerMode, const DecompiledTileset &decompiledTileset,
        const std::vector<RGBATile> &palettePrimers, const std::vector<RGBATile> &paletteOverrides,
        const std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, BGR15>>> &palOverridesMap);

} // namespace porytiles

/*
 * Some of the types we need are extremely verbose and confusing, so here let's define some better names to make the
 * code a bit more readable.
 */

// @formatter:off
// clang-format off

// ColorSets won't account for transparency color, we will handle that at the end
using ColorSet = std::pair<std::bitset<porytiles::MAX_BG_PALETTES * (porytiles::PAL_SIZE - 1)>, std::size_t>;

template <> struct std::hash<ColorSet> {
  std::size_t operator()(const ColorSet &colorSet) const noexcept
  {
    // TODO : better hash function
    return std::hash<std::bitset<porytiles::MAX_BG_PALETTES * (porytiles::PAL_SIZE - 1)>>{}(colorSet.first) ^
           std::hash<std::size_t>{}(colorSet.second);
  }
};

// @formatter:on
// clang-format on

using IndexAndNormTile = std::pair<porytiles::DecompiledIndex, porytiles::NormalizedTile>;
using IndexedNormTileWithColorSet = std::tuple<porytiles::DecompiledIndex, porytiles::NormalizedTile, ColorSet>;

#endif // PORYTILES_COMPILER_H
