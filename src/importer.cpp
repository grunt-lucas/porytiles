#include "importer.h"

#include <iostream>
#include <png.hpp>

#include "errors_warnings.h"
#include "logger.h"
#include "ptcontext.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {

DecompiledTileset importTilesFromPng(PtContext &ctx, const png::image<png::rgba_pixel> &png)
{
  if (png.get_height() % TILE_SIDE_LENGTH != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.inputPaths, "height", png.get_height());
  }
  if (png.get_width() % TILE_SIDE_LENGTH != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.inputPaths, "width", png.get_width());
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "freestanding input dimension not divisible by 8");
  }

  DecompiledTileset decompiledTiles;

  std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
  std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

  for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
    size_t tileRow = tileIndex / pngWidthInTiles;
    size_t tileCol = tileIndex % pngWidthInTiles;
    RGBATile tile{};
    tile.type = TileType::FREESTANDING;
    tile.tileIndex = tileIndex;
    for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
      size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
      size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
      tile.pixels[pixelIndex].red = png[pixelRow][pixelCol].red;
      tile.pixels[pixelIndex].green = png[pixelRow][pixelCol].green;
      tile.pixels[pixelIndex].blue = png[pixelRow][pixelCol].blue;
      tile.pixels[pixelIndex].alpha = png[pixelRow][pixelCol].alpha;
    }
    decompiledTiles.tiles.push_back(tile);
  }
  return decompiledTiles;
}

DecompiledTileset importLayeredTilesFromPngs(PtContext &ctx, const png::image<png::rgba_pixel> &bottom,
                                             const png::image<png::rgba_pixel> &middle,
                                             const png::image<png::rgba_pixel> &top)
{
  if (bottom.get_height() % METATILE_SIDE_LENGTH != 0) {
    error_layerHeightNotDivisibleBy16(ctx.err, TileLayer::BOTTOM, bottom.get_height());
  }
  if (middle.get_height() % METATILE_SIDE_LENGTH != 0) {
    error_layerHeightNotDivisibleBy16(ctx.err, TileLayer::MIDDLE, middle.get_height());
  }
  if (top.get_height() % METATILE_SIDE_LENGTH != 0) {
    error_layerHeightNotDivisibleBy16(ctx.err, TileLayer::TOP, top.get_height());
  }

  if (bottom.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
    error_layerWidthNeq128(ctx.err, TileLayer::BOTTOM, bottom.get_width());
  }
  if (middle.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
    error_layerWidthNeq128(ctx.err, TileLayer::MIDDLE, middle.get_width());
  }
  if (top.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
    error_layerWidthNeq128(ctx.err, TileLayer::TOP, top.get_width());
  }
  if ((bottom.get_height() != middle.get_height()) || (bottom.get_height() != top.get_height())) {
    error_layerHeightsMustEq(ctx.err, bottom.get_height(), middle.get_height(), top.get_height());
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "input layer png dimensions invalid");
  }

  DecompiledTileset decompiledTiles;

  // Since all widths and heights are the same, we can just read the bottom layer's width and height
  std::size_t widthInMetatiles = bottom.get_width() / METATILE_SIDE_LENGTH;
  std::size_t heightInMetatiles = bottom.get_height() / METATILE_SIDE_LENGTH;

  for (size_t metatileIndex = 0; metatileIndex < widthInMetatiles * heightInMetatiles; metatileIndex++) {
    size_t metatileRow = metatileIndex / widthInMetatiles;
    size_t metatileCol = metatileIndex % widthInMetatiles;
    for (size_t bottomTileIndex = 0; bottomTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         bottomTileIndex++) {
      size_t tileRow = bottomTileIndex / METATILE_TILE_SIDE_LENGTH;
      size_t tileCol = bottomTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile bottomTile{};
      bottomTile.type = TileType::LAYERED;
      bottomTile.layer = TileLayer::BOTTOM;
      bottomTile.metatileIndex = metatileIndex;
      bottomTile.subtile = static_cast<Subtile>(bottomTileIndex);
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
        bottomTile.pixels[pixelIndex].red = bottom[pixelRow][pixelCol].red;
        bottomTile.pixels[pixelIndex].green = bottom[pixelRow][pixelCol].green;
        bottomTile.pixels[pixelIndex].blue = bottom[pixelRow][pixelCol].blue;
        bottomTile.pixels[pixelIndex].alpha = bottom[pixelRow][pixelCol].alpha;
      }
      decompiledTiles.tiles.push_back(bottomTile);
    }
    for (size_t middleTileIndex = 0; middleTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         middleTileIndex++) {
      size_t tileRow = middleTileIndex / METATILE_TILE_SIDE_LENGTH;
      size_t tileCol = middleTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile middleTile{};
      middleTile.type = TileType::LAYERED;
      middleTile.layer = TileLayer::MIDDLE;
      middleTile.metatileIndex = metatileIndex;
      middleTile.subtile = static_cast<Subtile>(middleTileIndex);
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
        middleTile.pixels[pixelIndex].red = middle[pixelRow][pixelCol].red;
        middleTile.pixels[pixelIndex].green = middle[pixelRow][pixelCol].green;
        middleTile.pixels[pixelIndex].blue = middle[pixelRow][pixelCol].blue;
        middleTile.pixels[pixelIndex].alpha = middle[pixelRow][pixelCol].alpha;
      }
      decompiledTiles.tiles.push_back(middleTile);
    }
    for (size_t topTileIndex = 0; topTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         topTileIndex++) {
      size_t tileRow = topTileIndex / METATILE_TILE_SIDE_LENGTH;
      size_t tileCol = topTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile topTile{};
      topTile.type = TileType::LAYERED;
      topTile.layer = TileLayer::TOP;
      topTile.metatileIndex = metatileIndex;
      topTile.subtile = static_cast<Subtile>(topTileIndex);
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
        topTile.pixels[pixelIndex].red = top[pixelRow][pixelCol].red;
        topTile.pixels[pixelIndex].green = top[pixelRow][pixelCol].green;
        topTile.pixels[pixelIndex].blue = top[pixelRow][pixelCol].blue;
        topTile.pixels[pixelIndex].alpha = top[pixelRow][pixelCol].alpha;
      }
      decompiledTiles.tiles.push_back(topTile);
    }
  }

  return decompiledTiles;
}

