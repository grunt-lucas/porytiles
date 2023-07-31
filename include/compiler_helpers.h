#ifndef PORYTILES_COMPILER_HELPERS_H
#define PORYTILES_COMPILER_HELPERS_H

#include <bitset>
#include <tuple>
#include <utility>
#include <vector>

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
std::size_t insertRGBA(const RGBA32 &transparencyColor, NormalizedPalette &palette, RGBA32 rgba);

NormalizedTile candidate(const RGBA32 &transparencyColor, const RGBATile &rgba, bool hFlip, bool vFlip);

NormalizedTile normalize(const RGBA32 &transparencyColor, const RGBATile &rgba);

std::vector<IndexedNormTile> normalizeDecompTiles(const RGBA32 &transparencyColor,
                                                  const DecompiledTileset &decompiledTileset);

std::pair<std::unordered_map<BGR15, std::size_t>, std::unordered_map<std::size_t, BGR15>>
buildColorIndexMaps(PtContext &ctx, const std::vector<IndexedNormTile> &normalizedTiles,
                    const std::unordered_map<BGR15, std::size_t> &primaryIndexMap);

ColorSet toColorSet(const std::unordered_map<BGR15, std::size_t> &colorIndexMap, const NormalizedPalette &palette);

std::pair<std::vector<IndexedNormTileWithColorSet>, std::vector<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t> &colorIndexMap,
                             const std::vector<IndexedNormTile> &indexedNormalizedTiles);

struct AssignState {
  /*
   * One color set for each hardware palette, bits in color set will indicate which colors this HW palette will have.
   * The size of the vector should be fixed to maxPalettes.
   */
  std::vector<ColorSet> hardwarePalettes;

  // The unique color sets from the NormalizedTiles
  std::vector<ColorSet> unassigned;
};
extern std::size_t gRecurseCount;
bool assign(const std::size_t maxRecurseCount, AssignState state, std::vector<ColorSet> &solution,
            const std::vector<ColorSet> &primaryPalettes);

GBATile makeTile(const NormalizedTile &normalizedTile, GBAPalette palette);

void assignTilesPrimary(PtContext &ctx, CompiledTileset &compiled,
                        const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                        const std::vector<ColorSet> &assignedPalsSolution);

void assignTilesSecondary(PtContext &ctx, CompiledTileset &compiled,
                          const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                          const std::vector<ColorSet> &primaryPaletteColorSets,
                          const std::vector<ColorSet> &assignedPalsSolution);
} // namespace porytiles

#endif // PORYTILES_COMPILER_HELPERS_H
