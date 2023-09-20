#include "decompiler.h"

#include <cstdint>
#include <doctest.h>
#include <filesystem>
#include <memory>

#include "compiler.h"
#include "importer.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

static RGBATile makeTile(const GBATile &gbaTile, const GBAPalette &palette, bool hFlip, bool vFlip)
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

std::unique_ptr<DecompiledTileset> decompile(PtContext &ctx, const CompiledTileset &compiledTileset)
{
  auto decompiled = std::make_unique<DecompiledTileset>();

  for (const auto &assignment : compiledTileset.assignments) {
    const GBATile &gbaTile = compiledTileset.tiles.at(assignment.tileIndex);
    RGBATile rgbTile =
        makeTile(gbaTile, compiledTileset.palettes.at(assignment.paletteIndex), assignment.hFlip, assignment.vFlip);
    rgbTile.attributes = assignment.attributes;
    decompiled->tiles.push_back(rgbTile);
  }

  // TODO : fill in animations

  return decompiled;
}

} // namespace porytiles

TEST_CASE("decompile should decompile a basic tileset")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 6;
  ctx.fieldmapConfig.numPalettesTotal = 13;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.compilerConfig.assignAlgorithm = porytiles::AssignAlgorithm::DEPTH_FIRST;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_2/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_2/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_2/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary, middlePrimary, topPrimary);
  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary);

  auto decompiledViaAlgorithm = porytiles::decompile(ctx, *compiledPrimary);

  CHECK(decompiledViaAlgorithm->tiles.size() == decompiledPrimary.tiles.size());
  for (std::size_t i = 0; i < decompiledViaAlgorithm->tiles.size(); i++) {
    CHECK(decompiledViaAlgorithm->tiles.at(i).equalsAfterBgrConversion(decompiledPrimary.tiles.at(i)));
  }
}