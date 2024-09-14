#include "decompiler.h"

#include <cstdint>
#include <doctest.h>
#include <filesystem>
#include <functional>
#include <memory>

#include "compiler.h"
#include "importer.h"
#include "logger.h"
#include "porytiles_context.h"
#include "types.h"

namespace porytiles {

static RGBATile setTilePixels(const PorytilesContext &ctx, const GBATile &gbaTile, const GBAPalette &palette,
                              bool hFlip, bool vFlip)
{
  RGBATile rgbTile{};
  for (std::size_t row = 0; row < TILE_SIDE_LENGTH_PIX; row++) {
    for (std::size_t col = 0; col < TILE_SIDE_LENGTH_PIX; col++) {
      std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH_PIX - 1 - row : row;
      std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH_PIX - 1 - col : col;
      std::uint8_t pixel = gbaTile.getPixel(rowWithFlip, colWithFlip);
      RGBA32 rgb{};
      if (pixel == 0 && ctx.decompilerConfig.normalizeTransparency) {
        rgb = ctx.decompilerConfig.normalizeTransparencyColor;
      }
      else {
        rgb = bgrToRgba(palette.colors.at(pixel));
      }
      rgbTile.setPixel(row, col, rgb);
    }
  }
  return rgbTile;
}

static void setDecompTileFields(PorytilesContext &ctx, DecompilerMode mode, RGBATile &decompiledTile,
                                const std::vector<GBATile> &tiles, std::size_t tileIndex,
                                const std::vector<GBAPalette> &palettes, std::size_t paletteIndex,
                                const Attributes &attributes, bool hFlip, bool vFlip)
{
  if (tileIndex >= tiles.size() || paletteIndex >= ctx.fieldmapConfig.numPalettesTotal) {
    /*
     * This weird edge case can happen because some of the vanilla game metatiles.bin entries have garbage values
     * for the tile and palette indexes. See Petalburg tileset, metatile 0x24A for an example. In-game the garbage
     * tiles are invisible since they are covered by another layer.
     */
    if (tileIndex >= tiles.size()) {
      warn_tileIndexOutOfRange(ctx.err, mode, tileIndex, tiles.size(), decompiledTile);
    }
    if (paletteIndex >= ctx.fieldmapConfig.numPalettesTotal) {
      warn_paletteIndexOutOfRange(ctx.err, mode, paletteIndex, ctx.fieldmapConfig.numPalettesTotal, decompiledTile);
    }
    const GBATile &gbaTile = std::invoke([&]() -> const GBATile & {
      // tileIndex was invalid, so just grab the very first tile of the primary set (which is transparent)
      if (mode == DecompilerMode::SECONDARY) {
        return ctx.decompilerContext.pairedPrimaryTileset->tiles.at(0);
      }
      return tiles.at(0);
    });
    // Use tile 0, palette 0
    decompiledTile = setTilePixels(ctx, gbaTile, palettes.at(0), hFlip, vFlip);
    decompiledTile.attributes = attributes;
    return;
  }

  // Regular case, set the tile pixels and attributes and return it
  const GBATile &gbaTile = tiles.at(tileIndex);
  decompiledTile = setTilePixels(ctx, gbaTile, palettes.at(paletteIndex), hFlip, vFlip);
  decompiledTile.attributes = attributes;
}

std::unique_ptr<DecompiledTileset> decompile(PorytilesContext &ctx, DecompilerMode mode,
                                             const CompiledTileset &compiledTileset,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap)
{
  auto decompiledTileset = std::make_unique<DecompiledTileset>();

  /*
   * Infer layer type by dividing the compiled metatile entries size by 8 and 12, and comparing each result to the
   * attribute count (i.e. the true metatile count). If division by 8 matches, then we are dual layer. If 12 matches, we
   * are triple. Otherwise, we have corruption and should fail.
   */
  auto dualImpliedMetatileCount = compiledTileset.metatileEntries.size() / TILES_PER_METATILE_DUAL;
  auto tripleImpliedMetatileCount = compiledTileset.metatileEntries.size() / TILES_PER_METATILE_TRIPLE;

  if (dualImpliedMetatileCount == attributesMap.size()) {
    decompiledTileset->tripleLayer = false;
  }
  else if (tripleImpliedMetatileCount == attributesMap.size()) {
    decompiledTileset->tripleLayer = true;
  }
  else {
    fatalerror_noImpliedLayerType(ctx.err, ctx.decompilerSrcPaths, mode);
  }

  std::size_t metatileIndex = 0;
  for (std::size_t entryIndex = 0; const auto &metatileEntry : compiledTileset.metatileEntries) {
    // Set decomp tile metadata
    RGBATile decompiledTile{};
    decompiledTile.type = TileType::LAYERED;
    decompiledTile.metatileIndex = metatileIndex;
    std::size_t tileIndexWithinMetatile =
        entryIndex % (decompiledTileset->tripleLayer ? TILES_PER_METATILE_TRIPLE : TILES_PER_METATILE_DUAL);
    decompiledTile.subtile = indexToSubtile(tileIndexWithinMetatile);
    decompiledTile.layer = indexToLayer(tileIndexWithinMetatile, decompiledTileset->tripleLayer);

    // In secondary mode, we need to determine which compiled tileset this tile comes from: paired primary or secondary
    if (mode == DecompilerMode::SECONDARY) {
      if (metatileEntry.tileIndex < ctx.fieldmapConfig.numTilesInPrimary) {
        setDecompTileFields(ctx, mode, decompiledTile, ctx.decompilerContext.pairedPrimaryTileset->tiles,
                            metatileEntry.tileIndex, ctx.decompilerContext.pairedPrimaryTileset->palettes,
                            metatileEntry.paletteIndex, metatileEntry.attributes, metatileEntry.hFlip,
                            metatileEntry.vFlip);
      }
      else {
        setDecompTileFields(ctx, mode, decompiledTile, compiledTileset.tiles,
                            metatileEntry.tileIndex - ctx.fieldmapConfig.numTilesInPrimary, compiledTileset.palettes,
                            metatileEntry.paletteIndex, metatileEntry.attributes, metatileEntry.hFlip,
                            metatileEntry.vFlip);
      }
    }
    // For primary mode, we can just use the straight parameters
    else {
      setDecompTileFields(ctx, mode, decompiledTile, compiledTileset.tiles, metatileEntry.tileIndex,
                          compiledTileset.palettes, metatileEntry.paletteIndex, metatileEntry.attributes,
                          metatileEntry.hFlip, metatileEntry.vFlip);
    }
    decompiledTileset->tiles.push_back(decompiledTile);

    // Update tile tracking
    entryIndex++;
    metatileIndex = entryIndex / (decompiledTileset->tripleLayer ? TILES_PER_METATILE_TRIPLE : TILES_PER_METATILE_DUAL);
  }

  // TODO : fill in animations

  return decompiledTileset;
}

} // namespace porytiles

