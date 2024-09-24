#include "importer.h"

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include <algorithm>
#include <bitset>
#include <csv.h>
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <png.hpp>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "cli_options.h"
#include "driver.h"
#include "emitter.h"
#include "errors_warnings.h"
#include "logger.h"
#include "porytiles_context.h"
#include "porytiles_exception.h"
#include "types.h"
#include "utilities.h"

namespace porytiles {

DecompiledTileset importTilesFromPng(PorytilesContext &ctx, CompilerMode compilerMode,
                                     const png::image<png::rgba_pixel> &png)
{
  if (png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.compilerSrcPaths, "height", png.get_height());
  }
  if (png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.compilerSrcPaths, "width", png.get_width());
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                   "freestanding source dimension not divisible by 8");
  }

  DecompiledTileset decompiledTiles;

  std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH_PIX;
  std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH_PIX;

  for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / pngWidthInTiles;
    std::size_t tileCol = tileIndex % pngWidthInTiles;
    RGBATile tile{};
    tile.type = TileType::FREESTANDING;
    tile.tileIndex = tileIndex;
    for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
      tile.pixels[pixelIndex].red = png[pixelRow][pixelCol].red;
      tile.pixels[pixelIndex].green = png[pixelRow][pixelCol].green;
      tile.pixels[pixelIndex].blue = png[pixelRow][pixelCol].blue;
      tile.pixels[pixelIndex].alpha = png[pixelRow][pixelCol].alpha;
    }
    decompiledTiles.tiles.push_back(tile);
  }
  return decompiledTiles;
}

static std::bitset<3> getLayerBitset(const RGBA32 &transparentColor, const RGBATile &bottomTile,
                                     const RGBATile &middleTile, const RGBATile &topTile)
{
  std::bitset<3> layers{};
  if (!bottomTile.transparent(transparentColor)) {
    layers.set(0);
  }
  if (!middleTile.transparent(transparentColor)) {
    layers.set(1);
  }
  if (!topTile.transparent(transparentColor)) {
    layers.set(2);
  }
  return layers;
}

static LayerType layerBitsetToLayerType(PorytilesContext &ctx, std::bitset<3> layerBitset, std::size_t metatileIndex)
{
  bool bottomHasContent = layerBitset.test(0);
  bool middleHasContent = layerBitset.test(1);
  bool topHasContent = layerBitset.test(2);

  if (bottomHasContent && middleHasContent && topHasContent) {
    error_allThreeLayersHadNonTransparentContent(ctx.err, metatileIndex);
    return LayerType::TRIPLE;
  }
  else if (!bottomHasContent && !middleHasContent && !topHasContent) {
    // transparent tile case
    return LayerType::NORMAL;
  }
  else if (bottomHasContent && !middleHasContent && !topHasContent) {
    return LayerType::COVERED;
  }
  else if (!bottomHasContent && middleHasContent && !topHasContent) {
    return LayerType::NORMAL;
  }
  else if (!bottomHasContent && !middleHasContent && topHasContent) {
    return LayerType::NORMAL;
  }
  else if (!bottomHasContent && middleHasContent && topHasContent) {
    return LayerType::NORMAL;
  }
  else if (bottomHasContent && middleHasContent && !topHasContent) {
    return LayerType::COVERED;
  }

  // bottomHasContent && !middleHasContent && topHasContent
  return LayerType::SPLIT;
}

