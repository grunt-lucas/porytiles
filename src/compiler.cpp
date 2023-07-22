#include "compiler.h"

#include <png.hpp>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <bitset>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>

#include "doctest.h"
#include "config.h"
#include "ptexception.h"
#include "types.h"
#include "importer.h"
#include "compiler_helpers.h"

namespace porytiles {
CompiledTileset compile(const Config& config, const DecompiledTileset& decompiledTileset) {
    CompiledTileset compiled;
    compiled.palettes.resize(config.numPalettesInPrimary);
    // TODO : we should compute the number of metatiles here and throw if the user tilesheet exceeds the max
    compiled.assignments.resize(decompiledTileset.tiles.size());

    // Build helper data structures for the assignments
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, decompiledTileset);
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(config, indexedNormTiles, {});
    compiled.colorIndexMap = colorToIndex;
    auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    // Run palette assignment
    // assignedPalsSolution is an out param that the assign function will populate when it finds a solution
    std::vector<ColorSet> assignedPalsSolution;
    assignedPalsSolution.reserve(config.numPalettesInPrimary);
    std::vector<ColorSet> logicalPalettes;
    logicalPalettes.resize(config.numPalettesInPrimary);
    std::vector<ColorSet> unassignedNormPalettes;
    std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
    std::stable_sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
              [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
    AssignState state = {logicalPalettes, unassignedNormPalettes};
    gRecurseCount = 0;
    bool assignSuccessful = assign(config, state, assignedPalsSolution, {});

    if (!assignSuccessful) {
        // TODO : better error context
        throw PtException{"failed to allocate palettes"};
    }

    /*
     * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile sharing)
     * and so we may need to do something fancier here so that the colors align correctly.
     */
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
    // force tile 0 to be a transparent tile that uses palette 0
    tileIndexes.insert({GBA_TILE_TRANSPARENT, 0});
    compiled.tiles.push_back(GBA_TILE_TRANSPARENT);
    compiled.paletteIndexes.push_back(0);
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
        // insert only updates the map if the key is not already present
        auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
        if (inserted.second) {
            compiled.tiles.push_back(gbaTile);
            if (compiled.tiles.size() > config.numTilesInPrimary) {
                // TODO : better error context
                throw PtException{"too many tiles: " + std::to_string(compiled.tiles.size()) + " > " + std::to_string(config.numTilesInPrimary)};
            }
            compiled.paletteIndexes.push_back(paletteIndex);
        }
        std::size_t tileIndex = inserted.first->second;
        compiled.assignments[index] = {tileIndex, paletteIndex, normTile.hFlip, normTile.vFlip};
    }
    compiled.tileIndexes = tileIndexes;

    return compiled;
}