TEST_CASE("decompile should decompile a basic primary tileset")
{
  porytiles::PorytilesContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 6;
  ctx.fieldmapConfig.numPalettesTotal = 13;
  ctx.fieldmapConfig.numTilesInPrimary = 512;
  ctx.fieldmapConfig.numTilesTotal = 1024;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.tripleLayer = true;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/top.png"}));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_2/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_2/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_2/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
      ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
      middlePrimary, topPrimary);
  auto compiledPrimary =
      porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, std::vector<porytiles::RGBATile>{});

  auto decompiledViaAlgorithm =
      porytiles::decompile(ctx, porytiles::DecompilerMode::PRIMARY, *compiledPrimary,
                           compiledPrimary->generateAttributesMap(ctx.compilerConfig.tripleLayer));

  CHECK(decompiledViaAlgorithm->tiles.size() == decompiledPrimary.tiles.size());
  for (std::size_t i = 0; i < decompiledViaAlgorithm->tiles.size(); i++) {
    CHECK(decompiledViaAlgorithm->tiles.at(i).equalsAfterBgrConversion(decompiledPrimary.tiles.at(i)));
  }
}

TEST_CASE("decompile should decompile a basic secondary tileset")
{
  porytiles::PorytilesContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 6;
  ctx.fieldmapConfig.numPalettesTotal = 13;
  ctx.fieldmapConfig.numTilesInPrimary = 512;
  ctx.fieldmapConfig.numTilesTotal = 1024;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.tripleLayer = true;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/top.png"}));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_2/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_2/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_2/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
      ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
      middlePrimary, topPrimary);
  auto compiledPrimary =
      porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, std::vector<porytiles::RGBATile>{});
  ctx.compilerContext.pairedPrimaryTileset = std::move(compiledPrimary);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/top.png"}));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/simple_metatiles_2/secondary/bottom.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/simple_metatiles_2/secondary/middle.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/simple_metatiles_2/secondary/top.png"};
  porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(
      ctx, porytiles::CompilerMode::SECONDARY, std::unordered_map<std::size_t, porytiles::Attributes>{},
      bottomSecondary, middleSecondary, topSecondary);
  auto compiledSecondary = porytiles::compile(ctx, porytiles::CompilerMode::SECONDARY, decompiledSecondary,
                                              std::vector<porytiles::RGBATile>{});

  ctx.decompilerContext.pairedPrimaryTileset = std::move(ctx.compilerContext.pairedPrimaryTileset);
  auto decompiledViaAlgorithm =
      porytiles::decompile(ctx, porytiles::DecompilerMode::SECONDARY, *compiledSecondary,
                           compiledSecondary->generateAttributesMap(ctx.compilerConfig.tripleLayer));

  CHECK(decompiledViaAlgorithm->tiles.size() == decompiledSecondary.tiles.size());
  for (std::size_t i = 0; i < decompiledViaAlgorithm->tiles.size(); i++) {
    CHECK(decompiledViaAlgorithm->tiles.at(i).equalsAfterBgrConversion(decompiledSecondary.tiles.at(i)));
  }
}