DecompiledTileset importLayeredTilesFromPngs(PorytilesContext &ctx, CompilerMode compilerMode,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap,
                                             const png::image<png::rgba_pixel> &bottom,
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
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), "source layer png dimensions invalid");
  }

  DecompiledTileset decompiledTiles{};

  // Since all widths and heights are the same, we can just read the bottom layer's width and height
  std::size_t widthInMetatiles = bottom.get_width() / METATILE_SIDE_LENGTH;
  std::size_t heightInMetatiles = bottom.get_height() / METATILE_SIDE_LENGTH;

  for (size_t metatileIndex = 0; metatileIndex < widthInMetatiles * heightInMetatiles; metatileIndex++) {
    size_t metatileRow = metatileIndex / widthInMetatiles;
    size_t metatileCol = metatileIndex % widthInMetatiles;
    std::vector<RGBATile> bottomTiles{};
    std::vector<RGBATile> middleTiles{};
    std::vector<RGBATile> topTiles{};

    // Grab the supplied default behavior, encounter/terrain types
    // FIXME : default behavior/encounter/terrain parsing code is duped
    std::uint16_t defaultBehavior;
    EncounterType defaultEncounterType;
    TerrainType defaultTerrainType;
    try {
      defaultBehavior = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
    }
    catch (const std::exception &e) {
      defaultBehavior = 0;
      fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
                 fmt::format("supplied default behavior `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultBehavior, fmt::emphasis::bold)));
    }
    try {
      std::uint8_t encounterValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
      defaultEncounterType = encounterTypeFromInt(encounterValue);
    }
    catch (const std::exception &e) {
      defaultEncounterType = EncounterType::NONE;
      fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
                 fmt::format("supplied default EncounterType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultEncounterType, fmt::emphasis::bold)));
    }
    try {
      std::uint8_t terrainValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
      defaultTerrainType = terrainTypeFromInt(terrainValue);
    }
    catch (const std::exception &e) {
      defaultTerrainType = TerrainType::NORMAL;
      fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
                 fmt::format("supplied default TerrainType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultTerrainType, fmt::emphasis::bold)));
    }

    // Attributes are per-metatile so we can compute them once here
    Attributes metatileAttributes{};
    metatileAttributes.baseGame = ctx.targetBaseGame;
    metatileAttributes.metatileBehavior = defaultBehavior;
    metatileAttributes.encounterType = defaultEncounterType;
    metatileAttributes.terrainType = defaultTerrainType;
    if (attributesMap.contains(metatileIndex)) {
      const Attributes &fromMap = attributesMap.at(metatileIndex);
      metatileAttributes.metatileBehavior = fromMap.metatileBehavior;
      metatileAttributes.encounterType = fromMap.encounterType;
      metatileAttributes.terrainType = fromMap.terrainType;
    }

    // Bottom layer
    for (std::size_t bottomTileIndex = 0;
         bottomTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES; bottomTileIndex++) {
      std::size_t tileRow = bottomTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
      std::size_t tileCol = bottomTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
      RGBATile bottomTile{};
      bottomTile.type = TileType::LAYERED;
      bottomTile.layer = TileLayer::BOTTOM;
      bottomTile.metatileIndex = metatileIndex;
      bottomTile.subtile = static_cast<Subtile>(bottomTileIndex);
      bottomTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex / TILE_SIDE_LENGTH_PIX);
        std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex % TILE_SIDE_LENGTH_PIX);
        bottomTile.pixels[pixelIndex].red = bottom[pixelRow][pixelCol].red;
        bottomTile.pixels[pixelIndex].green = bottom[pixelRow][pixelCol].green;
        bottomTile.pixels[pixelIndex].blue = bottom[pixelRow][pixelCol].blue;
        bottomTile.pixels[pixelIndex].alpha = bottom[pixelRow][pixelCol].alpha;
      }
      bottomTiles.push_back(bottomTile);
    }

    // Middle layer
    for (std::size_t middleTileIndex = 0;
         middleTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES; middleTileIndex++) {
      std::size_t tileRow = middleTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
      std::size_t tileCol = middleTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
      RGBATile middleTile{};
      middleTile.type = TileType::LAYERED;
      middleTile.layer = TileLayer::MIDDLE;
      middleTile.metatileIndex = metatileIndex;
      middleTile.subtile = static_cast<Subtile>(middleTileIndex);
      middleTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex / TILE_SIDE_LENGTH_PIX);
        std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex % TILE_SIDE_LENGTH_PIX);
        middleTile.pixels[pixelIndex].red = middle[pixelRow][pixelCol].red;
        middleTile.pixels[pixelIndex].green = middle[pixelRow][pixelCol].green;
        middleTile.pixels[pixelIndex].blue = middle[pixelRow][pixelCol].blue;
        middleTile.pixels[pixelIndex].alpha = middle[pixelRow][pixelCol].alpha;
      }
      middleTiles.push_back(middleTile);
    }

    // Top layer
    for (std::size_t topTileIndex = 0; topTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES;
         topTileIndex++) {
      std::size_t tileRow = topTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
      std::size_t tileCol = topTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
      RGBATile topTile{};
      topTile.type = TileType::LAYERED;
      topTile.layer = TileLayer::TOP;
      topTile.metatileIndex = metatileIndex;
      topTile.subtile = static_cast<Subtile>(topTileIndex);
      topTile.attributes = metatileAttributes;
      topTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex / TILE_SIDE_LENGTH_PIX);
        std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                               (pixelIndex % TILE_SIDE_LENGTH_PIX);
        topTile.pixels[pixelIndex].red = top[pixelRow][pixelCol].red;
        topTile.pixels[pixelIndex].green = top[pixelRow][pixelCol].green;
        topTile.pixels[pixelIndex].blue = top[pixelRow][pixelCol].blue;
        topTile.pixels[pixelIndex].alpha = top[pixelRow][pixelCol].alpha;
      }
      topTiles.push_back(topTile);
    }

    if (bottomTiles.size() != middleTiles.size() || middleTiles.size() != topTiles.size()) {
      internalerror("importer::importLayeredTilesFromPng bottomTiles, middleTiles, topTiles sizes were not equivalent");
    }

    if (ctx.compilerConfig.tripleLayer) {
      // Triple layer case is easy, just set all three layers to LayerType::TRIPLE
      for (std::size_t i = 0; i < bottomTiles.size(); i++) {
        bottomTiles.at(i).attributes.layerType = LayerType::TRIPLE;
        middleTiles.at(i).attributes.layerType = LayerType::TRIPLE;
        topTiles.at(i).attributes.layerType = LayerType::TRIPLE;
      }
    }
    else {
      /*
       * Determine layer type by assigning each logical layer to a bit in a bitset. Then we compute the "layer bitset"
       * for each subtile in the metatile. Finally, we OR all these bitsets together. If the final bitset size is 2
       * or less, then we know we have a valid dual-layer tile. We can then read out the bits to determine which
       * layer type best describes this tile.
       */
      std::bitset<3> layers =
          getLayerBitset(ctx.compilerConfig.transparencyColor, bottomTiles.at(0), middleTiles.at(0), topTiles.at(0));
      for (std::size_t i = 1; i < bottomTiles.size(); i++) {
        std::bitset<3> newLayers =
            getLayerBitset(ctx.compilerConfig.transparencyColor, bottomTiles.at(i), middleTiles.at(i), topTiles.at(i));
        layers |= newLayers;
      }
      LayerType type = layerBitsetToLayerType(ctx, layers, metatileIndex);
      for (std::size_t i = 0; i < bottomTiles.size(); i++) {
        bottomTiles.at(i).attributes.layerType = type;
        middleTiles.at(i).attributes.layerType = type;
        topTiles.at(i).attributes.layerType = type;
      }
    }

    // Copy the tiles into the decompiled buffer, accounting for the LayerType we just computed
    switch (bottomTiles.at(0).attributes.layerType) {
    case LayerType::TRIPLE:
      for (std::size_t i = 0; i < bottomTiles.size(); i++) {
        decompiledTiles.tiles.push_back(bottomTiles.at(i));
      }
      for (std::size_t i = 0; i < middleTiles.size(); i++) {
        decompiledTiles.tiles.push_back(middleTiles.at(i));
      }
      for (std::size_t i = 0; i < topTiles.size(); i++) {
        decompiledTiles.tiles.push_back(topTiles.at(i));
      }
      break;
    case LayerType::NORMAL:
      for (std::size_t i = 0; i < middleTiles.size(); i++) {
        decompiledTiles.tiles.push_back(middleTiles.at(i));
      }
      for (std::size_t i = 0; i < topTiles.size(); i++) {
        decompiledTiles.tiles.push_back(topTiles.at(i));
      }
      break;
    case LayerType::COVERED:
      for (std::size_t i = 0; i < bottomTiles.size(); i++) {
        decompiledTiles.tiles.push_back(bottomTiles.at(i));
      }
      for (std::size_t i = 0; i < middleTiles.size(); i++) {
        decompiledTiles.tiles.push_back(middleTiles.at(i));
      }
      break;
    case LayerType::SPLIT:
      for (std::size_t i = 0; i < bottomTiles.size(); i++) {
        decompiledTiles.tiles.push_back(bottomTiles.at(i));
      }
      for (std::size_t i = 0; i < topTiles.size(); i++) {
        decompiledTiles.tiles.push_back(topTiles.at(i));
      }
      break;
    default:
      internalerror("importer::importLayeredTilesFromPng unknown LayerType");
    }
  }

  std::size_t metatileCount = decompiledTiles.tiles.size() / (ctx.compilerConfig.tripleLayer ? 12 : 8);
  for (const auto &[metatileId, _] : attributesMap) {
    if (metatileId > metatileCount - 1) {
      warn_unusedAttribute(ctx.err, metatileId, metatileCount,
                           ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string());
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                   "errors generated during layered tile import");
  }

  return decompiledTiles;
}

