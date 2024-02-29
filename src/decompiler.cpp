#include "decompiler.h"

#include <cstdint>
#include <doctest.h>
#include <filesystem>
#include <memory>

#include "compiler.h"
#include "importer.h"
#include "porytiles_context.h"
#include "types.h"

namespace porytiles {

static RGBATile setTilePixels(const GBATile &gbaTile, const GBAPalette &palette, bool hFlip, bool vFlip)
{
  RGBATile rgbTile{};
  for (std::size_t row = 0; row < TILE_SIDE_LENGTH; row++) {
    for (std::size_t col = 0; col < TILE_SIDE_LENGTH; col++) {
      std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH - 1 - row : row;
      std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH - 1 - col : col;
      RGBA32 rgb = bgrToRgba(palette.colors.at(gbaTile.getPixel(rowWithFlip, colWithFlip)));
      rgbTile.setPixel(row, col, rgb);
    }
  }
  return rgbTile;
}

static RGBATile createDecompiledTile(PorytilesContext &ctx, const std::vector<GBATile> &tiles, std::size_t tileIndex,
                              const std::vector<GBAPalette> &palettes, std::size_t paletteIndex, const Attributes &attributes, bool hFlip, bool vFlip)
{
  RGBATile decompiledTile{};
  const GBATile &gbaTile = tiles.at(tileIndex);
  decompiledTile = setTilePixels(gbaTile, palettes.at(paletteIndex), hFlip, vFlip);
  decompiledTile.attributes = attributes;
  return decompiledTile;
}

std::unique_ptr<DecompiledTileset> decompile2(PorytilesContext &ctx, const CompiledTileset &compiledTileset)
{
  auto decompiled = std::make_unique<DecompiledTileset>();

  for (const auto &assignment : compiledTileset.assignments) {
  }

  return decompiled;
}

std::unique_ptr<DecompiledTileset> decompile(PorytilesContext &ctx, const CompiledTileset &compiledTileset)
{
  auto decompiled = std::make_unique<DecompiledTileset>();

  for (const auto &assignment : compiledTileset.assignments) {
    const auto &srcTiles = ctx.decompilerConfig.mode == DecompilerMode::SECONDARY &&
                                   assignment.tileIndex < ctx.fieldmapConfig.numTilesInPrimary
                               ? ctx.decompilerContext.pairedPrimaryTileset->tiles
                               : compiledTileset.tiles;
    auto tileIndex = ctx.decompilerConfig.mode == DecompilerMode::SECONDARY &&
                             assignment.tileIndex >= ctx.fieldmapConfig.numTilesInPrimary
                         ? assignment.tileIndex - ctx.fieldmapConfig.numTilesInPrimary
                         : assignment.tileIndex;
    if (tileIndex >= srcTiles.size() || assignment.paletteIndex >= ctx.fieldmapConfig.numPalettesTotal) {
      /*
       * This weird edge case can happen because some of the vanilla game metatiles.bin entries have garbage values
       * for the tile and palette indexes. See Petalburg tileset, metatile 0x24A for an example. In-game the garbage
       * tiles are invisible since they are covered by another layer.
       */
      // TODO 1.0.0 : we should create a custom warning here so users know what happened
      if (ctx.decompilerConfig.mode == DecompilerMode::SECONDARY) {
        const GBATile &gbaTile = ctx.decompilerContext.pairedPrimaryTileset->tiles.at(0);
        // TODO 1.0.0 : handle correctly grabbing from primary palettes
        RGBATile rgbTile = setTilePixels(gbaTile, compiledTileset.palettes.at(0), assignment.hFlip, assignment.vFlip);
        rgbTile.attributes = assignment.attributes;
        decompiled->tiles.push_back(rgbTile);
      }
      else {
        const GBATile &gbaTile = compiledTileset.tiles.at(0);
        RGBATile rgbTile = setTilePixels(gbaTile, compiledTileset.palettes.at(0), assignment.hFlip, assignment.vFlip);
        rgbTile.attributes = assignment.attributes;
        decompiled->tiles.push_back(rgbTile);
      }
    }
    else {
      const GBATile &gbaTile = srcTiles.at(tileIndex);
      // TODO 1.0.0 : handle correctly grabbing from primary palettes if this a secondary tileset
      RGBATile rgbTile =
          setTilePixels(gbaTile, compiledTileset.palettes.at(assignment.paletteIndex), assignment.hFlip, assignment.vFlip);
      rgbTile.attributes = assignment.attributes;
      decompiled->tiles.push_back(rgbTile);
    }
  }

  // TODO : fill in animations

  return decompiled;
}

} // namespace porytiles

TEST_CASE("decompile should decompile a basic primary tileset")
{
  porytiles::PorytilesContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 6;
  ctx.fieldmapConfig.numPalettesTotal = 13;
  ctx.fieldmapConfig.numTilesInPrimary = 512;
  ctx.fieldmapConfig.numTilesTotal = 1024;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.decompilerConfig.mode = porytiles::DecompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/top.png"}));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_2/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_2/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_2/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary, middlePrimary, topPrimary);
  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary, std::vector<porytiles::RGBATile>{});

  auto decompiledViaAlgorithm = porytiles::decompile(ctx, *compiledPrimary);

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

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/primary/top.png"}));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_2/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_2/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_2/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary, middlePrimary, topPrimary);
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary, std::vector<porytiles::RGBATile>{});
  ctx.compilerContext.pairedPrimaryTileset = std::move(compiledPrimary);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_2/secondary/top.png"}));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/simple_metatiles_2/secondary/bottom.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/simple_metatiles_2/secondary/middle.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/simple_metatiles_2/secondary/top.png"};
  porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomSecondary, middleSecondary, topSecondary);
  ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
  auto compiledSecondary = porytiles::compile(ctx, decompiledSecondary, std::vector<porytiles::RGBATile>{});

  ctx.decompilerConfig.mode = porytiles::DecompilerMode::SECONDARY;
  ctx.decompilerContext.pairedPrimaryTileset = std::move(ctx.compilerContext.pairedPrimaryTileset);
  auto decompiledViaAlgorithm = porytiles::decompile(ctx, *compiledSecondary);
}
