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
        compiled.palettes[i].size = colorIndex;
    }

    /*
     * Build the tile assignments.
     */
    std::unordered_map<GBATile, std::size_t> tileIndexes;
    // force tile 0 to be a transparent tile that uses palette 0
    tileIndexes.insert({GBA_TILE_TRANSPARENT, 0});
    compiled.tiles.push_back(GBA_TILE_TRANSPARENT);
    compiled.paletteIndexesOfTile.push_back(0);
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
            compiled.paletteIndexesOfTile.push_back(paletteIndex);
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
    compiled.palettes.resize(config.numPalettesTotal);
    // TODO : we should compute the number of metatiles here and throw if the user tilesheet exceeds the max
    compiled.assignments.resize(decompiledTileset.tiles.size());

    // Build helper data structures for the assignments
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, decompiledTileset);
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(config, indexedNormTiles, primaryTileset.colorIndexMap);
    compiled.colorIndexMap = colorToIndex;
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
    primaryPaletteColorSets.reserve(primaryTileset.palettes.size());
    for (std::size_t i = 0; i < primaryTileset.palettes.size(); i++) {
        const auto& gbaPalette = primaryTileset.palettes[i];
        primaryPaletteColorSets.emplace_back();
        for (std::size_t j = 1; j < gbaPalette.size; j++) {
            primaryPaletteColorSets[i].set(colorToIndex.at(gbaPalette.colors[j]));
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
    for (std::size_t i = 0; i < config.numPalettesInPrimary; i++) {
        // Copy the primary set's palettes into this tileset so tiles can use them
        for (std::size_t j = 0; j < PAL_SIZE; j++) {
            compiled.palettes[i].colors[j] = primaryTileset.palettes[i].colors[j];
        }
    }
    for (std::size_t i = config.numPalettesInPrimary; i < config.numPalettesTotal; i++) {
        ColorSet palAssignments = assignedPalsSolution.at(i - config.numPalettesInPrimary);
        compiled.palettes[i].colors[0] = rgbaToBgr(config.transparencyColor);
        std::size_t colorIndex = 1;
        for (std::size_t j = 0; j < palAssignments.size(); j++) {
            if (palAssignments.test(j)) {
                compiled.palettes[i].colors[colorIndex] = indexToColor.at(j);
                colorIndex++;
            }
        }
        compiled.palettes[i].size = colorIndex;
    }

    /*
     * Build the tile assignments.
     */
    std::vector<ColorSet> allColorSets{};
    allColorSets.insert(allColorSets.end(), primaryPaletteColorSets.begin(), primaryPaletteColorSets.end());
    allColorSets.insert(allColorSets.end(), assignedPalsSolution.begin(), assignedPalsSolution.end());
    std::unordered_map<GBATile, std::size_t> tileIndexes;
    for (const auto& indexedNormTile: indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto& normTile = std::get<1>(indexedNormTile);
        auto& colorSet = std::get<2>(indexedNormTile);
        auto it = std::find_if(std::begin(allColorSets), std::end(allColorSets),
                               [&colorSet](const auto& assignedPal) {
                                   // Find which of the allColorSets palettes this tile belongs to
                                   return (colorSet & ~assignedPal).none();
                               });
        if (it == std::end(allColorSets)) {
            // TODO : better error context
            throw std::runtime_error{"internal error"};
        }
        std::size_t paletteIndex = it - std::begin(allColorSets);
        GBATile gbaTile = makeTile(normTile, compiled.palettes[paletteIndex]);
        if (primaryTileset.tileIndexes.find(gbaTile) != primaryTileset.tileIndexes.end()) {
            // Tile was in the primary set
            compiled.assignments[index] = {primaryTileset.tileIndexes.at(gbaTile), paletteIndex,
                                           normTile.hFlip, normTile.vFlip};
        }
        else {
            // Tile was in the secondary set
            auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
            if (inserted.second) {
                compiled.tiles.push_back(gbaTile);
                if (compiled.tiles.size() > config.numTilesInPrimary) {
                    // TODO : better error context
                    throw PtException{"too many tiles: " + std::to_string(compiled.tiles.size()) + " > " + std::to_string(config.numTilesInPrimary)};
                }
                compiled.paletteIndexesOfTile.push_back(paletteIndex);
            }
            std::size_t tileIndex = inserted.first->second;
            // Offset the tile index by the secondary tileset VRAM location, which is just the size of the primary tiles
            compiled.assignments[index] = {tileIndex + config.numTilesInPrimary, paletteIndex, normTile.hFlip, normTile.vFlip};
        }
    }
    compiled.tileIndexes = tileIndexes;

    return compiled;
}
}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("compile simple example should perform as expected") {
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
        CHECK(tile0.colorIndexes[i] == 0);
    }

    porytiles::GBATile& tile1 = compiledTiles.tiles[1];
    CHECK(tile1.colorIndexes[0] == 0);
    CHECK(tile1.colorIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile1.colorIndexes[i] == 1);
    }

    porytiles::GBATile tile2 = compiledTiles.tiles[2];
    CHECK(tile2.colorIndexes[0] == 0);
    CHECK(tile2.colorIndexes[54] == 1);
    CHECK(tile2.colorIndexes[55] == 1);
    CHECK(tile2.colorIndexes[62] == 1);
    CHECK(tile2.colorIndexes[63] == 2);

    porytiles::GBATile tile3 = compiledTiles.tiles[3];
    CHECK(tile3.colorIndexes[0] == 0);
    CHECK(tile3.colorIndexes[7] == 3);
    CHECK(tile3.colorIndexes[56] == 3);
    CHECK(tile3.colorIndexes[63] == 1);

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