void importAnimTiles(PorytilesContext &ctx, CompilerMode compilerMode,
                     const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims, DecompiledTileset &tiles)
{
  std::vector<DecompiledAnimation> anims{};

  for (const auto &rawAnim : rawAnims) {
    if (rawAnim.empty()) {
      internalerror("importer::importAnimTiles rawAnim was empty");
    }

    std::set<png::uint_32> frameWidths{};
    std::set<png::uint_32> frameHeights{};
    std::string animName = rawAnim.at(0).animName;
    DecompiledAnimation anim{animName};
    for (const auto &rawFrame : rawAnim) {
      DecompiledAnimFrame animFrame{rawFrame.frameName};

      if (rawFrame.png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frameName, "height",
                                           rawFrame.png.get_height());
      }
      if (rawFrame.png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frameName, "width",
                                           rawFrame.png.get_width());
      }

      if (ctx.err.errCount > 0) {
        die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "anim frame source dimension not divisible by 8");
      }

      frameWidths.insert(rawFrame.png.get_width());
      frameHeights.insert(rawFrame.png.get_height());
      if (frameWidths.size() != 1) {
        fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, compilerMode,
                                                            rawFrame.animName, rawFrame.frameName, "width",
                                                            rawFrame.png.get_width());
      }
      if (frameHeights.size() != 1) {
        fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, compilerMode,
                                                            rawFrame.animName, rawFrame.frameName, "height",
                                                            rawFrame.png.get_height());
      }

      std::size_t pngWidthInTiles = rawFrame.png.get_width() / TILE_SIDE_LENGTH_PIX;
      std::size_t pngHeightInTiles = rawFrame.png.get_height() / TILE_SIDE_LENGTH_PIX;
      for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / pngWidthInTiles;
        std::size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        tile.type = TileType::ANIM;
        tile.anim = rawFrame.animName;
        tile.frame = rawFrame.frameName;
        tile.tileIndex = tileIndex;

        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
          std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
          std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
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

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeaderHelper(PorytilesContext &ctx, CompilerMode *compilerMode, DecompilerMode *decompilerMode,
                                   std::ifstream &behaviorFile)
{
  std::unordered_map<std::string, std::uint8_t> behaviorMap{};
  std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};

  std::string line;
  std::size_t processedUpToLine = 1;
  while (std::getline(behaviorFile, line)) {
    std::string buffer;
    std::stringstream stringStream(line);
    std::vector<std::string> tokens{};
    while (stringStream >> buffer) {
      tokens.push_back(buffer);
    }
    if (tokens.size() >= 3 && tokens.at(1).starts_with("MB_")) {
      const std::string &behaviorName = tokens.at(1);
      const std::string &behaviorValueString = tokens.at(2);
      std::uint8_t behaviorVal;
      try {
        std::size_t pos;
        behaviorVal = std::stoi(behaviorValueString, &pos, 0);
        if (std::string{behaviorValueString}.size() != pos) {
          behaviorFile.close();
          // throw here so it catches below and prints an error message
          throw std::runtime_error{""};
        }
      }
      catch (const std::exception &e) {
        behaviorFile.close();
        if (compilerMode != nullptr) {
          fatalerror_invalidBehaviorValue(ctx.err, ctx.compilerSrcPaths, *compilerMode, behaviorName,
                                          behaviorValueString, processedUpToLine);
        }
        else if (decompilerMode != nullptr) {
          fatalerror_invalidBehaviorValue(ctx.err, ctx.decompilerSrcPaths, *decompilerMode, behaviorName,
                                          behaviorValueString, processedUpToLine);
        }
        else {
          internalerror("importer::importMetatileBehaviorHeader both compilerMode and decompilerMode were null");
        }
        // here so compiler won't complain
        behaviorVal = 0;
      }
      if (behaviorVal != 0xFF) {
        // Check for MB_INVALID above, only insert if it was a valid MB
        behaviorMap.insert(std::pair{behaviorName, behaviorVal});
        behaviorReverseMap.insert(std::pair{behaviorVal, behaviorName});
      }
    }
    processedUpToLine++;
  }
  behaviorFile.close();

  return std::pair{behaviorMap, behaviorReverseMap};
}

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeader(PorytilesContext &ctx, CompilerMode compilerMode, std::ifstream &behaviorFile)
{
  return importMetatileBehaviorHeaderHelper(ctx, &compilerMode, nullptr, behaviorFile);
}

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeader(PorytilesContext &ctx, DecompilerMode decompilerMode, std::ifstream &behaviorFile)
{
  return importMetatileBehaviorHeaderHelper(ctx, nullptr, &decompilerMode, behaviorFile);
}

std::unordered_map<std::size_t, Attributes>
importAttributesFromCsv(PorytilesContext &ctx, CompilerMode compilerMode,
                        const std::unordered_map<std::string, std::uint8_t> &behaviorMap, const std::string &filePath)
{
  std::unordered_map<std::size_t, Attributes> attributeMap{};
  std::unordered_map<std::size_t, std::size_t> lineFirstSeen{};
  io::CSVReader<4> in{filePath};
  try {
    in.read_header(io::ignore_missing_column, "id", "behavior", "terrainType", "encounterType");
  }
  catch (const std::exception &e) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.compilerSrcPaths, compilerMode, filePath);
  }

  std::string id;
  bool hasId = in.has_column("id");

  std::string behavior;
  bool hasBehavior = in.has_column("behavior");

  std::string terrainType;
  bool hasTerrainType = in.has_column("terrainType");

  std::string encounterType;
  bool hasEncounterType = in.has_column("encounterType");

  if (!hasId || !hasBehavior || (hasTerrainType && !hasEncounterType) || (!hasTerrainType && hasEncounterType)) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.compilerSrcPaths, compilerMode, filePath);
  }

  if (ctx.targetBaseGame == TargetBaseGame::FIRERED && (!hasTerrainType || !hasEncounterType)) {
    warn_tooFewAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
  }
  if ((ctx.targetBaseGame == TargetBaseGame::EMERALD || ctx.targetBaseGame == TargetBaseGame::RUBY) &&
      (hasTerrainType || hasEncounterType)) {
    warn_tooManyAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
  }

  // Grab the supplied default behavior, encounter/terrain types
  // FIXME : default behavior/encounter/terrain parsing code is duped
  std::uint16_t defaultBehavior;
  EncounterType defaultEncounterType;
  TerrainType defaultTerrainType;
  try {
    defaultBehavior = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
  }
  catch (const std::exception &e) {
    defaultBehavior = 0;
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("supplied default behavior `{}' was not valid",
                           fmt::styled(ctx.compilerConfig.defaultBehavior, fmt::emphasis::bold)));
  }
  try {
    std::uint8_t encounterValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
    defaultEncounterType = encounterTypeFromInt(encounterValue);
  }
  catch (const std::exception &e) {
    defaultEncounterType = EncounterType::NONE;
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("supplied default EncounterType `{}' was not valid",
                           fmt::styled(ctx.compilerConfig.defaultEncounterType, fmt::emphasis::bold)));
  }
  try {
    std::uint8_t terrainValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
    defaultTerrainType = terrainTypeFromInt(terrainValue);
  }
  catch (const std::exception &e) {
    defaultTerrainType = TerrainType::NORMAL;
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("supplied default TerrainType `{}' was not valid",
                           fmt::styled(ctx.compilerConfig.defaultTerrainType, fmt::emphasis::bold)));
  }

  // processedUpToLine starts at 1 since we processed the header already, which was on line 1
  std::size_t processedUpToLine = 1;
  while (true) {
    bool readRow = false;
    try {
      readRow = in.read_row(id, behavior, terrainType, encounterType);
      processedUpToLine++;
    }
    catch (const std::exception &e) {
      // increment processedUpToLine here, since we threw before we could increment in the try
      processedUpToLine++;
      error_invalidCsvRowFormat(ctx.err, filePath, processedUpToLine);
      continue;
    }
    if (!readRow) {
      break;
    }

    Attributes attribute{};
    attribute.baseGame = ctx.targetBaseGame;
    attribute.metatileBehavior = defaultBehavior;
    attribute.encounterType = defaultEncounterType;
    attribute.terrainType = defaultTerrainType;
    if (behaviorMap.contains(behavior)) {
      attribute.metatileBehavior = behaviorMap.at(behavior);
    }
    else {
      error_unknownMetatileBehavior(ctx.err, filePath, processedUpToLine, behavior);
    }

    if (hasTerrainType) {
      try {
        attribute.terrainType = stringToTerrainType(terrainType);
      }
      catch (const std::invalid_argument &e) {
        error_invalidTerrainType(ctx.err, filePath, processedUpToLine, terrainType);
      }
    }
    if (hasEncounterType) {
      try {
        attribute.encounterType = stringToEncounterType(encounterType);
      }
      catch (const std::invalid_argument &e) {
        error_invalidEncounterType(ctx.err, filePath, processedUpToLine, encounterType);
      }
    }

    std::size_t idVal;
    try {
      std::size_t pos;
      idVal = std::stoi(id, &pos, 0);
      if (std::string{id}.size() != pos) {
        // throw here so it catches below and prints an error
        throw std::runtime_error{""};
      }
    }
    catch (const std::exception &e) {
      fatalerror_invalidIdInCsv(ctx.err, ctx.compilerSrcPaths, compilerMode, filePath, id, processedUpToLine);
      // here so compiler won't complain
      idVal = 0;
    }

    auto inserted = attributeMap.insert(std::pair{idVal, attribute});
    if (!inserted.second) {
      error_duplicateAttribute(ctx.err, filePath, processedUpToLine, idVal, lineFirstSeen.at(idVal));
    }
    if (!lineFirstSeen.contains(idVal)) {
      lineFirstSeen.insert(std::pair{idVal, processedUpToLine});
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                   "errors generated during attributes CSV parsing");
  }

  return attributeMap;
}

