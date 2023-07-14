#include "compiler.h"

#include <png.hpp>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "doctest.h"
#include "config.h"
#include "ptexception.h"
#include "types.h"
#include "input_png.h"

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

// TODO : change this to receive CompilerContext once I have made that type available
static size_t insertRGBA(const Config& config, NormalizedPalette& palette, RGBA32 rgba) {
    /*
     * Insert an rgba32 color into a normalized palette. The color will be converted to bgr15 format in the process,
     * and possibly deduped (depending on user settings). Transparent alpha pixels will be treated as transparent, as
     * will pixels that are of transparent color (again, set by the user but default to magenta). Fails if a tile
     * contains too many unique colors or if an invalid alpha value is detected.
     */
    if (rgba.alpha == ALPHA_TRANSPARENT || rgba == config.transparencyColor) {
        return 0;
    }
    else if (rgba.alpha == ALPHA_OPAQUE) {
        /*
         * TODO : we lose color precision here, it would be nice to warn the user if two distinct RGBA colors they used
         * in the master sheet are going to collapse to one BGR color on the GBA. This should default fail the build,
         * but a compiler flag '--ignore-color-precision-loss' would disable this warning
         */
        auto bgr = rgbaToBgr(rgba);
        auto itrAtBgr = std::find(std::begin(palette.colors) + 1, std::begin(palette.colors) + palette.size, bgr);
        auto bgrPosInPalette = itrAtBgr - std::begin(palette.colors);
        if (bgrPosInPalette == palette.size) {
            // palette size will grow as we add to it
            if (palette.size == PAL_SIZE) {
                // TODO : better error context
                throw PtException{"too many unique colors in tile"};
            }
            palette.colors[palette.size++] = bgr;
        }
        return bgrPosInPalette;
    }
    // TODO : better error context
    throw PtException{"invalid alpha value: " + std::to_string(rgba.alpha)};
}

static NormalizedTile candidate(const Config& config, const RGBATile& rgba, bool hFlip, bool vFlip) {
    /*
     * NOTE: This only produces a _candidate_ normalized tile (a different choice of hFlip/vFlip might be the normal
     * form). We'll use this to generate candidates to find the true normal form.
     */
    NormalizedTile candidateTile{};
    // Size is 1 to account for transparent color in first palette slot
    candidateTile.palette.size = 1;
    // TODO : same color precision note as above in insertRGBA
    candidateTile.palette.colors[0] = rgbaToBgr(config.transparencyColor);
    candidateTile.hFlip = hFlip;
    candidateTile.vFlip = vFlip;

    for (std::size_t row = 0; row < TILE_SIDE_LENGTH; row++) {
        for (std::size_t col = 0; col < TILE_SIDE_LENGTH; col++) {
            std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH - 1 - row : row;
            std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH - 1 - col : col;
            candidateTile.setPixel(row, col,
                                   insertRGBA(config, candidateTile.palette, rgba.getPixel(rowWithFlip, colWithFlip)));
        }
    }

    return candidateTile;
}

static NormalizedTile normalize(const Config& config, const RGBATile& rgba) {
    /*
     * Normalize the given tile by checking each of the 4 possible flip states, and choosing the one that comes first in
     * "lexicographic" order, where this order is determined by the std::array spaceship operator.
     */
    auto noFlipsTile = candidate(config, rgba, false, false);

    // Short-circuit because transparent tiles are common in metatiles and trivially in normal form.
    if (noFlipsTile.transparent()) {
        return noFlipsTile;
    }

    auto hFlipTile = candidate(config, rgba, true, false);
    auto vFlipTile = candidate(config, rgba, false, true);
    auto bothFlipsTile = candidate(config, rgba, true, true);

    std::array<const NormalizedTile*, 4> candidates = {&noFlipsTile, &hFlipTile, &vFlipTile, &bothFlipsTile};
    auto normalizedTile = std::min_element(std::begin(candidates), std::end(candidates),
                                           [](auto tile1, auto tile2) { return tile1->pixels < tile2->pixels; });
    return **normalizedTile;
}

static std::vector<IndexedNormTile>
normalizeDecompTiles(const Config& config, const DecompiledTileset& decompiledTileset) {
    /*
     * For each tile in the decomp tileset, normalize it and tag it with its index in the decomp tileset.
     */
    std::vector<IndexedNormTile> normalizedTiles;
    DecompiledIndex decompiledIndex = 0;
    for (auto const& tile: decompiledTileset.tiles) {
        auto normalizedTile = normalize(config, tile);
        normalizedTiles.emplace_back(decompiledIndex++, normalizedTile);
    }
    return normalizedTiles;
}