CompiledTileset compileSecondary(const Config& config, const DecompiledTileset& decompiledTileset, const CompiledTileset& primaryTileset) {
    if (config.numPalettesInPrimary != primaryTileset.palettes.size()) {
        // TODO : better error context
        throw std::runtime_error{"config.numPalettesInPrimary did not match primary palette set size"};
    }

    CompiledTileset compiled;
    compiled.palettes.resize(config.numPalettesInSecondary());
    // TODO : we should compute the number of metatiles here and throw if the user tilesheet exceeds the max
    compiled.assignments.resize(decompiledTileset.tiles.size());

    // Build helper data structures for the assignments
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, decompiledTileset);
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(config, indexedNormTiles, primaryTileset.colorIndexMap);
    auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    // Run palette assignment
    // assignedPalsSolution is an out param that the assign function will populate when it finds a solution
    std::vector<ColorSet> assignedPalsSolution;
    assignedPalsSolution.reserve(config.numPalettesInSecondary());
    std::vector<ColorSet> logicalPalettes;
    logicalPalettes.resize(config.numPalettesInSecondary());
    std::vector<ColorSet> unassignedNormPalettes;
    std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
    std::stable_sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
              [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
    AssignState state = {logicalPalettes, unassignedNormPalettes};
    /*
     * Construct ColorSets for the primary palettes, assign can use these to decide if a tile is entirely covered by a
     * primary palette and hence does not need to extend the search by assigning its colors to one of the new secondary
     * palettes.
     */
    std::vector<ColorSet> primaryPaletteColorSets{};
    for (const auto& gbaPalette : primaryTileset.palettes) {
        // starts at 1, skip the transparent color at slot 0 in the palette
        for (std::size_t i = 1; i < gbaPalette.colors.size(); i++) {
            primaryPaletteColorSets[i].set(colorToIndex.at(gbaPalette.colors[i]));
        }
    }
    gRecurseCount = 0;
    bool assignSuccessful = assign(config, state, assignedPalsSolution, primaryPaletteColorSets);

    if (!assignSuccessful) {
        // TODO : better error context
        throw PtException{"failed to allocate palettes"};
    }

    /*
     * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile sharing)
     * and so we may need to do something fancier here so that the colors align correctly.
     */
    for (std::size_t i = 0; i < config.numPalettesInSecondary(); i++) {
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
    // TODO : fill in the code here
    // std::unordered_map<GBATile, std::size_t> tileIndexes;
    // for (const auto& indexedNormTile: indexedNormTilesWithColorSets) {
    // }

    return compiled;
}
}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("compile function should assign all tiles as expected") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 2;
    config.numTilesInPrimary = 4;
    config.secondary = false;
    config.maxRecurseCount = 5;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
    porytiles::CompiledTileset compiledTiles = porytiles::compile(config, tiles);

    // Check that compiled palettes are as expected
    CHECK(compiledTiles.palettes.at(0).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledTiles.palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledTiles.palettes.at(1).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledTiles.palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledTiles.palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledTiles.palettes.at(1).colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));

    /*
     * Check that compiled GBATiles have expected index values, there are only 3 in final tileset (ignoring the
     * transparent tile at the start) since two of the original tiles are flips of each other.
     */
    porytiles::GBATile& tile0 = compiledTiles.tiles[0];
    for (size_t i = 0; i < 64; i++) {
        CHECK(tile0.paletteIndexes[i] == 0);
    }

    porytiles::GBATile& tile1 = compiledTiles.tiles[1];
    CHECK(tile1.paletteIndexes[0] == 0);
    CHECK(tile1.paletteIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile1.paletteIndexes[i] == 1);
    }

    porytiles::GBATile tile2 = compiledTiles.tiles[2];
    CHECK(tile2.paletteIndexes[0] == 0);
    CHECK(tile2.paletteIndexes[54] == 1);
    CHECK(tile2.paletteIndexes[55] == 1);
    CHECK(tile2.paletteIndexes[62] == 1);
    CHECK(tile2.paletteIndexes[63] == 2);

    porytiles::GBATile tile3 = compiledTiles.tiles[3];
    CHECK(tile3.paletteIndexes[0] == 0);
    CHECK(tile3.paletteIndexes[7] == 3);
    CHECK(tile3.paletteIndexes[56] == 3);
    CHECK(tile3.paletteIndexes[63] == 1);

    /*
     * Check that all the assignments are correct.
     */
    CHECK(compiledTiles.assignments[0].tileIndex == 1);
    CHECK(compiledTiles.assignments[0].paletteIndex == 0);
    CHECK_FALSE(compiledTiles.assignments[0].hFlip);
    CHECK(compiledTiles.assignments[0].vFlip);

    CHECK(compiledTiles.assignments[1].tileIndex == 2);
    CHECK(compiledTiles.assignments[1].paletteIndex == 1);
    CHECK_FALSE(compiledTiles.assignments[1].hFlip);
    CHECK_FALSE(compiledTiles.assignments[1].vFlip);

    CHECK(compiledTiles.assignments[2].tileIndex == 3);
    CHECK(compiledTiles.assignments[2].paletteIndex == 1);
    CHECK(compiledTiles.assignments[2].hFlip);
    CHECK_FALSE(compiledTiles.assignments[2].vFlip);

    CHECK(compiledTiles.assignments[3].tileIndex == 1);
    CHECK(compiledTiles.assignments[3].paletteIndex == 0);
    CHECK(compiledTiles.assignments[3].hFlip);
    CHECK(compiledTiles.assignments[3].vFlip);
}

TEST_CASE("CompileComplexTest") {
    // TODO : name this test and actually check things
//    porytiles::Config config = porytiles::defaultConfig();

//    REQUIRE(std::filesystem::exists("res/tests/compile_raw_set_1/set.png"));
//    png::image<png::rgba_pixel> png1{"res/tests/compile_raw_set_1/set.png"};
//    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
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