static void runAssignmentConfigImport(PorytilesContext &ctx, CompilerMode compilerMode, std::ifstream &config,
                                      std::string assignCachePath)
{
  std::string line;
  std::size_t processedUpToLine = 1;
  while (std::getline(config, line)) {
    trim(line);
    if (line.at(0) == '#' || line.size() == 0) {
      // If first char of line is comment marker or the line is blank, skip it.
      processedUpToLine++;
      continue;
    }
    std::vector<std::string> lineTokens = split(line, "=");
    if (lineTokens.size() != 2) {
      fatalerror_assignCacheSyntaxError(ctx.err, ctx.compilerSrcPaths, compilerMode, line, processedUpToLine,
                                        assignCachePath);
    }
    std::string key = lineTokens.at(0);
    std::string value = lineTokens.at(1);
    if (key == ASSIGN_ALGO) {
      if (value == assignAlgorithmString(AssignAlgorithm::DFS)) {
        if (compilerMode == CompilerMode::PRIMARY) {
          ctx.compilerConfig.primaryAssignAlgorithm = AssignAlgorithm::DFS;
        }
        else if (compilerMode == CompilerMode::SECONDARY) {
          ctx.compilerConfig.secondaryAssignAlgorithm = AssignAlgorithm::DFS;
        }
        else {
          internalerror(fmt::format("importer::runAssignmentConfigImport unknown CompilerMode: {}",
                                    static_cast<int>(compilerMode)));
        }
      }
      else if (value == assignAlgorithmString(AssignAlgorithm::BFS)) {
        if (compilerMode == CompilerMode::PRIMARY) {
          ctx.compilerConfig.primaryAssignAlgorithm = AssignAlgorithm::BFS;
        }
        else if (compilerMode == CompilerMode::SECONDARY) {
          ctx.compilerConfig.secondaryAssignAlgorithm = AssignAlgorithm::BFS;
        }
        else {
          internalerror(fmt::format("importer::runAssignmentConfigImport unknown CompilerMode: {}",
                                    static_cast<int>(compilerMode)));
        }
      }
      else {
        fatalerror_assignCacheInvalidValue(ctx.err, ctx.compilerSrcPaths, compilerMode, key, value, processedUpToLine,
                                           assignCachePath);
      }
    }
    else if (key == EXPLORE_CUTOFF) {
      std::size_t assignExploreValue;
      try {
        assignExploreValue = parseInteger<std::size_t>(value.c_str());
      }
      catch (const std::exception &e) {
        assignExploreValue = 0;
        fatalerror_assignCacheInvalidValue(ctx.err, ctx.compilerSrcPaths, compilerMode, key, value, processedUpToLine,
                                           assignCachePath);
      }
      if (compilerMode == CompilerMode::PRIMARY) {
        ctx.compilerConfig.primaryExploredNodeCutoff = assignExploreValue;
      }
      else if (compilerMode == CompilerMode::SECONDARY) {
        ctx.compilerConfig.secondaryExploredNodeCutoff = assignExploreValue;
      }
      else {
        internalerror(fmt::format("importer::runAssignmentConfigImport unknown CompilerMode: {}",
                                  static_cast<int>(compilerMode)));
      }
    }
    else if (key == BEST_BRANCHES) {
      if (value == SMART_PRUNE) {
        if (compilerMode == CompilerMode::PRIMARY) {
          ctx.compilerConfig.primaryBestBranches = SIZE_MAX;
          ctx.compilerConfig.primarySmartPrune = true;
        }
        else if (compilerMode == CompilerMode::SECONDARY) {
          ctx.compilerConfig.secondaryBestBranches = SIZE_MAX;
          ctx.compilerConfig.secondarySmartPrune = true;
        }
        else {
          internalerror(fmt::format("importer::runAssignmentConfigImport unknown CompilerMode: {}",
                                    static_cast<int>(compilerMode)));
        }
      }
      else {
        std::size_t bestBranchValue;
        try {
          bestBranchValue = parseInteger<std::size_t>(value.c_str());
        }
        catch (const std::exception &e) {
          bestBranchValue = 0;
          fatalerror_assignCacheInvalidValue(ctx.err, ctx.compilerSrcPaths, compilerMode, key, value, processedUpToLine,
                                             assignCachePath);
        }
        if (compilerMode == CompilerMode::PRIMARY) {
          ctx.compilerConfig.primaryBestBranches = bestBranchValue;
        }
        else if (compilerMode == CompilerMode::SECONDARY) {
          ctx.compilerConfig.secondaryBestBranches = bestBranchValue;
        }
        else {
          internalerror(fmt::format("importer::runAssignmentConfigImport unknown CompilerMode: {}",
                                    static_cast<int>(compilerMode)));
        }
      }
    }
    else {
      fatalerror_assignCacheInvalidKey(ctx.err, ctx.compilerSrcPaths, compilerMode, key, processedUpToLine,
                                       assignCachePath);
    }
    processedUpToLine++;
  }
}

void importAssignmentCache(PorytilesContext &ctx, CompilerMode compilerMode, CompilerMode parentCompilerMode,
                           std::ifstream &config)
{
  if (parentCompilerMode == CompilerMode::SECONDARY && compilerMode == CompilerMode::PRIMARY &&
      ctx.compilerConfig.providedPrimaryAssignCacheOverride) {
    /*
     * User is running compile-secondary, we are compiling the paired primary, and user supplied an explicit primary
     * override value. In this case, we don't want to read anything from the assign config. Just return.
     */
    warn_assignCacheOverride(ctx.err, compilerMode, ctx.compilerConfig, ctx.compilerSrcPaths.primaryAssignCache());
    return;
  }
  if (parentCompilerMode == CompilerMode::PRIMARY && compilerMode == CompilerMode::PRIMARY &&
      ctx.compilerConfig.providedAssignCacheOverride) {
    /*
     * User is running compile-primary, we are compiling the primary, and user supplied an explicit override value. In
     * this case, we don't want to read anything from the assign config. Just return.
     */
    warn_assignCacheOverride(ctx.err, compilerMode, ctx.compilerConfig, ctx.compilerSrcPaths.primaryAssignCache());
    return;
  }
  if (parentCompilerMode == CompilerMode::SECONDARY && compilerMode == CompilerMode::SECONDARY &&
      ctx.compilerConfig.providedAssignCacheOverride) {
    /*
     * User is running compile-secondary, we are compiling the secondary, and user supplied an explicit override value.
     * In this case, we don't want to read anything from the assign config. Just return.
     */
    warn_assignCacheOverride(ctx.err, compilerMode, ctx.compilerConfig, ctx.compilerSrcPaths.secondaryAssignCache());
    return;
  }
  runAssignmentConfigImport(ctx, compilerMode, config, ctx.compilerSrcPaths.modeBasedAssignCachePath(compilerMode));
  if (compilerMode == CompilerMode::PRIMARY) {
    ctx.compilerConfig.readPrimaryAssignCache = true;
  }
  else if (compilerMode == CompilerMode::SECONDARY) {
    ctx.compilerConfig.readSecondaryAssignCache = true;
  }
  else {
    internalerror(
        fmt::format("importer::importAssignmentCache unknown CompilerMode: {}", static_cast<int>(compilerMode)));
  }
}