TEST_CASE("compile function should fill out CompiledTileset struct with expected values") {
    porytiles::Config config = porytiles::defaultConfig();
    config.numPalettesInPrimary = 3;
    config.numPalettesTotal = 6;

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_primary.png"));
    png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/bottom_primary.png"};
    png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/middle_primary.png"};
    png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/top_primary.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(bottomPrimary, middlePrimary,
                                                                                           topPrimary);
    porytiles::CompiledTileset compiledPrimary = porytiles::compile(config, decompiledPrimary);

    // Check that tiles are as expected
    CHECK(compiledPrimary.tiles.size() == 5);
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary_expected_tiles.png"));
    png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/primary_expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledPrimary.tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
                CHECK(
                    compiledPrimary.tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
                    expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]
                );
            }
        }
    }

    // Check that paletteIndexesOfTile are correct
    CHECK(compiledPrimary.paletteIndexesOfTile.size() == 5);
    CHECK(compiledPrimary.paletteIndexesOfTile[0] == 0);
    CHECK(compiledPrimary.paletteIndexesOfTile[1] == 2);
    CHECK(compiledPrimary.paletteIndexesOfTile[2] == 1);
    CHECK(compiledPrimary.paletteIndexesOfTile[3] == 1);
    CHECK(compiledPrimary.paletteIndexesOfTile[4] == 0);

    // Check that compiled palettes are as expected
    CHECK(compiledPrimary.palettes.size() == config.numPalettesInPrimary);
    CHECK(compiledPrimary.palettes.at(0).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledPrimary.palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
    CHECK(compiledPrimary.palettes.at(1).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledPrimary.palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledPrimary.palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledPrimary.palettes.at(2).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledPrimary.palettes.at(2).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledPrimary.palettes.at(2).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));

    // Check that all assignments are correct
    CHECK(compiledPrimary.assignments.size() == porytiles::METATILES_IN_ROW * config.numTilesPerMetatile);

    CHECK(compiledPrimary.assignments[0].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[0].vFlip);
    CHECK(compiledPrimary.assignments[0].tileIndex == 1);
    CHECK(compiledPrimary.assignments[0].paletteIndex == 2);

    CHECK_FALSE(compiledPrimary.assignments[1].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[1].vFlip);
    CHECK(compiledPrimary.assignments[1].tileIndex == 0);
    CHECK(compiledPrimary.assignments[1].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[2].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[2].vFlip);
    CHECK(compiledPrimary.assignments[2].tileIndex == 0);
    CHECK(compiledPrimary.assignments[2].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[3].hFlip);
    CHECK(compiledPrimary.assignments[3].vFlip);
    CHECK(compiledPrimary.assignments[3].tileIndex == 2);
    CHECK(compiledPrimary.assignments[3].paletteIndex == 1);

    CHECK_FALSE(compiledPrimary.assignments[4].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[4].vFlip);
    CHECK(compiledPrimary.assignments[4].tileIndex == 0);
    CHECK(compiledPrimary.assignments[4].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[5].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[5].vFlip);
    CHECK(compiledPrimary.assignments[5].tileIndex == 0);
    CHECK(compiledPrimary.assignments[5].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[6].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[6].vFlip);
    CHECK(compiledPrimary.assignments[6].tileIndex == 3);
    CHECK(compiledPrimary.assignments[6].paletteIndex == 1);

    CHECK_FALSE(compiledPrimary.assignments[7].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[7].vFlip);
    CHECK(compiledPrimary.assignments[7].tileIndex == 0);
    CHECK(compiledPrimary.assignments[7].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[8].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[8].vFlip);
    CHECK(compiledPrimary.assignments[8].tileIndex == 0);
    CHECK(compiledPrimary.assignments[8].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[9].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[9].vFlip);
    CHECK(compiledPrimary.assignments[9].tileIndex == 4);
    CHECK(compiledPrimary.assignments[9].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[10].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[10].vFlip);
    CHECK(compiledPrimary.assignments[10].tileIndex == 0);
    CHECK(compiledPrimary.assignments[10].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary.assignments[11].hFlip);
    CHECK_FALSE(compiledPrimary.assignments[11].vFlip);
    CHECK(compiledPrimary.assignments[11].tileIndex == 0);
    CHECK(compiledPrimary.assignments[11].paletteIndex == 0);

    for (std::size_t index = config.numTilesPerMetatile; index < porytiles::METATILES_IN_ROW * config.numTilesPerMetatile; index++) {
        CHECK_FALSE(compiledPrimary.assignments[index].hFlip);
        CHECK_FALSE(compiledPrimary.assignments[index].vFlip);
        CHECK(compiledPrimary.assignments[index].tileIndex == 0);
        CHECK(compiledPrimary.assignments[index].paletteIndex == 0);
    }

    // Check that colorIndexMap is correct
    CHECK(compiledPrimary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 0);
    CHECK(compiledPrimary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_YELLOW)] == 1);
    CHECK(compiledPrimary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 2);
    CHECK(compiledPrimary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 3);
    CHECK(compiledPrimary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_WHITE)] == 4);

    // Check that tileIndexes is correct
    CHECK(compiledPrimary.tileIndexes.size() == compiledPrimary.tiles.size());
    CHECK(compiledPrimary.tileIndexes[compiledPrimary.tiles[0]] == 0);
    CHECK(compiledPrimary.tileIndexes[compiledPrimary.tiles[1]] == 1);
    CHECK(compiledPrimary.tileIndexes[compiledPrimary.tiles[2]] == 2);
    CHECK(compiledPrimary.tileIndexes[compiledPrimary.tiles[3]] == 3);
    CHECK(compiledPrimary.tileIndexes[compiledPrimary.tiles[4]] == 4);
}

