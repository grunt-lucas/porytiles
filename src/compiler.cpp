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

    if (colorIndex > (PAL_SIZE - 1) * config.numPalettesInPrimary) {
        // TODO : better error context
        throw PtException{"too many unique colors"};
    }

    return {colorIndexes, indexesToColors};
}

static std::pair<std::vector<IndexedNormTileWithColorSet>, std::vector<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t>& colorIndexMap,
                             const std::vector<IndexedNormTile>& indexedNormalizedTiles) {
    std::vector<IndexedNormTileWithColorSet> indexedNormTilesWithColorSets;
    std::unordered_set<ColorSet> uniqueColorSets;
    std::vector<ColorSet> colorSets;
    for (const auto& [index, normalizedTile]: indexedNormalizedTiles) {
        // Compute the ColorSet for this normalized tile, then add it to our indexes
        auto colorSet = toColorSet(colorIndexMap, normalizedTile.palette);
        indexedNormTilesWithColorSets.emplace_back(index, normalizedTile, colorSet);
        if (!uniqueColorSets.contains(colorSet)) {
            colorSets.push_back(colorSet);
            uniqueColorSets.insert(colorSet);
        }
    }
    return std::pair{indexedNormTilesWithColorSets, colorSets};
}

CompiledTileset compile(const Config& config, const DecompiledTileset& decompiledTileset) {
    CompiledTileset compiled;
    compiled.palettes.resize(config.numPalettesInPrimary);
    // TODO : we should compute the number of metatiles here and throw if the user tilesheet exceeds the max
    compiled.assignments.resize(decompiledTileset.tiles.size());

    // Build helper data structures for the assignments
    std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(config, decompiledTileset);
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(config, indexedNormTiles);
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
    bool assignSuccessful = assign(config, state, assignedPalsSolution, std::vector<ColorSet>{});

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
}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("buildColorIndexMap should build a map of all unique colors in the decomp tileset") {
    porytiles::Config config = porytiles::defaultConfig();

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
    std::vector<IndexedNormTile> normalizedTiles = normalizeDecompTiles(config, tiles);

    auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(config, normalizedTiles);

    CHECK(colorToIndex.size() == 4);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);
}

TEST_CASE("matchNormalizedWithColorSets should return the expected data structures") {
    porytiles::Config config = porytiles::defaultConfig();

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
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
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[0])) != colorSets.end());

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
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[1])) != colorSets.end());

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
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[2])) != colorSets.end());

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
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[3])) != colorSets.end());
}

TEST_CASE("assign should correctly assign all normalized palettes or fail if impossible") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;

    SUBCASE("It should successfully allocate a simple 2x2 tileset png") {
        constexpr int SOLUTION_SIZE = 2;
        config.numPalettesInPrimary = SOLUTION_SIZE;
        config.secondary = false;
        config.maxRecurseCount = 20;

        REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
        png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
        porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
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
        std::stable_sort(std::begin(unassigned), std::end(unassigned),
                  [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
        porytiles::AssignState state = {hardwarePalettes, unassigned};

        porytiles::gRecurseCount = 0;
        CHECK(porytiles::assign(config, state, solution, std::vector<ColorSet>{}));
        CHECK(solution.size() == SOLUTION_SIZE);
        CHECK(solution.at(0).count() == 1);
        CHECK(solution.at(1).count() == 3);
        CHECK(solution.at(0).test(0));
        CHECK(solution.at(1).test(1));
        CHECK(solution.at(1).test(2));
        CHECK(solution.at(1).test(3));
    }

    SUBCASE("It should successfully allocate a large, complex PNG") {
        constexpr int SOLUTION_SIZE = 5;
        config.numPalettesInPrimary = SOLUTION_SIZE;
        config.secondary = false;
        config.maxRecurseCount = 200;

        REQUIRE(std::filesystem::exists("res/tests/compile_raw_set_1/set.png"));
        png::image<png::rgba_pixel> png1{"res/tests/compile_raw_set_1/set.png"};
        porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
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
        std::stable_sort(std::begin(unassigned), std::end(unassigned),
                  [](const auto& cs1, const auto& cs2) { return cs1.count() < cs2.count(); });
        porytiles::AssignState state = {hardwarePalettes, unassigned};

        porytiles::gRecurseCount = 0;
        CHECK(porytiles::assign(config, state, solution, std::vector<ColorSet>{}));
        CHECK(solution.size() == SOLUTION_SIZE);
        CHECK(solution.at(0).count() == 11);
        CHECK(solution.at(1).count() == 12);
        CHECK(solution.at(2).count() == 14);
        CHECK(solution.at(3).count() == 14);
        CHECK(solution.at(4).count() == 15);
    }
}

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