static std::vector<GBAPalette> importCompiledPalettes(PorytilesContext &ctx, DecompilerMode decompilerMode,
                                                      const std::vector<std::unique_ptr<std::ifstream>> &paletteFiles)
{
  std::vector<GBAPalette> palettes{};

  for (const std::unique_ptr<std::ifstream> &stream : paletteFiles) {
    std::string line;

    /*
     * TODO : should fatal errors here have better messages? Users shouldn't ever really see these errors, since
     * compiled palettes will always presumably have correct formatting unless a user has manually messed with one
     */

    // FIXME 1.0.0 : this function assumes the pal file is DOS format, need to fix this

    std::getline(*stream, line);
    if (line.size() == 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode, "invalid blank line in pal file");
    }
    line.pop_back();
    if (line != "JASC-PAL") {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
                 fmt::format("expected `JASC-PAL' in pal file, saw `{}'", line));
    }

    std::getline(*stream, line);
    if (line.size() == 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode, "invalid blank line in pal file");
    }
    line.pop_back();
    if (line != "0100") {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
                 fmt::format("expected `0100' in pal file, saw `{}'", line));
    }

    std::getline(*stream, line);
    if (line.size() == 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode, "invalid blank line in pal file");
    }
    line.pop_back();
    if (line != "16") {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
                 fmt::format("expected `16' in pal file, saw `{}'", line));
    }

    GBAPalette palette{};
    /*
     * Set palette size to PAL_SIZE (16). There is really no way to truly tell a compiled palette's size, since 0 could
     * have been an intentional black, or it could mean the color was unset. For decompilation, we don't really care
     * anyway.
     */
    palette.size = PAL_SIZE;
    std::size_t colorIndex = 0;
    while (std::getline(*stream, line)) {
      BGR15 bgr = rgbaToBgr(parseJascLineDecompiler(ctx, decompilerMode, line));
      palette.colors.at(colorIndex) = bgr;
      colorIndex++;
    }
    palettes.push_back(palette);
  }

  return palettes;
}

static std::vector<GBATile> importCompiledTiles(PorytilesContext &ctx, const png::image<png::index_pixel> &tiles)
{
  std::vector<GBATile> gbaTiles{};

  std::size_t widthInTiles = tiles.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t heightInTiles = tiles.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  for (std::size_t tileIndex = 0; tileIndex < widthInTiles * heightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / widthInTiles;
    std::size_t tileCol = tileIndex % widthInTiles;
    GBATile tile{};
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      tile.colorIndexes.at(pixelIndex) = tiles[pixelRow][pixelCol];
    }
    gbaTiles.push_back(tile);
  }

  return gbaTiles;
}

static std::vector<MetatileEntry>
importCompiledMetatiles(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatilesBin,
                        std::unordered_map<std::size_t, Attributes> &attributesMap,
                        const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap)
{
  std::vector<MetatileEntry> metatileEntries{};

  std::vector<unsigned char> metatileDataBuf{std::istreambuf_iterator<char>(metatilesBin), {}};

  /*
   * Each subtile is 2 bytes (u16), so our byte total should be either a multiple of 16 or 24. 16 for dual-layer, since
   * there are 8 subtiles per metatile. 24 for triple layer, since there are 12 subtiles per metatile.
   */
  if (metatileDataBuf.size() % (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_DUAL) != 0 &&
      metatileDataBuf.size() % (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE) != 0) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               "decompiler input metatiles.bin corrupted, not valid uint16 data");
  }

  bool tripleLayer =
      (metatileDataBuf.size() / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE) == attributesMap.size());

  std::size_t metatileIndex = 0;
  for (std::size_t metatileBinByteIndex = 0; metatileBinByteIndex < metatileDataBuf.size(); metatileBinByteIndex += 2) {
    MetatileEntry metatileEntry{};

    // Compute the actual metatileIndex
    metatileIndex = tripleLayer ? metatileBinByteIndex / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE)
                                : metatileBinByteIndex / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_DUAL);

    std::uint16_t lowerByte = metatileDataBuf.at(metatileBinByteIndex);
    std::uint16_t upperByte = metatileDataBuf.at(metatileBinByteIndex + 1);
    std::uint16_t entryBits = (upperByte << 8) | lowerByte;
    metatileEntry.tileIndex = entryBits & 0x03FF;
    metatileEntry.hFlip = (entryBits >> 10) & 0x0001;
    metatileEntry.vFlip = (entryBits >> 11) & 0x0001;
    metatileEntry.paletteIndex = (entryBits >> 12) & 0x000F;

    metatileEntry.attributes.baseGame = ctx.targetBaseGame;
    if (tripleLayer) {
      metatileEntry.attributes.layerType = LayerType::TRIPLE;
    }
    else {
      metatileEntry.attributes.layerType = attributesMap.at(metatileIndex).layerType;
    }
    metatileEntry.attributes.metatileBehavior = attributesMap.at(metatileIndex).metatileBehavior;
    metatileEntry.attributes.encounterType = attributesMap.at(metatileIndex).encounterType;
    metatileEntry.attributes.terrainType = attributesMap.at(metatileIndex).terrainType;

    std::string behaviorString = std::to_string(metatileEntry.attributes.metatileBehavior);
    if (behaviorReverseMap.contains(metatileEntry.attributes.metatileBehavior)) {
      behaviorString = behaviorReverseMap.at(metatileEntry.attributes.metatileBehavior);
    }

    if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
      pt_logln(ctx, stderr,
               "found MetatileEntry[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}, "
               "terrainType: {}, encounterType: {}]]",
               metatileEntry.tileIndex, metatileEntry.hFlip, metatileEntry.vFlip, metatileEntry.paletteIndex,
               behaviorString, layerTypeString(metatileEntry.attributes.layerType),
               terrainTypeString(metatileEntry.attributes.terrainType),
               encounterTypeString(metatileEntry.attributes.encounterType));
    }
    else {
      pt_logln(ctx, stderr,
               "found MetatileEntry[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}]]",
               metatileEntry.tileIndex, metatileEntry.hFlip, metatileEntry.vFlip, metatileEntry.paletteIndex,
               behaviorString, layerTypeString(metatileEntry.attributes.layerType));
    }

    metatileEntries.push_back(metatileEntry);
  }

  return metatileEntries;
}

static std::unordered_map<std::size_t, Attributes>
importCompiledMetatileAttributes(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatileAttributesBin)
{
  std::vector<unsigned char> attributesDataBuf{std::istreambuf_iterator<char>(metatileAttributesBin), {}};

  std::unordered_map<std::size_t, Attributes> attributesMap{};

  std::size_t metatileCount;
  if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
    if (attributesDataBuf.size() % BYTES_PER_ATTRIBUTE_FIRERED != 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                 "decompiler input `metatile_attributes.bin' corrupted, not valid uint32 data");
    }
    metatileCount = attributesDataBuf.size() / BYTES_PER_ATTRIBUTE_FIRERED;
  }
  else {
    if (attributesDataBuf.size() % BYTES_PER_ATTRIBUTE_EMERALD != 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                 "decompiler input `metatile_attributes.bin' corrupted, not valid uint16 data");
    }
    metatileCount = attributesDataBuf.size() / BYTES_PER_ATTRIBUTE_EMERALD;
  }

  for (std::size_t metatileIndex = 0; metatileIndex < metatileCount; metatileIndex++) {
    Attributes attributes{};
    if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
      std::uint32_t byte0 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED));
      std::uint32_t byte1 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 1);
      std::uint32_t byte2 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 2);
      std::uint32_t byte3 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 3);
      std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
      attributes.metatileBehavior = attribute & 0x000001FF;
      attributes.terrainType = terrainTypeFromInt((attribute >> 9) & 0x0000001F);
      attributes.encounterType = encounterTypeFromInt((attribute >> 24) & 0x00000007);
      attributes.layerType = layerTypeFromInt((attribute >> 29) & 0x00000003);
    }
    else {
      std::uint16_t byte0 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_EMERALD));
      std::uint16_t byte1 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_EMERALD) + 1);
      std::uint16_t attribute = (byte1 << 8) | byte0;
      attributes.metatileBehavior = attribute & 0x00FF;
      attributes.layerType = layerTypeFromInt((attribute >> 12) & 0x000F);
    }
    attributesMap.insert(std::pair{metatileIndex, attributes});
  }
  return attributesMap;
}

