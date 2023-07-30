#include "compiler.h"

#include <algorithm>
#include <png.hpp>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "compiler_context.h"
#include "compiler_helpers.h"
#include "config.h"
#include "doctest.h"
#include "errors.h"
#include "importer.h"
#include "logger.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {
CompiledTileset compile(const CompilerContext &context, const DecompiledTileset &decompiledTileset)
{
  if (context.mode == SECONDARY && (context.config.numPalettesInPrimary != context.primaryTileset->palettes.size())) {
    fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(context.config.numPalettesInPrimary,
                                                     context.primaryTileset->palettes.size());
  }

  CompiledTileset compiled;
  if (context.mode == PRIMARY) {
    compiled.palettes.resize(context.config.numPalettesInPrimary);
    std::size_t inputMetatileCount = (decompiledTileset.tiles.size() / context.config.numTilesPerMetatile);
    if (inputMetatileCount > context.config.numMetatilesInPrimary) {
      throw PtException{"input metatile count (" + std::to_string(inputMetatileCount) +
                        ") exceeded primary metatile limit (" + std::to_string(context.config.numMetatilesInPrimary) +
                        ")"};
    }
  }
  else if (context.mode == SECONDARY) {
    compiled.palettes.resize(context.config.numPalettesTotal);
    std::size_t inputMetatileCount = (decompiledTileset.tiles.size() / context.config.numTilesPerMetatile);
    if (inputMetatileCount > context.config.numMetatilesInSecondary()) {
      throw PtException{"input metatile count (" + std::to_string(inputMetatileCount) +
                        ") exceeded secondary metatile limit (" +
                        std::to_string(context.config.numMetatilesInSecondary()) + ")"};
    }
  }
  else if (context.mode == RAW) {
    throw std::runtime_error{"TODO : support RAW mode"};
  }
  else {
    fatal_unknownCompilerMode(context.mode);
  }
  compiled.assignments.resize(decompiledTileset.tiles.size());

  // Build helper data structures for the assignments
  std::unordered_map<BGR15, std::size_t> emptyPrimaryColorIndexMap;
  const std::unordered_map<BGR15, std::size_t> *primaryColorIndexMap = &emptyPrimaryColorIndexMap;
  if (context.mode == SECONDARY) {
    primaryColorIndexMap = &(context.primaryTileset->colorIndexMap);
  }
  std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(context.config, decompiledTileset);
  auto [colorToIndex, indexToColor] = buildColorIndexMaps(context.config, indexedNormTiles, *primaryColorIndexMap);
  compiled.colorIndexMap = colorToIndex;
  auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

  /*
   * Run palette assignment:
   * `assignedPalsSolution' is an out param that the assign function will populate when it finds a solution
   */
  std::vector<ColorSet> assignedPalsSolution;
  std::vector<ColorSet> tmpHardwarePalettes;
  if (context.mode == PRIMARY) {
    assignedPalsSolution.reserve(context.config.numPalettesInPrimary);
    tmpHardwarePalettes.resize(context.config.numPalettesInPrimary);
  }
  else if (context.mode == SECONDARY) {
    assignedPalsSolution.reserve(context.config.numPalettesInSecondary());
    tmpHardwarePalettes.resize(context.config.numPalettesInSecondary());
  }
  else if (context.mode == RAW) {
    throw std::runtime_error{"TODO : support RAW mode"};
  }
  else {
    fatal_unknownCompilerMode(context.mode);
  }
  std::vector<ColorSet> unassignedNormPalettes;
  std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
  std::stable_sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
                   [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
  std::vector<ColorSet> primaryPaletteColorSets{};
  if (context.mode == SECONDARY) {
    /*
     * Construct ColorSets for the primary palettes, assign can use these to decide if a tile is entirely covered by a
     * primary palette and hence does not need to extend the search by assigning its colors to one of the new secondary
     * palettes.
     */
    primaryPaletteColorSets.reserve(context.primaryTileset->palettes.size());
    for (std::size_t i = 0; i < context.primaryTileset->palettes.size(); i++) {
      const auto &gbaPalette = context.primaryTileset->palettes.at(i);
      primaryPaletteColorSets.emplace_back();
      for (std::size_t j = 1; j < gbaPalette.size; j++) {
        primaryPaletteColorSets.at(i).set(colorToIndex.at(gbaPalette.colors.at(j)));
      }
    }
  }

  AssignState state = {tmpHardwarePalettes, unassignedNormPalettes};
  gRecurseCount = 0;
  bool assignSuccessful = assign(context.config, state, assignedPalsSolution, primaryPaletteColorSets);
  if (!assignSuccessful) {
    // TODO : better error context
    throw PtException{"failed to allocate palettes"};
  }

  /*
   * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile sharing)
   * and so we may need to do something fancier here so that the colors align correctly.
   */
  if (context.mode == PRIMARY) {
    for (std::size_t i = 0; i < context.config.numPalettesInPrimary; i++) {
      ColorSet palAssignments = assignedPalsSolution.at(i);
      compiled.palettes.at(i).colors.at(0) = rgbaToBgr(context.config.transparencyColor);
      std::size_t colorIndex = 1;
      for (std::size_t j = 0; j < palAssignments.size(); j++) {
        if (palAssignments.test(j)) {
          compiled.palettes.at(i).colors.at(colorIndex) = indexToColor.at(j);
          colorIndex++;
        }
      }
      compiled.palettes.at(i).size = colorIndex;
    }
  }
  else if (context.mode == SECONDARY) {
    for (std::size_t i = 0; i < context.config.numPalettesInPrimary; i++) {
      // Copy the primary set's palettes into this tileset so tiles can use them
      for (std::size_t j = 0; j < PAL_SIZE; j++) {
        compiled.palettes.at(i).colors.at(j) = context.primaryTileset->palettes.at(i).colors.at(j);
      }
    }
    for (std::size_t i = context.config.numPalettesInPrimary; i < context.config.numPalettesTotal; i++) {
      ColorSet palAssignments = assignedPalsSolution.at(i - context.config.numPalettesInPrimary);
      compiled.palettes.at(i).colors.at(0) = rgbaToBgr(context.config.transparencyColor);
      std::size_t colorIndex = 1;
      for (std::size_t j = 0; j < palAssignments.size(); j++) {
        if (palAssignments.test(j)) {
          compiled.palettes.at(i).colors.at(colorIndex) = indexToColor.at(j);
          colorIndex++;
        }
      }
      compiled.palettes.at(i).size = colorIndex;
    }
  }
  else if (context.mode == RAW) {
    throw std::runtime_error{"TODO : support RAW mode"};
  }
  else {
    fatal_unknownCompilerMode(context.mode);
  }

  /*
   * Build the tile assignments.
   */
  if (context.mode == PRIMARY) {
    assignTilesPrimary(context, compiled, indexedNormTilesWithColorSets, assignedPalsSolution);
  }
  else if (context.mode == SECONDARY) {
    assignTilesSecondary(context, compiled, indexedNormTilesWithColorSets, primaryPaletteColorSets,
                         assignedPalsSolution);
  }
  else if (context.mode == RAW) {
    throw std::runtime_error{"TODO : support RAW mode"};
  }
  else {
    fatal_unknownCompilerMode(context.mode);
  }

  return compiled;
}
} // namespace porytiles

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("compile simple example should perform as expected")
{
  porytiles::Config config = porytiles::defaultConfig();
  config.transparencyColor = porytiles::RGBA_MAGENTA;
  config.numPalettesInPrimary = 2;
  config.numTilesInPrimary = 4;
  config.maxRecurseCount = 5;
  config.verbose = true;
  porytiles::CompilerContext context{config, porytiles::CompilerMode::PRIMARY};

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
  porytiles::CompiledTileset compiledTiles = porytiles::compile(context, tiles);

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
  porytiles::GBATile &tile0 = compiledTiles.tiles[0];
  for (size_t i = 0; i < 64; i++) {
    CHECK(tile0.colorIndexes[i] == 0);
  }

  porytiles::GBATile &tile1 = compiledTiles.tiles[1];
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

TEST_CASE("compile function should fill out CompiledTileset struct with expected values")
{
  porytiles::Config config = porytiles::defaultConfig();
  config.numPalettesInPrimary = 3;
  config.numPalettesTotal = 6;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_primary.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/bottom_primary.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/middle_primary.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/top_primary.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(bottomPrimary, middlePrimary, topPrimary);

  porytiles::CompilerContext context{config, porytiles::CompilerMode::PRIMARY};
  porytiles::CompiledTileset compiledPrimary = porytiles::compile(context, decompiledPrimary);

  // Check that tiles are as expected
  CHECK(compiledPrimary.tiles.size() == 5);
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary_expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/primary_expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledPrimary.tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledPrimary.tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
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

  for (std::size_t index = config.numTilesPerMetatile; index < porytiles::METATILES_IN_ROW * config.numTilesPerMetatile;
       index++) {
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

TEST_CASE("compileSecondary function should fill out CompiledTileset struct with expected values")
{
  porytiles::Config config = porytiles::defaultConfig();
  config.numPalettesInPrimary = 3;
  config.numPalettesTotal = 6;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_primary.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/bottom_primary.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/middle_primary.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/top_primary.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(bottomPrimary, middlePrimary, topPrimary);

  porytiles::CompilerContext primaryContext{config, porytiles::CompilerMode::PRIMARY};
  porytiles::CompiledTileset compiledPrimary = porytiles::compile(primaryContext, decompiledPrimary);

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/bottom_secondary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/middle_secondary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/top_secondary.png"));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/simple_metatiles_3/bottom_secondary.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/simple_metatiles_3/middle_secondary.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/simple_metatiles_3/top_secondary.png"};
  porytiles::DecompiledTileset decompiledSecondary =
      porytiles::importLayeredTilesFromPngs(bottomSecondary, middleSecondary, topSecondary);
  porytiles::CompilerContext secondaryContext{config, porytiles::CompilerMode::SECONDARY};
  secondaryContext.primaryTileset = &compiledPrimary;
  porytiles::CompiledTileset compiledSecondary = porytiles::compile(secondaryContext, decompiledSecondary);

  // Check that tiles are as expected
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary_expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/secondary_expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledSecondary.tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledSecondary.tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
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

  for (std::size_t index = config.numTilesPerMetatile; index < porytiles::METATILES_IN_ROW * config.numTilesPerMetatile;
       index++) {
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