static std::pair<std::unordered_map<BGR15, std::size_t>, std::unordered_map<std::size_t, BGR15>>
buildColorIndexMaps(const Config& config, const std::vector<IndexedNormTile>& normalizedTiles) {
    /*
     * Iterate over every color in each tile's NormalizedPalette, adding it to the map if not already present. We end up
     * with a map of colors to unique indexes.
     */
    std::unordered_map<BGR15, std::size_t> colorIndexes;
    std::unordered_map<std::size_t, BGR15> indexesToColors;
    std::size_t colorIndex = 0;
    for (const auto& [_, normalizedTile]: normalizedTiles) {
        // i starts at 1, since first color in each palette is the transparency color
        for (int i = 1; i < normalizedTile.palette.size; i++) {
            bool inserted = colorIndexes.insert(std::pair{normalizedTile.palette.colors[i], colorIndex}).second;
            if (inserted) {
                indexesToColors.insert(std::pair{colorIndex, normalizedTile.palette.colors[i]});
                colorIndex++;
            }
        }
    }
    // TODO : this needs to take into account secondary tilesets, so `numPalettesTotal - numPalettesInPrimary'
    if (colorIndex > (PAL_SIZE - 1) * config.numPalettesInPrimary) {
        // TODO : better error context
        throw PtException{"too many unique colors"};
    }

    return {colorIndexes, indexesToColors};
}

static ColorSet
toColorSet(const std::unordered_map<BGR15, std::size_t>& colorIndexMap, const NormalizedPalette& palette) {
    /*
     * Set a color set based on a given palette. Each bit in the ColorSet represents if the color at the given index in
     * the supplied color map was present in the palette. E.g. suppose the color map has 12 unique colors. The supplied
     * palette has two colors in it, which correspond to index 2 and index 11. The ColorSet bitset would be:
     * 0010 0000 0001
     */
    ColorSet colorSet;
    // starts at 1, skip the transparent color at slot 0 in the normalized palette
    for (int i = 1; i < palette.size; i++) {
        colorSet.set(colorIndexMap.at(palette.colors[i]));
    }
    return colorSet;
}

static std::pair<std::vector<IndexedNormTileWithColorSet>, std::unordered_set<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t>& colorIndexMap,
                             const std::vector<IndexedNormTile>& indexedNormalizedTiles) {
    std::vector<IndexedNormTileWithColorSet> indexedNormTilesWithColorSets;
    std::unordered_set<ColorSet> colorSets;
    for (const auto& [index, normalizedTile]: indexedNormalizedTiles) {
        // Compute the ColorSet for this normalized tile, then add it to our indexes
        auto colorSet = toColorSet(colorIndexMap, normalizedTile.palette);
        indexedNormTilesWithColorSets.emplace_back(index, normalizedTile, colorSet);
        colorSets.insert(colorSet);
    }
    return std::pair{indexedNormTilesWithColorSets, colorSets};
}

struct AssignState {
    /*
     * One color set for each hardware palette, bits in color set will indicate which colors this HW palette will have.
     * The size of the vector should be fixed to maxPalettes.
     */
    std::vector<ColorSet> hardwarePalettes;

    // The unique color sets from the NormalizedTiles
    std::vector<ColorSet> unassigned;
};

static bool assign(AssignState state, std::vector<ColorSet>& solution) {
    if (state.unassigned.empty()) {
        // No tiles left to assign, found a solution!
        std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes), std::back_inserter(solution));
        return true;
    }

    /*
     * We will try to assign the last element to one of the 6 hw palettes, last because it is a vector so easier to
     * add/remove from the end.
     */
    ColorSet& toAssign = state.unassigned.back();

    /*
     * For this next step, we want to sort the hw palettes before we try iterating. Sort them by the size of their
     * intersection with the toAssign ColorSet. Effectively, this means that we will always first try branching into an
     * assignment that re-uses hw palettes more effectively. We also have a tie-breaker heuristic for cases where two
     * palettes have the same intersect size. Right now we just use palette size, but in the future we may want to look
     * at color distances so we can pick a palette with more similar colors.
     */
    std::sort(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
              [&toAssign](const auto& pal1, const auto& pal2) {
                  std::size_t pal1IntersectSize = (pal1 & toAssign).count();
                  std::size_t pal2IntersectSize = (pal2 & toAssign).count();

                  /*
                   * TODO : Instead of just using palette count, maybe can we check for color distance here and try to choose the
                   * palette that has the "closest" colors to our toAssign palette? That might be a good heuristic for attempting
                   * to keep similar colors in the same palette. I.e. especially in cases where there are no palette
                   * intersections, it may be better to first try placing the new colors into a palette with similar colors rather
                   * than into the smallest palette
                   */
                  if (pal1IntersectSize == pal2IntersectSize) {
                      return pal1.count() < pal2.count();
                  }

                  return pal1IntersectSize > pal2IntersectSize;
              });

    for (size_t i = 0; i < state.hardwarePalettes.size(); i++) {
        ColorSet& palette = state.hardwarePalettes.at(i);

        // > PAL_SIZE - 1 because we need to save a slot for transparency
        if ((palette | toAssign).count() > PAL_SIZE - 1) {
            /*
             *  Skip this palette, cannot assign because there is not enough room in the palette. If we end up skipping
             * all of them that means the palettes are all too full and we cannot assign this tile in the state we are
             * in. The algorithm will be forced to backtrack and try other assignments.
             */
            continue;
        }

        /*
         * Prep the recursive call to assign(). If we got here, we know it is possible to assign toAssign to the palette
         * at hardwarePalettes[i]. So we make a copy of unassigned with toAssign removed and a copy of hardwarePalettes
         * with toAssigned assigned to the palette at index i. Then we call assign again with this updated state, and
         * return true if there is a valid solution somewhere down in this recursive branch.
         */
        std::vector<ColorSet> unassignedCopy;
        std::copy(std::begin(state.unassigned), std::end(state.unassigned), std::back_inserter(unassignedCopy));
        std::vector<ColorSet> hardwarePalettesCopy;
        std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                  std::back_inserter(hardwarePalettesCopy));
        unassignedCopy.pop_back();
        hardwarePalettesCopy[i] |= toAssign;
        AssignState updatedState = {hardwarePalettesCopy, unassignedCopy};

        if (assign(updatedState, solution)) {
            return true;
        }
    }

    /*
     * TODO : for any reasonably sized tileset, reaching this state takes AGES. We need some heuristics that abort the
     * search early if we are fairly confident there is no solution.
     */
    // No solution found
    return false;
}