static std::vector<CompiledAnimation>
importCompiledAnimations(PorytilesContext &ctx, DecompilerMode mode,
                         const std::vector<std::vector<AnimationPng<png::index_pixel>>> &rawAnims)
{
  std::vector<CompiledAnimation> anims{};
  for (const auto &rawAnim : rawAnims) {
    std::set<png::uint_32> frameWidths{};
    std::set<png::uint_32> frameHeights{};
    if (rawAnim.size() == 0) {
      internalerror("importer::importCompiledAnimations frames.size() was 0");
    }
    CompiledAnimation compiledAnim{rawAnim.at(0).animName};
    for (const auto &animPng : rawAnim) {
      CompiledAnimFrame animFrame{animPng.frameName};

      if (animPng.png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
        fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                   fmt::format("anim `{}' frame `{}' width `{}' was not divisible by 8",
                               fmt::styled(compiledAnim.animName, fmt::emphasis::bold),
                               fmt::styled(animFrame.frameName, fmt::emphasis::bold),
                               fmt::styled(animPng.png.get_width(), fmt::emphasis::bold)));
      }
      if (animPng.png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
        fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                   fmt::format("anim `{}' frame `{}' height `{}' was not divisible by 8",
                               fmt::styled(compiledAnim.animName, fmt::emphasis::bold),
                               fmt::styled(animFrame.frameName, fmt::emphasis::bold),
                               fmt::styled(animPng.png.get_height(), fmt::emphasis::bold)));
      }

      frameWidths.insert(animPng.png.get_width());
      frameHeights.insert(animPng.png.get_height());
      if (frameWidths.size() != 1) {
        fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                   fmt::format("anim `{}' frame `{}' width `{}' differed from previous frame width",
                               fmt::styled(compiledAnim.animName, fmt::emphasis::bold),
                               fmt::styled(animFrame.frameName, fmt::emphasis::bold),
                               fmt::styled(animPng.png.get_width(), fmt::emphasis::bold)));
      }
      if (frameHeights.size() != 1) {
        fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                   fmt::format("anim `{}' frame `{}' height `{}' differed from previous frame height",
                               fmt::styled(compiledAnim.animName, fmt::emphasis::bold),
                               fmt::styled(animFrame.frameName, fmt::emphasis::bold),
                               fmt::styled(animPng.png.get_height(), fmt::emphasis::bold)));
      }

      std::size_t pngWidthInTiles = animPng.png.get_width() / TILE_SIDE_LENGTH_PIX;
      std::size_t pngHeightInTiles = animPng.png.get_height() / TILE_SIDE_LENGTH_PIX;
      for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / pngWidthInTiles;
        std::size_t tileCol = tileIndex % pngWidthInTiles;
        GBATile tile{};
        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
          std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
          std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
          tile.colorIndexes[pixelIndex] = animPng.png[pixelRow][pixelCol];
        }
        animFrame.tiles.push_back(tile);
      }
      compiledAnim.frames.push_back(animFrame);
    }
    anims.push_back(compiledAnim);
  }

  return anims;
}

std::pair<CompiledTileset, std::unordered_map<std::size_t, Attributes>>
importCompiledTileset(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatiles, std::ifstream &attributes,
                      const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap,
                      const png::image<png::index_pixel> &tilesheetPng,
                      const std::vector<std::unique_ptr<std::ifstream>> &paletteFiles,
                      const std::vector<std::vector<AnimationPng<png::index_pixel>>> &compiledAnims)
{
  CompiledTileset tileset{};

  tileset.tiles = importCompiledTiles(ctx, tilesheetPng);
  tileset.palettes = importCompiledPalettes(ctx, mode, paletteFiles);
  auto attributesMap = importCompiledMetatileAttributes(ctx, mode, attributes);
  tileset.metatileEntries = importCompiledMetatiles(ctx, mode, metatiles, attributesMap, behaviorReverseMap);
  tileset.anims = importCompiledAnimations(ctx, mode, compiledAnims);

  /*
   * TODO : perform key frame inference here. We have to determine the key frame in order to
   * determine which palette each anim is actually using. If key frame inference fails, skip
   * decompilation of this anim?
   */

  return {tileset, attributesMap};
}

RGBATile importPalettePrimer(PorytilesContext &ctx, CompilerMode compilerMode, std::ifstream &paletteFile)
{
  RGBATile primerTile{};
  primerTile.type = TileType::PRIMER;

  /*
   * TODO : fatalerrors in this function need better messaging
   */

  // FIXME 1.0.0 : this function assumes the pal file is DOS format, need to fix this

  std::string line;
  std::getline(paletteFile, line);
  if (line.size() == 0) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode, "invalid blank line in pal file");
  }
  line.pop_back();
  if (line != "JASC-PAL") {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("expected `JASC-PAL' in pal file, saw `{}'", line));
  }
  std::getline(paletteFile, line);
  if (line.size() == 0) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode, "invalid blank line in pal file");
  }
  line.pop_back();
  if (line != "0100") {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode, fmt::format("expected `0100' in pal file, saw `{}'", line));
  }
  std::getline(paletteFile, line);
  if (line.size() == 0) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode, "invalid blank line in pal file");
  }
  line.pop_back();

  std::uint8_t paletteSize;
  try {
    paletteSize = parseInteger<std::uint8_t>(line.c_str());
  }
  catch (const std::exception &e) {
    paletteSize = 0;
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode, fmt::format("invalid palette size line {}", line));
  }
  if (paletteSize == 0 || paletteSize > PAL_SIZE - 1) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("invalid palette size {}, must be 1 <= size <= 15", line));
  }

  std::uint8_t lineCount = 0;
  while (std::getline(paletteFile, line)) {
    RGBA32 rgba = parseJascLineCompiler(ctx, compilerMode, line);
    primerTile.pixels.at(lineCount) = rgba;
    lineCount++;
    if (lineCount > PAL_SIZE - 1) {
      break;
    }
  }

  if (lineCount != paletteSize) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("line count {} did not match stated palette size {}", lineCount, paletteSize));
  }

  return primerTile;
}

} // namespace porytiles