void importAnimTiles(PtContext &ctx, const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles)
{
  std::vector<DecompiledAnimation> anims{};

  for (const auto &rawAnim : rawAnims) {
    if (rawAnim.empty()) {
      internalerror_custom("importer::importAnimTiles rawAnim was empty");
    }

    std::string animName = rawAnim.at(0).animName;
    DecompiledAnimation anim{animName};
    for (const auto &rawFrame : rawAnim) {
      DecompiledAnimFrame animFrame{rawFrame.frame};

      if (rawFrame.png.get_height() % TILE_SIDE_LENGTH != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frame, "height",
                                           rawFrame.png.get_height());
      }
      if (rawFrame.png.get_width() % TILE_SIDE_LENGTH != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frame, "width",
                                           rawFrame.png.get_width());
      }

      if (ctx.err.errCount > 0) {
        die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                       "anim frame input dimension not divisible by 8");
      }

      // TODO : throw if this frame's dimensions don't match dimensions of other frames in this anim

      std::size_t pngWidthInTiles = rawFrame.png.get_width() / TILE_SIDE_LENGTH;
      std::size_t pngHeightInTiles = rawFrame.png.get_height() / TILE_SIDE_LENGTH;
      for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / pngWidthInTiles;
        size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        tile.type = TileType::ANIM;
        tile.anim = rawFrame.animName;
        tile.frame = rawFrame.frame;
        tile.tileIndex = tileIndex;

        for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
          size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
          size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
          tile.pixels[pixelIndex].red = rawFrame.png[pixelRow][pixelCol].red;
          tile.pixels[pixelIndex].green = rawFrame.png[pixelRow][pixelCol].green;
          tile.pixels[pixelIndex].blue = rawFrame.png[pixelRow][pixelCol].blue;
          tile.pixels[pixelIndex].alpha = rawFrame.png[pixelRow][pixelCol].alpha;
        }
        animFrame.tiles.push_back(tile);
      }
      anim.frames.push_back(animFrame);
    }
    anims.push_back(anim);
  }

  tiles.anims = anims;
}

} // namespace porytiles