static GBATile makeTile(const NormalizedTile& normalizedTile, GBAPalette palette) {
    GBATile gbaTile{};
    std::array<std::uint8_t, PAL_SIZE> paletteIndexes{};
    paletteIndexes[0] = 0;
    for (int i = 1; i < normalizedTile.palette.size; i++) {
        auto it = std::find(std::begin(palette.colors) + 1, std::end(palette.colors), normalizedTile.palette.colors[i]);
        if (it == std::end(palette.colors)) {
            // TODO : better error context
            throw std::runtime_error{"internal error"};
        }
        paletteIndexes[i] = it - std::begin(palette.colors);
    }
    for (std::size_t i = 0; i < normalizedTile.pixels.paletteIndexes.size(); i++) {
        gbaTile.paletteIndexes[i] = paletteIndexes[normalizedTile.pixels.paletteIndexes[i]];
    }
    return gbaTile;
}

CompiledTileset compile(const Config& config, const DecompiledTileset& decompiledTileset) {
    CompiledTileset compiled;
    // TODO : this needs to take into account secondary tilesets, so `numPalettesTotal - numPalettesInPrimary'
    compiled.palettes.resize(config.numPalettesInPrimary);
    compiled.assignments.resize(decompiledTileset.tiles.size());

    // Build helper data structures for the assignments
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, decompiledTileset);
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(config, indexedNormTiles);
    auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    // Run palette assignment
    // TODO : this needs to take into account secondary tilesets, so `numPalettesTotal - numPalettesInPrimary'
    // assignedPalsSolution is an out param that the assign function will populate when it finds a solution
    std::vector<ColorSet> assignedPalsSolution;
    assignedPalsSolution.reserve(config.numPalettesInPrimary);
    std::vector<ColorSet> logicalPalettes;
    logicalPalettes.resize(config.numPalettesInPrimary);
    std::vector<ColorSet> unassignedNormPalettes;
    std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
    std::sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
              [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
    AssignState state = {logicalPalettes, unassignedNormPalettes};
    bool assignSuccessful = assign(state, assignedPalsSolution);

    if (!assignSuccessful) {
        // TODO : better error context
        throw PtException{"failed to allocate palettes"};
    }

    /*
     * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile sharing)
     * and so we may need to do something fancier here so that the colors align correctly.
     */
    // TODO : this needs to take into account secondary tilesets, so `numPalettesTotal - numPalettesInPrimary'
    for (std::size_t i = 0; i < config.numPalettesInPrimary; i++) {
        ColorSet palAssignments = assignedPalsSolution.at(i);
        compiled.palettes[i].colors[0] = rgbaToBgr(config.transparencyColor);
        std::size_t colorIndex = 1;
        for (std::size_t j = 0; j < palAssignments.size(); j++) {
            if (palAssignments.test(j)) {
                compiled.palettes[i].colors[colorIndex] = indexToColor.at(j);
                colorIndex++;
            }
        }
    }

    /*
     * Build the tile assignments.
     */
    std::unordered_map<GBATile, std::size_t> tileIndexes;
    for (const auto& indexedNormTile: indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto& normTile = std::get<1>(indexedNormTile);
        auto& colorSet = std::get<2>(indexedNormTile);
        auto it = std::find_if(std::begin(assignedPalsSolution), std::end(assignedPalsSolution),
                               [&colorSet](const auto& assignedPal) {
                                   // Find which of the assignedSolution palettes this tile belongs to
                                   return (colorSet & ~assignedPal).none();
                               });
        if (it == std::end(assignedPalsSolution)) {
            // TODO : better error context
            throw std::runtime_error{"internal error"};
        }
        std::size_t paletteIndex = it - std::begin(assignedPalsSolution);
        GBATile gbaTile = makeTile(normTile, compiled.palettes[paletteIndex]);
        auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
        if (inserted.second) {
            compiled.tiles.push_back(gbaTile);
        }
        std::size_t tileIndex = inserted.first->second;
        compiled.assignments[index] = {tileIndex, paletteIndex, normTile.hFlip, normTile.vFlip};
    }

    return compiled;
}

}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("insertRGBA should add new colors in order and return the correct index for a given color") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    porytiles::NormalizedPalette palette1{};
    palette1.size = 1;
    palette1.colors = {};

    // invalid alpha value, must be opaque or transparent
    CHECK_THROWS_WITH_AS(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, 12}),
                         "invalid alpha value: 12",
                         const porytiles::PtException&);

    // Transparent should return 0
    CHECK(insertRGBA(config, palette1, porytiles::RGBA_MAGENTA) == 0);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}) == 0);

    // insert colors
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_OPAQUE}) == 1);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{8, 0, 0, porytiles::ALPHA_OPAQUE}) == 2);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{16, 0, 0, porytiles::ALPHA_OPAQUE}) == 3);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{24, 0, 0, porytiles::ALPHA_OPAQUE}) == 4);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{32, 0, 0, porytiles::ALPHA_OPAQUE}) == 5);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{40, 0, 0, porytiles::ALPHA_OPAQUE}) == 6);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{48, 0, 0, porytiles::ALPHA_OPAQUE}) == 7);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{56, 0, 0, porytiles::ALPHA_OPAQUE}) == 8);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{64, 0, 0, porytiles::ALPHA_OPAQUE}) == 9);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}) == 10);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{80, 0, 0, porytiles::ALPHA_OPAQUE}) == 11);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{88, 0, 0, porytiles::ALPHA_OPAQUE}) == 12);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{96, 0, 0, porytiles::ALPHA_OPAQUE}) == 13);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{104, 0, 0, porytiles::ALPHA_OPAQUE}) == 14);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}) == 15);

    // repeat colors should return their indexes
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}) == 10);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}) == 15);

    // Transparent should still return 0
    CHECK(insertRGBA(config, palette1, porytiles::RGBA_MAGENTA) == 0);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}) == 0);

    // Should throw, palette full
    CHECK_THROWS_WITH_AS(insertRGBA(config, palette1, porytiles::RGBA_CYAN),
                         "too many unique colors in tile",
                         const porytiles::PtException&);
}