TEST_CASE("importTilesFromPng should read an RGBA PNG into a DecompiledTileset in tile-wise left-to-right, "
          "top-to-bottom order")
{
  porytiles::PorytilesContext ctx{};
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/2x2_pattern_1.png"}));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_1.png"};

  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);

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
  porytiles::PorytilesContext ctx{};

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_1/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_1/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/simple_metatiles_1/top.png"}));

  png::image<png::rgba_pixel> bottom{"res/tests/simple_metatiles_1/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/simple_metatiles_1/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/simple_metatiles_1/top.png"};

  porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(
      ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottom, middle,
      top);

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
  porytiles::PorytilesContext ctx{};
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow"}));

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

  porytiles::importAnimTiles(ctx, porytiles::CompilerMode::PRIMARY, anims, tiles);

  CHECK(tiles.anims.size() == 2);
  CHECK(tiles.anims.at(0).size() == 3);
  CHECK(tiles.anims.at(1).size() == 3);

  // white flower, frame 0
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame0_tile0.png"}));
  png::image<png::rgba_pixel> frame0Tile0Png{"res/tests/anim_flower_white/expected/frame0_tile0.png"};
  porytiles::DecompiledTileset frame0Tile0 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0) == frame0Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame0_tile1.png"}));
  png::image<png::rgba_pixel> frame0Tile1Png{"res/tests/anim_flower_white/expected/frame0_tile1.png"};
  porytiles::DecompiledTileset frame0Tile1 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1) == frame0Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame0_tile2.png"}));
  png::image<png::rgba_pixel> frame0Tile2Png{"res/tests/anim_flower_white/expected/frame0_tile2.png"};
  porytiles::DecompiledTileset frame0Tile2 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2) == frame0Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame0_tile3.png"}));
  png::image<png::rgba_pixel> frame0Tile3Png{"res/tests/anim_flower_white/expected/frame0_tile3.png"};
  porytiles::DecompiledTileset frame0Tile3 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3) == frame0Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3).type == porytiles::TileType::ANIM);

  // white flower, frame 1
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame1_tile0.png"}));
  png::image<png::rgba_pixel> frame1Tile0Png{"res/tests/anim_flower_white/expected/frame1_tile0.png"};
  porytiles::DecompiledTileset frame1Tile0 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0) == frame1Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame1_tile1.png"}));
  png::image<png::rgba_pixel> frame1Tile1Png{"res/tests/anim_flower_white/expected/frame1_tile1.png"};
  porytiles::DecompiledTileset frame1Tile1 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1) == frame1Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame1_tile2.png"}));
  png::image<png::rgba_pixel> frame1Tile2Png{"res/tests/anim_flower_white/expected/frame1_tile2.png"};
  porytiles::DecompiledTileset frame1Tile2 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2) == frame1Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame1_tile3.png"}));
  png::image<png::rgba_pixel> frame1Tile3Png{"res/tests/anim_flower_white/expected/frame1_tile3.png"};
  porytiles::DecompiledTileset frame1Tile3 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3) == frame1Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3).type == porytiles::TileType::ANIM);

  // white flower, frame 2
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame2_tile0.png"}));
  png::image<png::rgba_pixel> frame2Tile0Png{"res/tests/anim_flower_white/expected/frame2_tile0.png"};
  porytiles::DecompiledTileset frame2Tile0 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile0Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0) == frame2Tile0.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame2_tile1.png"}));
  png::image<png::rgba_pixel> frame2Tile1Png{"res/tests/anim_flower_white/expected/frame2_tile1.png"};
  porytiles::DecompiledTileset frame2Tile1 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile1Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1) == frame2Tile1.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame2_tile2.png"}));
  png::image<png::rgba_pixel> frame2Tile2Png{"res/tests/anim_flower_white/expected/frame2_tile2.png"};
  porytiles::DecompiledTileset frame2Tile2 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile2Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2) == frame2Tile2.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_white/expected/frame2_tile3.png"}));
  png::image<png::rgba_pixel> frame2Tile3Png{"res/tests/anim_flower_white/expected/frame2_tile3.png"};
  porytiles::DecompiledTileset frame2Tile3 =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile3Png);
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3) == frame2Tile3.tiles.at(0));
  CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 0
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame0_tile0.png"}));
  png::image<png::rgba_pixel> frame0Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile0.png"};
  porytiles::DecompiledTileset frame0Tile0_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0) == frame0Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame0_tile1.png"}));
  png::image<png::rgba_pixel> frame0Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile1.png"};
  porytiles::DecompiledTileset frame0Tile1_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1) == frame0Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame0_tile2.png"}));
  png::image<png::rgba_pixel> frame0Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile2.png"};
  porytiles::DecompiledTileset frame0Tile2_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2) == frame0Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame0_tile3.png"}));
  png::image<png::rgba_pixel> frame0Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame0_tile3.png"};
  porytiles::DecompiledTileset frame0Tile3_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame0Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3) == frame0Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 1
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame1_tile0.png"}));
  png::image<png::rgba_pixel> frame1Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile0.png"};
  porytiles::DecompiledTileset frame1Tile0_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0) == frame1Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame1_tile1.png"}));
  png::image<png::rgba_pixel> frame1Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile1.png"};
  porytiles::DecompiledTileset frame1Tile1_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1) == frame1Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame1_tile2.png"}));
  png::image<png::rgba_pixel> frame1Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile2.png"};
  porytiles::DecompiledTileset frame1Tile2_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2) == frame1Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame1_tile3.png"}));
  png::image<png::rgba_pixel> frame1Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame1_tile3.png"};
  porytiles::DecompiledTileset frame1Tile3_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame1Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3) == frame1Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3).type == porytiles::TileType::ANIM);

  // yellow flower, frame 2
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame2_tile0.png"}));
  png::image<png::rgba_pixel> frame2Tile0Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile0.png"};
  porytiles::DecompiledTileset frame2Tile0_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile0Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0) == frame2Tile0_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame2_tile1.png"}));
  png::image<png::rgba_pixel> frame2Tile1Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile1.png"};
  porytiles::DecompiledTileset frame2Tile1_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile1Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1) == frame2Tile1_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame2_tile2.png"}));
  png::image<png::rgba_pixel> frame2Tile2Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile2.png"};
  porytiles::DecompiledTileset frame2Tile2_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile2Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2) == frame2Tile2_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2).type == porytiles::TileType::ANIM);

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_flower_yellow/expected/frame2_tile3.png"}));
  png::image<png::rgba_pixel> frame2Tile3Png_yellow{"res/tests/anim_flower_yellow/expected/frame2_tile3.png"};
  porytiles::DecompiledTileset frame2Tile3_yellow =
      porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, frame2Tile3Png_yellow);
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3) == frame2Tile3_yellow.tiles.at(0));
  CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3).type == porytiles::TileType::ANIM);
}

TEST_CASE("importLayeredTilesFromPngs should correctly import a dual layer tileset via layer type inference")
{
  porytiles::PorytilesContext ctx{};
  ctx.compilerConfig.tripleLayer = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/dual_layer_metatiles_1/bottom.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/dual_layer_metatiles_1/middle.png"}));
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/dual_layer_metatiles_1/top.png"}));

  png::image<png::rgba_pixel> bottom{"res/tests/dual_layer_metatiles_1/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/dual_layer_metatiles_1/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/dual_layer_metatiles_1/top.png"};

  porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(
      ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottom, middle,
      top);

  // Metatile 0
  CHECK(tiles.tiles.at(0).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(1).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(2).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(3).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(4).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(5).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(6).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(7).attributes.layerType == porytiles::LayerType::COVERED);

  // Metatile 1
  CHECK(tiles.tiles.at(8).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(9).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(10).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(11).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(12).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(13).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(14).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(15).attributes.layerType == porytiles::LayerType::NORMAL);

  // Metatile 2
  CHECK(tiles.tiles.at(16).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(17).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(18).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(19).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(20).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(21).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(22).attributes.layerType == porytiles::LayerType::SPLIT);
  CHECK(tiles.tiles.at(23).attributes.layerType == porytiles::LayerType::SPLIT);

  // Metatile 3
  CHECK(tiles.tiles.at(24).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(25).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(26).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(27).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(28).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(29).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(30).attributes.layerType == porytiles::LayerType::COVERED);
  CHECK(tiles.tiles.at(31).attributes.layerType == porytiles::LayerType::COVERED);

  // Metatile 4
  CHECK(tiles.tiles.at(32).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(33).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(34).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(35).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(36).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(37).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(38).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(39).attributes.layerType == porytiles::LayerType::NORMAL);

  // Metatile 5
  CHECK(tiles.tiles.at(40).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(41).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(42).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(43).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(44).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(45).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(46).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(47).attributes.layerType == porytiles::LayerType::NORMAL);

  // Metatile 6
  CHECK(tiles.tiles.at(48).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(49).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(50).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(51).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(52).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(53).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(54).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(55).attributes.layerType == porytiles::LayerType::NORMAL);

  // Metatile 7
  CHECK(tiles.tiles.at(56).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(57).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(58).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(59).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(60).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(61).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(62).attributes.layerType == porytiles::LayerType::NORMAL);
  CHECK(tiles.tiles.at(63).attributes.layerType == porytiles::LayerType::NORMAL);
}