TEST_CASE("importTilesFromPng should read an RGBA PNG into a DecompiledTileset in tile-wise left-to-right, "
          "top-to-bottom order")
{
  porytiles::PtContext ctx{};
  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_1.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_1.png"};

  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);

  // Tile 0 should have blue stripe from top left to bottom right
  CHECK(tiles.tiles[0].pixels[0] == porytiles::RGBA_BLUE);
  CHECK(tiles.tiles[0].pixels[9] == porytiles::RGBA_BLUE);
  CHECK(tiles.tiles[0].pixels[54] == porytiles::RGBA_BLUE);
  CHECK(tiles.tiles[0].pixels[63] == porytiles::RGBA_BLUE);
  CHECK(tiles.tiles[0].pixels[1] == porytiles::RGBA_MAGENTA);
  CHECK(tiles.tiles[0].type == porytiles::TileType::FREESTANDING);
  CHECK(tiles.tiles[0].tileIndex == 0);

  // Tile 1 should have red stripe from top right to bottom left
  CHECK(tiles.tiles[1].pixels[7] == porytiles::RGBA_RED);
  CHECK(tiles.tiles[1].pixels[14] == porytiles::RGBA_RED);
  CHECK(tiles.tiles[1].pixels[49] == porytiles::RGBA_RED);
  CHECK(tiles.tiles[1].pixels[56] == porytiles::RGBA_RED);
  CHECK(tiles.tiles[1].pixels[0] == porytiles::RGBA_MAGENTA);
  CHECK(tiles.tiles[1].type == porytiles::TileType::FREESTANDING);
  CHECK(tiles.tiles[1].tileIndex == 1);

  // Tile 2 should have green stripe from top right to bottom left
  CHECK(tiles.tiles[2].pixels[7] == porytiles::RGBA_GREEN);
  CHECK(tiles.tiles[2].pixels[14] == porytiles::RGBA_GREEN);
  CHECK(tiles.tiles[2].pixels[49] == porytiles::RGBA_GREEN);
  CHECK(tiles.tiles[2].pixels[56] == porytiles::RGBA_GREEN);
  CHECK(tiles.tiles[2].pixels[0] == porytiles::RGBA_MAGENTA);
  CHECK(tiles.tiles[2].type == porytiles::TileType::FREESTANDING);
  CHECK(tiles.tiles[2].tileIndex == 2);

  // Tile 3 should have yellow stripe from top left to bottom right
  CHECK(tiles.tiles[3].pixels[0] == porytiles::RGBA_YELLOW);
  CHECK(tiles.tiles[3].pixels[9] == porytiles::RGBA_YELLOW);
  CHECK(tiles.tiles[3].pixels[54] == porytiles::RGBA_YELLOW);
  CHECK(tiles.tiles[3].pixels[63] == porytiles::RGBA_YELLOW);
  CHECK(tiles.tiles[3].pixels[1] == porytiles::RGBA_MAGENTA);
  CHECK(tiles.tiles[3].type == porytiles::TileType::FREESTANDING);
  CHECK(tiles.tiles[3].tileIndex == 3);
}