TEST_CASE("candidate should return the NormalizedTile with requested flips") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    REQUIRE(std::filesystem::exists("res/tests/corners.png"));
    png::image<png::rgba_pixel> png1{"res/tests/corners.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    porytiles::RGBATile tile = tiles.tiles[0];

    SUBCASE("case: no flips") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, false, false);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 2);
        CHECK(candidate.pixels.paletteIndexes[21] == 5);
        CHECK(candidate.pixels.paletteIndexes[42] == 3);
        CHECK(candidate.pixels.paletteIndexes[45] == 1);
        CHECK(candidate.pixels.paletteIndexes[49] == 6);
        CHECK(candidate.pixels.paletteIndexes[54] == 7);
        CHECK(candidate.pixels.paletteIndexes[56] == 8);
        CHECK(candidate.pixels.paletteIndexes[63] == 5);
    }

    SUBCASE("case: hFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, true, false);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 1);
        CHECK(candidate.pixels.paletteIndexes[42] == 2);
        CHECK(candidate.pixels.paletteIndexes[45] == 4);
        CHECK(candidate.pixels.paletteIndexes[49] == 6);
        CHECK(candidate.pixels.paletteIndexes[54] == 7);
        CHECK(candidate.pixels.paletteIndexes[56] == 5);
        CHECK(candidate.pixels.paletteIndexes[63] == 8);
    }

    SUBCASE("case: vFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, false, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 6);
        CHECK(candidate.pixels.paletteIndexes[42] == 7);
        CHECK(candidate.pixels.paletteIndexes[45] == 2);
        CHECK(candidate.pixels.paletteIndexes[49] == 5);
        CHECK(candidate.pixels.paletteIndexes[54] == 8);
        CHECK(candidate.pixels.paletteIndexes[56] == 6);
        CHECK(candidate.pixels.paletteIndexes[63] == 7);
    }

    SUBCASE("case: hFlip and vFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, true, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 6);
        CHECK(candidate.pixels.paletteIndexes[42] == 1);
        CHECK(candidate.pixels.paletteIndexes[45] == 7);
        CHECK(candidate.pixels.paletteIndexes[49] == 8);
        CHECK(candidate.pixels.paletteIndexes[54] == 6);
        CHECK(candidate.pixels.paletteIndexes[56] == 7);
        CHECK(candidate.pixels.paletteIndexes[63] == 5);
    }
}

