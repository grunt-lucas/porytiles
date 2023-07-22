#ifndef PORYTILES_COMPILER_HELPERS_H
#define PORYTILES_COMPILER_HELPERS_H

#include <vector>
#include <bitset>
#include <tuple>
#include <utility>

#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

/*
 * Some of the types we need are extremely verbose and confusing, so here let's define some better names to make the
 * code a bit more readable.
 */
// ColorSets won't account for transparency color, we will handle that at the end
using ColorSet = std::bitset<porytiles::MAX_BG_PALETTES * (porytiles::PAL_SIZE - 1)>;
using DecompiledIndex = std::size_t;
using IndexedNormTile = std::pair<DecompiledIndex, porytiles::NormalizedTile>;
using IndexedNormTileWithColorSet = std::tuple<DecompiledIndex, porytiles::NormalizedTile, ColorSet>;

namespace porytiles {
std::size_t insertRGBA(const Config& config, NormalizedPalette& palette, RGBA32 rgba);

NormalizedTile candidate(const Config& config, const RGBATile& rgba, bool hFlip, bool vFlip);

NormalizedTile normalize(const Config& config, const RGBATile& rgba);

std::vector<IndexedNormTile> normalizeDecompTiles(const Config& config, const DecompiledTileset& decompiledTileset);

std::pair<std::unordered_map<BGR15, std::size_t>, std::unordered_map<std::size_t, BGR15>>
buildColorIndexMaps(const Config& config, const std::vector<IndexedNormTile>& normalizedTiles, const std::unordered_map<BGR15, std::size_t>& primaryIndexMap);

ColorSet toColorSet(const std::unordered_map<BGR15, std::size_t>& colorIndexMap, const NormalizedPalette& palette);

std::pair<std::vector<IndexedNormTileWithColorSet>, std::vector<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t>& colorIndexMap,
                             const std::vector<IndexedNormTile>& indexedNormalizedTiles);

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
bool assign(const Config& config, AssignState state, std::vector<ColorSet>& solution, const std::vector<ColorSet>& primaryPalettes);

GBATile makeTile(const NormalizedTile& normalizedTile, GBAPalette palette);
}

#endif // PORYTILES_COMPILER_HELPERS_H
