#include "decompiler.h"

#include <cstdint>
#include <memory>

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
    decompiled->tiles.push_back(rgbTile);
  }

  return decompiled;
}

} // namespace porytiles