TEST_CASE("normalize should return the normal form of the given tile") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    REQUIRE(std::filesystem::exists("res/tests/corners.png"));
    png::image<png::rgba_pixel> png1{"res/tests/corners.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    porytiles::RGBATile tile = tiles.tiles[0];

    porytiles::NormalizedTile normalizedTile = porytiles::normalize(config, tile);
    CHECK(normalizedTile.palette.size == 9);
    CHECK_FALSE(normalizedTile.hFlip);
    CHECK_FALSE(normalizedTile.vFlip);
    CHECK(normalizedTile.pixels.paletteIndexes[0] == 1);
    CHECK(normalizedTile.pixels.paletteIndexes[7] == 2);
    CHECK(normalizedTile.pixels.paletteIndexes[9] == 3);
    CHECK(normalizedTile.pixels.paletteIndexes[14] == 4);
    CHECK(normalizedTile.pixels.paletteIndexes[18] == 2);
    CHECK(normalizedTile.pixels.paletteIndexes[21] == 5);
    CHECK(normalizedTile.pixels.paletteIndexes[42] == 3);
    CHECK(normalizedTile.pixels.paletteIndexes[45] == 1);
    CHECK(normalizedTile.pixels.paletteIndexes[49] == 6);
    CHECK(normalizedTile.pixels.paletteIndexes[54] == 7);
    CHECK(normalizedTile.pixels.paletteIndexes[56] == 8);
    CHECK(normalizedTile.pixels.paletteIndexes[63] == 5);
}