TEST_CASE("importLayeredTilesFromPngs should read the RGBA PNGs into a DecompiledTileset in correct metatile order")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/top.png"));

  png::image<png::rgba_pixel> bottom{"res/tests/simple_metatiles_1/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/simple_metatiles_1/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/simple_metatiles_1/top.png"};

  porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(ctx, bottom, middle, top);

  // Metatile 0 bottom layer
  CHECK(tiles.tiles[0] == porytiles::RGBA_TILE_RED);
  CHECK(tiles.tiles[0].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[0].layer == porytiles::TileLayer::BOTTOM);
  CHECK(tiles.tiles[0].metatileIndex == 0);
  CHECK(tiles.tiles[0].subtile == porytiles::Subtile::NORTHWEST);
  CHECK(tiles.tiles[1] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[1].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[1].layer == porytiles::TileLayer::BOTTOM);
  CHECK(tiles.tiles[1].metatileIndex == 0);
  CHECK(tiles.tiles[1].subtile == porytiles::Subtile::NORTHEAST);
  CHECK(tiles.tiles[2] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[2].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[2].layer == porytiles::TileLayer::BOTTOM);
  CHECK(tiles.tiles[2].metatileIndex == 0);
  CHECK(tiles.tiles[2].subtile == porytiles::Subtile::SOUTHWEST);
  CHECK(tiles.tiles[3] == porytiles::RGBA_TILE_YELLOW);
  CHECK(tiles.tiles[3].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[3].layer == porytiles::TileLayer::BOTTOM);
  CHECK(tiles.tiles[3].metatileIndex == 0);
  CHECK(tiles.tiles[3].subtile == porytiles::Subtile::SOUTHEAST);

  // Metatile 0 middle layer
  CHECK(tiles.tiles[4] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[4].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[4].layer == porytiles::TileLayer::MIDDLE);
  CHECK(tiles.tiles[4].metatileIndex == 0);
  CHECK(tiles.tiles[4].subtile == porytiles::Subtile::NORTHWEST);
  CHECK(tiles.tiles[5] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[5].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[5].layer == porytiles::TileLayer::MIDDLE);
  CHECK(tiles.tiles[5].metatileIndex == 0);
  CHECK(tiles.tiles[5].subtile == porytiles::Subtile::NORTHEAST);
  CHECK(tiles.tiles[6] == porytiles::RGBA_TILE_GREEN);
  CHECK(tiles.tiles[6].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[6].layer == porytiles::TileLayer::MIDDLE);
  CHECK(tiles.tiles[6].metatileIndex == 0);
  CHECK(tiles.tiles[6].subtile == porytiles::Subtile::SOUTHWEST);
  CHECK(tiles.tiles[7] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[7].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[7].layer == porytiles::TileLayer::MIDDLE);
  CHECK(tiles.tiles[7].metatileIndex == 0);
  CHECK(tiles.tiles[7].subtile == porytiles::Subtile::SOUTHEAST);

  // Metatile 0 top layer
  CHECK(tiles.tiles[8] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[8].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[8].layer == porytiles::TileLayer::TOP);
  CHECK(tiles.tiles[8].metatileIndex == 0);
  CHECK(tiles.tiles[8].subtile == porytiles::Subtile::NORTHWEST);
  CHECK(tiles.tiles[9] == porytiles::RGBA_TILE_BLUE);
  CHECK(tiles.tiles[9].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[9].layer == porytiles::TileLayer::TOP);
  CHECK(tiles.tiles[9].metatileIndex == 0);
  CHECK(tiles.tiles[9].subtile == porytiles::Subtile::NORTHEAST);
  CHECK(tiles.tiles[10] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[10].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[10].layer == porytiles::TileLayer::TOP);
  CHECK(tiles.tiles[10].metatileIndex == 0);
  CHECK(tiles.tiles[10].subtile == porytiles::Subtile::SOUTHWEST);
  CHECK(tiles.tiles[11] == porytiles::RGBA_TILE_MAGENTA);
  CHECK(tiles.tiles[11].type == porytiles::TileType::LAYERED);
  CHECK(tiles.tiles[11].layer == porytiles::TileLayer::TOP);
  CHECK(tiles.tiles[11].metatileIndex == 0);
  CHECK(tiles.tiles[11].subtile == porytiles::Subtile::SOUTHEAST);
}