TEST_CASE("importMetatileBehaviorHeader should parse metatile behaviors as expected")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::ifstream behaviorFile{"res/tests/metatile_behaviors.h"};
  auto [behaviorMap, behaviorReverseMap] =
      porytiles::importMetatileBehaviorHeader(ctx, porytiles::CompilerMode::PRIMARY, behaviorFile);
  behaviorFile.close();

  CHECK(!behaviorMap.contains("MB_INVALID"));
  CHECK(behaviorMap.at("MB_NORMAL") == 0x00);
  CHECK(behaviorMap.at("MB_SHALLOW_WATER") == 0x17);
  CHECK(behaviorMap.at("MB_ICE") == 0x20);
  CHECK(behaviorMap.at("MB_UNUSED_EF") == 0xEF);

  CHECK(!behaviorReverseMap.contains(0xFF));
  CHECK(behaviorReverseMap.at(0x00) == "MB_NORMAL");
  CHECK(behaviorReverseMap.at(0x17) == "MB_SHALLOW_WATER");
  CHECK(behaviorReverseMap.at(0x20) == "MB_ICE");
  CHECK(behaviorReverseMap.at(0xEF) == "MB_UNUSED_EF");
}

TEST_CASE("importAttributesFromCsv should parse source CSVs as expected")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("It should parse an Emerald-style attributes CSV correctly")
  {
    auto attributesMap = porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "res/tests/csv/correct_1.csv");
    CHECK_FALSE(attributesMap.contains(0));
    CHECK_FALSE(attributesMap.contains(1));
    CHECK_FALSE(attributesMap.contains(2));
    CHECK(attributesMap.contains(3));
    CHECK_FALSE(attributesMap.contains(4));
    CHECK(attributesMap.contains(5));
    CHECK_FALSE(attributesMap.contains(6));

    CHECK(attributesMap.at(3).metatileBehavior == behaviorMap.at("MB_NORMAL"));
    CHECK(attributesMap.at(5).metatileBehavior == behaviorMap.at("MB_NORMAL"));
  }

  SUBCASE("It should parse a Firered-style attributes CSV correctly")
  {
    auto attributesMap = porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "res/tests/csv/correct_2.csv");
    CHECK_FALSE(attributesMap.contains(0));
    CHECK_FALSE(attributesMap.contains(1));
    CHECK(attributesMap.contains(2));
    CHECK_FALSE(attributesMap.contains(3));
    CHECK(attributesMap.contains(4));
    CHECK_FALSE(attributesMap.contains(5));
    CHECK_FALSE(attributesMap.contains(6));

    CHECK(attributesMap.at(2).metatileBehavior == behaviorMap.at("MB_NORMAL"));
    CHECK(attributesMap.at(2).terrainType == porytiles::TerrainType::NORMAL);
    CHECK(attributesMap.at(2).encounterType == porytiles::EncounterType::NONE);
    CHECK(attributesMap.at(4).metatileBehavior == behaviorMap.at("MB_NORMAL"));
    CHECK(attributesMap.at(4).terrainType == porytiles::TerrainType::NORMAL);
    CHECK(attributesMap.at(4).encounterType == porytiles::EncounterType::NONE);
  }
}

TEST_CASE("importCompiledTileset should import a triple-layer pokeemerald tileset correctly")
{
  porytiles::PorytilesContext compileCtx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  compileCtx.output.path = parentDir;
  compileCtx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  compileCtx.err.printErrors = false;
  compileCtx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  compileCtx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  compileCtx.compilerConfig.cacheAssign = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary"}));
  compileCtx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/metatile_behaviors.h"}));
  compileCtx.compilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";
  porytiles::drive(compileCtx);

  porytiles::PorytilesContext decompileCtx{};
  decompileCtx.decompilerSrcPaths.primarySourcePath = parentDir;

  std::ifstream metatiles{decompileCtx.decompilerSrcPaths.primaryMetatilesBin(), std::ios::binary};
  std::ifstream attributes{decompileCtx.decompilerSrcPaths.primaryAttributesBin(), std::ios::binary};
  png::image<png::index_pixel> tilesheetPng{decompileCtx.decompilerSrcPaths.primaryTilesPng()};
  std::vector<std::unique_ptr<std::ifstream>> paletteFiles{};
  for (std::size_t index = 0; index < decompileCtx.fieldmapConfig.numPalettesTotal; index++) {
    std::ostringstream filename;
    if (index < 10) {
      filename << "0";
    }
    filename << index << ".pal";
    std::filesystem::path paletteFile = decompileCtx.decompilerSrcPaths.primaryPalettes() / filename.str();
    paletteFiles.push_back(std::make_unique<std::ifstream>(paletteFile));
  }
  // TODO tests : (importCompiledTileset should import a triple-layer...) actually test anims import
  auto [importedTileset, attributesMap] =
      porytiles::importCompiledTileset(decompileCtx, porytiles::DecompilerMode::PRIMARY, metatiles, attributes,
                                       std::unordered_map<std::uint8_t, std::string>{}, tilesheetPng, paletteFiles,
                                       std::vector<std::vector<porytiles::AnimationPng<png::index_pixel>>>{});
  metatiles.close();
  attributes.close();
  std::for_each(paletteFiles.begin(), paletteFiles.end(),
                [](const std::unique_ptr<std::ifstream> &stream) { stream->close(); });

  CHECK((compileCtx.compilerContext.resultTileset)->tiles.size() == importedTileset.tiles.size());
  CHECK((compileCtx.compilerContext.resultTileset)->tiles == importedTileset.tiles);

  CHECK((compileCtx.compilerContext.resultTileset)->metatileEntries.size() == importedTileset.metatileEntries.size());
  for (std::size_t entryIndex = 0; entryIndex < importedTileset.metatileEntries.size(); entryIndex++) {
    const porytiles::MetatileEntry &expectedEntry =
        (compileCtx.compilerContext.resultTileset)->metatileEntries.at(entryIndex);
    const porytiles::MetatileEntry &actualEntry = importedTileset.metatileEntries.at(entryIndex);
    CHECK(expectedEntry.tileIndex == actualEntry.tileIndex);
    CHECK(expectedEntry.hFlip == actualEntry.hFlip);
    CHECK(expectedEntry.vFlip == actualEntry.vFlip);
    CHECK(expectedEntry.paletteIndex == actualEntry.paletteIndex);
    CHECK(expectedEntry.attributes.metatileBehavior == actualEntry.attributes.metatileBehavior);
    CHECK(expectedEntry.attributes.layerType == actualEntry.attributes.layerType);
  }

  for (std::size_t palIndex = 0; palIndex < (compileCtx.compilerContext.resultTileset)->palettes.size(); palIndex++) {
    auto origPal = (compileCtx.compilerContext.resultTileset)->palettes.at(palIndex).colors;
    auto decompPal = importedTileset.palettes.at(palIndex).colors;
    for (std::size_t colorIndex = 0; colorIndex < 16; colorIndex++) {
      CHECK(origPal.at(colorIndex) == decompPal.at(colorIndex));
    }
  }

  // TODO tests : (importCompiledTileset should import a triple-layer...) check attributes map correctness

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("importCompiledTileset should import a dual-layer pokefirered tileset correctly")
{
  // TODO tests : (importCompiledTileset should import a dual-layer pokefirered tileset correctly)
}