TEST_CASE("normalizeDecompTiles should correctly normalize all tiles in the decomp tileset") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);

    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, tiles);

    CHECK(indexedNormTiles.size() == 4);

    // First tile normal form is vFlipped, palette should have 2 colors
    CHECK(indexedNormTiles[0].second.pixels.paletteIndexes[0] == 0);
    CHECK(indexedNormTiles[0].second.pixels.paletteIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(indexedNormTiles[0].second.pixels.paletteIndexes[i] == 1);
    }
    CHECK(indexedNormTiles[0].second.palette.size == 2);
    CHECK(indexedNormTiles[0].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[0].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK_FALSE(indexedNormTiles[0].second.hFlip);
    CHECK(indexedNormTiles[0].second.vFlip);
    CHECK(indexedNormTiles[0].first == 0);

    // Second tile already in normal form, palette should have 3 colors
    CHECK(indexedNormTiles[1].second.pixels.paletteIndexes[0] == 0);
    CHECK(indexedNormTiles[1].second.pixels.paletteIndexes[54] == 1);
    CHECK(indexedNormTiles[1].second.pixels.paletteIndexes[55] == 1);
    CHECK(indexedNormTiles[1].second.pixels.paletteIndexes[62] == 1);
    CHECK(indexedNormTiles[1].second.pixels.paletteIndexes[63] == 2);
    CHECK(indexedNormTiles[1].second.palette.size == 3);
    CHECK(indexedNormTiles[1].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[1].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(indexedNormTiles[1].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK_FALSE(indexedNormTiles[1].second.hFlip);
    CHECK_FALSE(indexedNormTiles[1].second.vFlip);
    CHECK(indexedNormTiles[1].first == 1);

    // Third tile normal form is hFlipped, palette should have 3 colors
    CHECK(indexedNormTiles[2].second.pixels.paletteIndexes[0] == 0);
    CHECK(indexedNormTiles[2].second.pixels.paletteIndexes[7] == 1);
    CHECK(indexedNormTiles[2].second.pixels.paletteIndexes[56] == 1);
    CHECK(indexedNormTiles[2].second.pixels.paletteIndexes[63] == 2);
    CHECK(indexedNormTiles[2].second.palette.size == 3);
    CHECK(indexedNormTiles[2].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[2].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(indexedNormTiles[2].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK_FALSE(indexedNormTiles[2].second.vFlip);
    CHECK(indexedNormTiles[2].second.hFlip);
    CHECK(indexedNormTiles[2].first == 2);

    // Fourth tile normal form is hFlipped and vFlipped, palette should have 2 colors
    CHECK(indexedNormTiles[3].second.pixels.paletteIndexes[0] == 0);
    CHECK(indexedNormTiles[3].second.pixels.paletteIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(indexedNormTiles[3].second.pixels.paletteIndexes[i] == 1);
    }
    CHECK(indexedNormTiles[3].second.palette.size == 2);
    CHECK(indexedNormTiles[3].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[3].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(indexedNormTiles[3].second.hFlip);
    CHECK(indexedNormTiles[3].second.vFlip);
    CHECK(indexedNormTiles[3].first == 3);
}

TEST_CASE("buildColorIndexMap should build a map of all unique colors in the decomp tileset") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    std::vector<IndexedNormTile> normalizedTiles = normalizeDecompTiles(config, tiles);

    auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(config, normalizedTiles);

    CHECK(colorToIndex.size() == 4);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);
}

TEST_CASE("toColorSet should return the correct bitset based on the supplied palette") {
    std::unordered_map<porytiles::BGR15, std::size_t> colorIndexMap = {
            {porytiles::rgbaToBgr(porytiles::RGBA_BLUE),   0},
            {porytiles::rgbaToBgr(porytiles::RGBA_RED),    1},
            {porytiles::rgbaToBgr(porytiles::RGBA_GREEN),  2},
            {porytiles::rgbaToBgr(porytiles::RGBA_CYAN),   3},
            {porytiles::rgbaToBgr(porytiles::RGBA_YELLOW), 4},
    };

    SUBCASE("palette 1") {
        porytiles::NormalizedPalette palette{};
        palette.size = 2;
        palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
        palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_RED);

        ColorSet colorSet = porytiles::toColorSet(colorIndexMap, palette);
        CHECK(colorSet.count() == 1);
        CHECK(colorSet.test(1));
    }

    SUBCASE("palette 2") {
        porytiles::NormalizedPalette palette{};
        palette.size = 4;
        palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
        palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_YELLOW);
        palette.colors[2] = porytiles::rgbaToBgr(porytiles::RGBA_GREEN);
        palette.colors[3] = porytiles::rgbaToBgr(porytiles::RGBA_CYAN);

        ColorSet colorSet = porytiles::toColorSet(colorIndexMap, palette);
        CHECK(colorSet.count() == 3);
        CHECK(colorSet.test(4));
        CHECK(colorSet.test(2));
        CHECK(colorSet.test(3));
    }
}

TEST_CASE("matchNormalizedWithColorSets should return the expected data structures") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 6;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, tiles);
    auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(config, indexedNormTiles);

    CHECK(colorToIndex.size() == 4);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);

    auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    CHECK(indexedNormTilesWithColorSets.size() == 4);
    // colorSets size is 3 because first and fourth tiles have the same palette
    CHECK(colorSets.size() == 3);

    // First tile has 1 non-transparent color, color should be BLUE
    CHECK(std::get<0>(indexedNormTilesWithColorSets[0]) == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).pixels.paletteIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).pixels.paletteIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).pixels.paletteIndexes[i] == 1);
    }
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.size == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[0]).hFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).count() == 1);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).test(0));
    CHECK(colorSets.contains(std::get<2>(indexedNormTilesWithColorSets[0])));

    // Second tile has two non-transparent colors, RED and GREEN
    CHECK(std::get<0>(indexedNormTilesWithColorSets[1]) == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).pixels.paletteIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).pixels.paletteIndexes[54] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).pixels.paletteIndexes[55] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).pixels.paletteIndexes[62] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).pixels.paletteIndexes[63] == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.size == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).hFlip);
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).count() == 2);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).test(1));
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).test(2));
    CHECK(colorSets.contains(std::get<2>(indexedNormTilesWithColorSets[1])));

    // Third tile has two non-transparent colors, CYAN and GREEN
    CHECK(std::get<0>(indexedNormTilesWithColorSets[2]) == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).pixels.paletteIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).pixels.paletteIndexes[7] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).pixels.paletteIndexes[56] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).pixels.paletteIndexes[63] == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.size == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[2] ==
          porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[2]).vFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).hFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).count() == 2);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).test(1));
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).test(3));
    CHECK(colorSets.contains(std::get<2>(indexedNormTilesWithColorSets[2])));

    // Fourth tile has 1 non-transparent color, color should be BLUE
    CHECK(std::get<0>(indexedNormTilesWithColorSets[3]) == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).pixels.paletteIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).pixels.paletteIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).pixels.paletteIndexes[i] == 1);
    }
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.size == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).hFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).count() == 1);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).test(0));
    CHECK(colorSets.contains(std::get<2>(indexedNormTilesWithColorSets[3])));
}