TEST_CASE("compileSecondary function should fill out CompiledTileset struct with expected values") {
    porytiles::Config config = porytiles::defaultConfig();
    config.numPalettesInPrimary = 3;
    config.numPalettesTotal = 6;

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_primary.png"));
    png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/bottom_primary.png"};
    png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/middle_primary.png"};
    png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/top_primary.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(bottomPrimary, middlePrimary,
                                                                                           topPrimary);
    porytiles::CompiledTileset compiledPrimary = porytiles::compile(config, decompiledPrimary);

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_secondary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_secondary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_secondary.png"));
    png::image<png::rgba_pixel> bottomSecondary{"res/tests/simple_metatiles_3/bottom_secondary.png"};
    png::image<png::rgba_pixel> middleSecondary{"res/tests/simple_metatiles_3/middle_secondary.png"};
    png::image<png::rgba_pixel> topSecondary{"res/tests/simple_metatiles_3/top_secondary.png"};
    porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(bottomSecondary,
                                                                                             middleSecondary,
                                                                                             topSecondary);
    porytiles::CompiledTileset compiledSecondary = porytiles::compileSecondary(config, decompiledSecondary,
                                                                               compiledPrimary);

    // Check that tiles are as expected
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary_expected_tiles.png"));
    png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/secondary_expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledSecondary.tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
                CHECK(
                    compiledSecondary.tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
                    expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]
                );
            }
        }
    }

    // Check that paletteIndexesOfTile are correct
    CHECK(compiledSecondary.paletteIndexesOfTile[0] == 2);
    CHECK(compiledSecondary.paletteIndexesOfTile[1] == 3);
    CHECK(compiledSecondary.paletteIndexesOfTile[2] == 3);
    CHECK(compiledSecondary.paletteIndexesOfTile[3] == 3);
    CHECK(compiledSecondary.paletteIndexesOfTile[4] == 3);
    CHECK(compiledSecondary.paletteIndexesOfTile[5] == 5);

    // Check that compiled palettes are as expected
    CHECK(compiledSecondary.palettes.at(0).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
    CHECK(compiledSecondary.palettes.at(1).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledSecondary.palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledSecondary.palettes.at(2).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(2).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledSecondary.palettes.at(2).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
    CHECK(compiledSecondary.palettes.at(3).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(3).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledSecondary.palettes.at(3).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(compiledSecondary.palettes.at(3).colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_PURPLE));
    CHECK(compiledSecondary.palettes.at(3).colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_LIME));
    CHECK(compiledSecondary.palettes.at(4).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(5).colors[0] == porytiles::rgbaToBgr(config.transparencyColor));
    CHECK(compiledSecondary.palettes.at(5).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));

    // Check that all assignments are correct
    CHECK(compiledSecondary.assignments.size() == porytiles::METATILES_IN_ROW * config.numTilesPerMetatile);

    CHECK_FALSE(compiledSecondary.assignments[0].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[0].vFlip);
    CHECK(compiledSecondary.assignments[0].tileIndex == 0);
    CHECK(compiledSecondary.assignments[0].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary.assignments[1].hFlip);
    CHECK(compiledSecondary.assignments[1].vFlip);
    CHECK(compiledSecondary.assignments[1].tileIndex == 0 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[1].paletteIndex == 2);

    CHECK_FALSE(compiledSecondary.assignments[2].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[2].vFlip);
    CHECK(compiledSecondary.assignments[2].tileIndex == 1 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[2].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary.assignments[3].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[3].vFlip);
    CHECK(compiledSecondary.assignments[3].tileIndex == 0);
    CHECK(compiledSecondary.assignments[3].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary.assignments[4].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[4].vFlip);
    CHECK(compiledSecondary.assignments[4].tileIndex == 0);
    CHECK(compiledSecondary.assignments[4].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary.assignments[5].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[5].vFlip);
    CHECK(compiledSecondary.assignments[5].tileIndex == 2 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[5].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary.assignments[6].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[6].vFlip);
    CHECK(compiledSecondary.assignments[6].tileIndex == 3 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[6].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary.assignments[7].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[7].vFlip);
    CHECK(compiledSecondary.assignments[7].tileIndex == 0);
    CHECK(compiledSecondary.assignments[7].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary.assignments[8].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[8].vFlip);
    CHECK(compiledSecondary.assignments[8].tileIndex == 4 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[8].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary.assignments[9].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[9].vFlip);
    CHECK(compiledSecondary.assignments[9].tileIndex == 0);
    CHECK(compiledSecondary.assignments[9].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary.assignments[10].hFlip);
    CHECK_FALSE(compiledSecondary.assignments[10].vFlip);
    CHECK(compiledSecondary.assignments[10].tileIndex == 0);
    CHECK(compiledSecondary.assignments[10].paletteIndex == 0);

    CHECK(compiledSecondary.assignments[11].hFlip);
    CHECK(compiledSecondary.assignments[11].vFlip);
    CHECK(compiledSecondary.assignments[11].tileIndex == 5 + config.numTilesInPrimary);
    CHECK(compiledSecondary.assignments[11].paletteIndex == 5);

    for (std::size_t index = config.numTilesPerMetatile; index < porytiles::METATILES_IN_ROW * config.numTilesPerMetatile; index++) {
        CHECK_FALSE(compiledSecondary.assignments[index].hFlip);
        CHECK_FALSE(compiledSecondary.assignments[index].vFlip);
        CHECK(compiledSecondary.assignments[index].tileIndex == 0);
        CHECK(compiledSecondary.assignments[index].paletteIndex == 0);
    }

    // Check that colorIndexMap is correct
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 0);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_YELLOW)] == 1);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 2);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 3);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_WHITE)] == 4);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 5);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_PURPLE)] == 6);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_LIME)] == 7);
    CHECK(compiledSecondary.colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREY)] == 8);

    // Check that tileIndexes is correct
    CHECK(compiledSecondary.tileIndexes.size() == compiledSecondary.tiles.size());
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[0]] == 0);
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[1]] == 1);
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[2]] == 2);
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[3]] == 3);
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[4]] == 4);
    CHECK(compiledSecondary.tileIndexes[compiledSecondary.tiles[5]] == 5);
}