TEST_CASE("importAnimTiles should read each animation and correctly populate the DecompiledTileset anims field")
{
  porytiles::PtContext ctx{};
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white"));
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow"));

  porytiles::AnimationPng<png::rgba_pixel> white00{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/00.png"},
                                                   "anim_flower_white", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> white01{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/01.png"},
                                                   "anim_flower_white", "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> white02{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/02.png"},
                                                   "anim_flower_white", "02.png"};

  porytiles::AnimationPng<png::rgba_pixel> yellow00{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/00.png"},
                                                    "anim_flower_yellow", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> yellow01{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/01.png"},
                                                    "anim_flower_yellow", "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> yellow02{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/02.png"},
                                                    "anim_flower_yellow", "02.png"};

  std::vector<porytiles::AnimationPng<png::rgba_pixel>> whiteAnim{};
  std::vector<porytiles::AnimationPng<png::rgba_pixel>> yellowAnim{};

  whiteAnim.push_back(white00);
  whiteAnim.push_back(white01);
  whiteAnim.push_back(white02);

  yellowAnim.push_back(yellow00);
  yellowAnim.push_back(yellow01);
  yellowAnim.push_back(yellow02);

  std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
  anims.push_back(whiteAnim);
  anims.push_back(yellowAnim);

  porytiles::DecompiledTileset tiles{};

  porytiles::importAnimTiles(ctx, anims, tiles);

  CHECK(tiles.anims.size() == 2);
  CHECK(tiles.anims.at(0).size() == 3);
  CHECK(tiles.anims.at(1).size() == 3);

  // white flower, frame 0
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame0_tile0.png"));
  png::image<png::rgba_pixel> frame0Tile0Png{"res/tests/anim_flower_white/expected/frame0_tile0.png"};
  porytiles::DecompiledTileset frame0Tile0 = porytiles::importTilesFromPng(ctx, frame0Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0) == frame0Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame0_tile1.png"));
  png::image<png::rgba_pixel> frame0Tile1Png{"res/tests/anim_flower_white/expected/frame0_tile1.png"};
  porytiles::DecompiledTileset frame0Tile1 = porytiles::importTilesFromPng(ctx, frame0Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1) == frame0Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame0_tile2.png"));
  png::image<png::rgba_pixel> frame0Tile2Png{"res/tests/anim_flower_white/expected/frame0_tile2.png"};
  porytiles::DecompiledTileset frame0Tile2 = porytiles::importTilesFromPng(ctx, frame0Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2) == frame0Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame0_tile3.png"));
  png::image<png::rgba_pixel> frame0Tile3Png{"res/tests/anim_flower_white/expected/frame0_tile3.png"};
  porytiles::DecompiledTileset frame0Tile3 = porytiles::importTilesFromPng(ctx, frame0Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3) == frame0Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3).type == porytiles::TileType::ANIM);

  // white flower, frame 1
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame1_tile0.png"));
  png::image<png::rgba_pixel> frame1Tile0Png{"res/tests/anim_flower_white/expected/frame1_tile0.png"};
  porytiles::DecompiledTileset frame1Tile0 = porytiles::importTilesFromPng(ctx, frame1Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0) == frame1Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame1_tile1.png"));
  png::image<png::rgba_pixel> frame1Tile1Png{"res/tests/anim_flower_white/expected/frame1_tile1.png"};
  porytiles::DecompiledTileset frame1Tile1 = porytiles::importTilesFromPng(ctx, frame1Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1) == frame1Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame1_tile2.png"));
  png::image<png::rgba_pixel> frame1Tile2Png{"res/tests/anim_flower_white/expected/frame1_tile2.png"};
  porytiles::DecompiledTileset frame1Tile2 = porytiles::importTilesFromPng(ctx, frame1Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2) == frame1Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame1_tile3.png"));
  png::image<png::rgba_pixel> frame1Tile3Png{"res/tests/anim_flower_white/expected/frame1_tile3.png"};
  porytiles::DecompiledTileset frame1Tile3 = porytiles::importTilesFromPng(ctx, frame1Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3) == frame1Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3).type == porytiles::TileType::ANIM);

  // white flower, frame 2
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame2_tile0.png"));
  png::image<png::rgba_pixel> frame2Tile0Png{"res/tests/anim_flower_white/expected/frame2_tile0.png"};
  porytiles::DecompiledTileset frame2Tile0 = porytiles::importTilesFromPng(ctx, frame2Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0) == frame2Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame2_tile1.png"));
  png::image<png::rgba_pixel> frame2Tile1Png{"res/tests/anim_flower_white/expected/frame2_tile1.png"};
  porytiles::DecompiledTileset frame2Tile1 = porytiles::importTilesFromPng(ctx, frame2Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1) == frame2Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame2_tile2.png"));
  png::image<png::rgba_pixel> frame2Tile2Png{"res/tests/anim_flower_white/expected/frame2_tile2.png"};
  porytiles::DecompiledTileset frame2Tile2 = porytiles::importTilesFromPng(ctx, frame2Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2) == frame2Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white/expected/frame2_tile3.png"));
  png::image<png::rgba_pixel> frame2Tile3Png{"res/tests/anim_flower_white/expected/frame2_tile3.png"};
  porytiles::DecompiledTileset frame2Tile3 = porytiles::importTilesFromPng(ctx, frame2Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3) == frame2Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 0
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame0_tile0.png"));
  png::image<png::rgba_pixel> frame0Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile0.png"};
  porytiles::DecompiledTileset frame0Tile0_yellow = porytiles::importTilesFromPng(ctx, frame0Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0) == frame0Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame0_tile1.png"));
  png::image<png::rgba_pixel> frame0Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile1.png"};
  porytiles::DecompiledTileset frame0Tile1_yellow = porytiles::importTilesFromPng(ctx, frame0Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1) == frame0Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame0_tile2.png"));
  png::image<png::rgba_pixel> frame0Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile2.png"};
  porytiles::DecompiledTileset frame0Tile2_yellow = porytiles::importTilesFromPng(ctx, frame0Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2) == frame0Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame0_tile3.png"));
  png::image<png::rgba_pixel> frame0Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile3.png"};
  porytiles::DecompiledTileset frame0Tile3_yellow = porytiles::importTilesFromPng(ctx, frame0Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3) == frame0Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 1
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame1_tile0.png"));
  png::image<png::rgba_pixel> frame1Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile0.png"};
  porytiles::DecompiledTileset frame1Tile0_yellow = porytiles::importTilesFromPng(ctx, frame1Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0) == frame1Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame1_tile1.png"));
  png::image<png::rgba_pixel> frame1Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile1.png"};
  porytiles::DecompiledTileset frame1Tile1_yellow = porytiles::importTilesFromPng(ctx, frame1Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1) == frame1Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame1_tile2.png"));
  png::image<png::rgba_pixel> frame1Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile2.png"};
  porytiles::DecompiledTileset frame1Tile2_yellow = porytiles::importTilesFromPng(ctx, frame1Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2) == frame1Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame1_tile3.png"));
  png::image<png::rgba_pixel> frame1Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile3.png"};
  porytiles::DecompiledTileset frame1Tile3_yellow = porytiles::importTilesFromPng(ctx, frame1Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3) == frame1Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 2
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame2_tile0.png"));
  png::image<png::rgba_pixel> frame2Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile0.png"};
  porytiles::DecompiledTileset frame2Tile0_yellow = porytiles::importTilesFromPng(ctx, frame2Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0) == frame2Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame2_tile1.png"));
  png::image<png::rgba_pixel> frame2Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile1.png"};
  porytiles::DecompiledTileset frame2Tile1_yellow = porytiles::importTilesFromPng(ctx, frame2Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1) == frame2Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame2_tile2.png"));
  png::image<png::rgba_pixel> frame2Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile2.png"};
  porytiles::DecompiledTileset frame2Tile2_yellow = porytiles::importTilesFromPng(ctx, frame2Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2) == frame2Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow/expected/frame2_tile3.png"));
  png::image<png::rgba_pixel> frame2Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile3.png"};
  porytiles::DecompiledTileset frame2Tile3_yellow = porytiles::importTilesFromPng(ctx, frame2Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3) == frame2Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3).type == porytiles::TileType::ANIM);
}