TEST_CASE("assignTest should correctly assign all normalized palettes or fail if impossible") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;

    SUBCASE("It should successfully allocate a simple 2x2 tileset png") {
        constexpr int SOLUTION_SIZE = 2;
        config.numPalettesInPrimary = SOLUTION_SIZE;
        REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
        png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
        porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
        std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, tiles);
        auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(config, indexedNormTiles);
        auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

        // Set up the state struct
        std::vector<ColorSet> solution;
        solution.reserve(SOLUTION_SIZE);
        std::vector<ColorSet> hardwarePalettes;
        hardwarePalettes.resize(SOLUTION_SIZE);
        std::vector<ColorSet> unassigned;
        std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
        std::sort(std::begin(unassigned), std::end(unassigned),
                  [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
        porytiles::AssignState state = {hardwarePalettes, unassigned};

        CHECK(porytiles::assign(state, solution));
        CHECK(solution.size() == SOLUTION_SIZE);
        CHECK(solution.at(0).count() == 1);
        CHECK(solution.at(1).count() == 3);
        CHECK(solution.at(0).test(0));
        CHECK(solution.at(1).test(1));
        CHECK(solution.at(1).test(2));
        CHECK(solution.at(1).test(3));
    }

    /*
     * TODO : this subcase fails CI right now. Currently, it only passes locally with Clang. Locally it fails with GCC
     * and in CI it fails with GCC and Clang. This is probably because of the custom spaceship operator I have for local
     * Clang. As far as I can tell, local GCC gives the exact same result as CI GCC and CI Clang. Also, the difference
     * in final tileset size is likely because Local/CI GCC and CI Clang are able to accidentally make use of sibling
     * tiles. It will be eaiser to see what is happening once I can write out palette files and a tiles.png, so let's
     * return to this issue later. Ideally, Clang will finally support <=> and then we don't have to roll our own crap
     * custom operator
     */
    SUBCASE("It should successfully allocate a large, complex PNG") {
        // constexpr int SOLUTION_SIZE = 5;
        // config.numPalettesInPrimary = SOLUTION_SIZE;
        // REQUIRE(std::filesystem::exists("res/tests/primary_set.png"));
        // png::image<png::rgba_pixel> png1{"res/tests/primary_set.png"};
        // porytiles::DecompiledTileset tiles = porytiles::importTilesFrom(png1);
        // std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, tiles);
        // auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(config, indexedNormTiles);
        // auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

        // // Set up the state struct
        // std::vector<ColorSet> solution;
        // solution.reserve(SOLUTION_SIZE);
        // std::vector<ColorSet> hardwarePalettes;
        // hardwarePalettes.resize(SOLUTION_SIZE);
        // std::vector<ColorSet> unassigned;
        // std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
        // std::sort(std::begin(unassigned), std::end(unassigned),
        //           [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
        // porytiles::AssignState state = {hardwarePalettes, unassigned};

        // CHECK(porytiles::assign(state, solution));
        // CHECK(solution.size() == SOLUTION_SIZE);
        // CHECK(solution.at(0).count() == 12);
        // CHECK(solution.at(1).count() == 12);
        // CHECK(solution.at(2).count() == 13);
        // CHECK(solution.at(3).count() == 14);
        // CHECK(solution.at(4).count() == 15);
    }
}

TEST_CASE("makeTile should create the expected GBATile from the given NormalizedTile and GBAPalette") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 2;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, tiles);
    porytiles::CompiledTileset compiledTiles = porytiles::compile(config, tiles);

    porytiles::GBATile tile0 = porytiles::makeTile(indexedNormTiles[0].second, compiledTiles.palettes[0]);
    CHECK_FALSE(indexedNormTiles[0].second.hFlip);
    CHECK(indexedNormTiles[0].second.vFlip);
    CHECK(tile0.paletteIndexes[0] == 0);
    CHECK(tile0.paletteIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile0.paletteIndexes[i] == 1);
    }

    porytiles::GBATile tile1 = porytiles::makeTile(indexedNormTiles[1].second, compiledTiles.palettes[1]);
    CHECK_FALSE(indexedNormTiles[1].second.hFlip);
    CHECK_FALSE(indexedNormTiles[1].second.vFlip);
    CHECK(tile1.paletteIndexes[0] == 0);
    CHECK(tile1.paletteIndexes[54] == 1);
    CHECK(tile1.paletteIndexes[55] == 1);
    CHECK(tile1.paletteIndexes[62] == 1);
    CHECK(tile1.paletteIndexes[63] == 2);

    porytiles::GBATile tile2 = porytiles::makeTile(indexedNormTiles[2].second, compiledTiles.palettes[1]);
    CHECK(indexedNormTiles[2].second.hFlip);
    CHECK_FALSE(indexedNormTiles[2].second.vFlip);
    CHECK(tile2.paletteIndexes[0] == 0);
    CHECK(tile2.paletteIndexes[7] == 3);
    CHECK(tile2.paletteIndexes[56] == 3);
    CHECK(tile2.paletteIndexes[63] == 1);

    porytiles::GBATile tile3 = porytiles::makeTile(indexedNormTiles[3].second, compiledTiles.palettes[0]);
    CHECK(indexedNormTiles[3].second.hFlip);
    CHECK(indexedNormTiles[3].second.vFlip);
    CHECK(tile3.paletteIndexes[0] == 0);
    CHECK(tile3.paletteIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile3.paletteIndexes[i] == 1);
    }
}

TEST_CASE("compile function should assign all tiles as expected") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 2;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFrom(png1);
    porytiles::CompiledTileset compiledTiles = porytiles::compile(config, tiles);

    // Check that compiled palettes are as expected
    CHECK(compiledTiles.palettes.at(0).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledTiles.palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledTiles.palettes.at(1).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledTiles.palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledTiles.palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledTiles.palettes.at(1).colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));

    /*
     * Check that compiled GBATiles have expected index values, there are only 3 in final tileset since two of the
     * original tiles are flips of each other.
     */
    porytiles::GBATile& tile0 = compiledTiles.tiles[0];
    CHECK(tile0.paletteIndexes[0] == 0);
    CHECK(tile0.paletteIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile0.paletteIndexes[i] == 1);
    }

    porytiles::GBATile tile1 = compiledTiles.tiles[1];
    CHECK(tile1.paletteIndexes[0] == 0);
    CHECK(tile1.paletteIndexes[54] == 1);
    CHECK(tile1.paletteIndexes[55] == 1);
    CHECK(tile1.paletteIndexes[62] == 1);
    CHECK(tile1.paletteIndexes[63] == 2);

    porytiles::GBATile tile2 = compiledTiles.tiles[2];
    CHECK(tile2.paletteIndexes[0] == 0);
    CHECK(tile2.paletteIndexes[7] == 3);
    CHECK(tile2.paletteIndexes[56] == 3);
    CHECK(tile2.paletteIndexes[63] == 1);

    /*
     * Check that all the assignments are correct.
     */
    CHECK(compiledTiles.assignments[0].tileIndex == 0);
    CHECK(compiledTiles.assignments[0].paletteIndex == 0);
    CHECK_FALSE(compiledTiles.assignments[0].hFlip);
    CHECK(compiledTiles.assignments[0].vFlip);

    CHECK(compiledTiles.assignments[1].tileIndex == 1);
    CHECK(compiledTiles.assignments[1].paletteIndex == 1);
    CHECK_FALSE(compiledTiles.assignments[1].hFlip);
    CHECK_FALSE(compiledTiles.assignments[1].vFlip);

    CHECK(compiledTiles.assignments[2].tileIndex == 2);
    CHECK(compiledTiles.assignments[2].paletteIndex == 1);
    CHECK(compiledTiles.assignments[2].hFlip);
    CHECK_FALSE(compiledTiles.assignments[2].vFlip);

    CHECK(compiledTiles.assignments[3].tileIndex == 0);
    CHECK(compiledTiles.assignments[3].paletteIndex == 0);
    CHECK(compiledTiles.assignments[3].hFlip);
    CHECK(compiledTiles.assignments[3].vFlip);
}

TEST_CASE("CompileComplexTest") {
    // TODO : name this test and actually check things
//    porytiles::Config config{};
//    config.transparencyColor = porytiles::RGBA_MAGENTA;
//    config.numPalettesInPrimary = 5;

//    REQUIRE(std::filesystem::exists("res/tests/primary_set.png"));
//    png::image<png::rgba_pixel> png1{"res/tests/primary_set.png"};
//    porytiles::DecompiledTileset tiles = porytiles::importTilesFrom(png1);
//    porytiles::CompiledTileset compiledTiles = porytiles::compile(config, tiles);

//    std::cout << "COMPILED TILES" << std::endl;
//    std::cout << "tiles size: " << compiledTiles.tiles.size() << std::endl;
//    for (std::size_t i = 0; i < compiledTiles.palettes.size(); i++) {
//        porytiles::GBAPalette& palette = compiledTiles.palettes.at(i);
//        std::cout << "Palette " << i << std::endl;
//        for (std::size_t j = 0; j < palette.colors.size(); j++) {
//            std::cout << porytiles::bgrToRgba(palette.colors.at(j)) << std::endl;
//        }
//    }
//    for (std::size_t i = 0; i < compiledTiles.assignments.size(); i++) {
//        auto& assignment = compiledTiles.assignments.at(i);
//        std::cout << "assignment[" << i
//                  << "]={tile=" << assignment.tileIndex << ",pal=" << assignment.paletteIndex
//                  << ",hFlip=" << assignment.hFlip << ",vFlip=" << assignment.vFlip << "}" << std::endl;
//    }
}
