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

#include "driver.h"
#include "emitter.h"
#include "errors_warnings.h"
#include "logger.h"
#include "ptcontext.h"
#include "ptexception.h"
#include "types.h"
#include "utilities.h"

namespace porytiles {

DecompiledTileset importTilesFromPng(PtContext &ctx, const png::image<png::rgba_pixel> &png)
{
  if (png.get_height() % TILE_SIDE_LENGTH != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.compilerSrcPaths, "height", png.get_height());
  }
  if (png.get_width() % TILE_SIDE_LENGTH != 0) {
    error_freestandingDimensionNotDivisibleBy8(ctx.err, ctx.compilerSrcPaths, "width", png.get_width());
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "freestanding source dimension not divisible by 8");
  }

  DecompiledTileset decompiledTiles;

  std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
  std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

  for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / pngWidthInTiles;
    std::size_t tileCol = tileIndex % pngWidthInTiles;
    RGBATile tile{};
    tile.type = TileType::FREESTANDING;
    tile.tileIndex = tileIndex;
    for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
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

static LayerType layerBitsetToLayerType(PtContext &ctx, std::bitset<3> layerBitset, std::size_t metatileIndex)
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

DecompiledTileset importLayeredTilesFromPngs(PtContext &ctx,
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
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "source layer png dimensions invalid");
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

    // Attributes are per-metatile so we can compute them once here
    Attributes metatileAttributes{};
    metatileAttributes.baseGame = ctx.targetBaseGame;
    if (attributesMap.contains(metatileIndex)) {
      const Attributes &fromMap = attributesMap.at(metatileIndex);
      metatileAttributes.metatileBehavior = fromMap.metatileBehavior;
      metatileAttributes.terrainType = fromMap.terrainType;
      metatileAttributes.encounterType = fromMap.encounterType;
    }

    // Bottom layer
    for (std::size_t bottomTileIndex = 0; bottomTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         bottomTileIndex++) {
      std::size_t tileRow = bottomTileIndex / METATILE_TILE_SIDE_LENGTH;
      std::size_t tileCol = bottomTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile bottomTile{};
      bottomTile.type = TileType::LAYERED;
      bottomTile.layer = TileLayer::BOTTOM;
      bottomTile.metatileIndex = metatileIndex;
      bottomTile.subtile = static_cast<Subtile>(bottomTileIndex);
      bottomTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        std::size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
        bottomTile.pixels[pixelIndex].red = bottom[pixelRow][pixelCol].red;
        bottomTile.pixels[pixelIndex].green = bottom[pixelRow][pixelCol].green;
        bottomTile.pixels[pixelIndex].blue = bottom[pixelRow][pixelCol].blue;
        bottomTile.pixels[pixelIndex].alpha = bottom[pixelRow][pixelCol].alpha;
      }
      bottomTiles.push_back(bottomTile);
    }

    // Middle layer
    for (std::size_t middleTileIndex = 0; middleTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         middleTileIndex++) {
      std::size_t tileRow = middleTileIndex / METATILE_TILE_SIDE_LENGTH;
      std::size_t tileCol = middleTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile middleTile{};
      middleTile.type = TileType::LAYERED;
      middleTile.layer = TileLayer::MIDDLE;
      middleTile.metatileIndex = metatileIndex;
      middleTile.subtile = static_cast<Subtile>(middleTileIndex);
      middleTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        std::size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
        middleTile.pixels[pixelIndex].red = middle[pixelRow][pixelCol].red;
        middleTile.pixels[pixelIndex].green = middle[pixelRow][pixelCol].green;
        middleTile.pixels[pixelIndex].blue = middle[pixelRow][pixelCol].blue;
        middleTile.pixels[pixelIndex].alpha = middle[pixelRow][pixelCol].alpha;
      }
      middleTiles.push_back(middleTile);
    }

    // Top layer
    for (std::size_t topTileIndex = 0; topTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
         topTileIndex++) {
      std::size_t tileRow = topTileIndex / METATILE_TILE_SIDE_LENGTH;
      std::size_t tileCol = topTileIndex % METATILE_TILE_SIDE_LENGTH;
      RGBATile topTile{};
      topTile.type = TileType::LAYERED;
      topTile.layer = TileLayer::TOP;
      topTile.metatileIndex = metatileIndex;
      topTile.subtile = static_cast<Subtile>(topTileIndex);
      topTile.attributes = metatileAttributes;
      topTile.attributes = metatileAttributes;
      for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
        std::size_t pixelRow =
            (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
        std::size_t pixelCol =
            (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
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
                           ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode).string());
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "errors generated during layered tile import");
  }

  return decompiledTiles;
}

void importAnimTiles(PtContext &ctx, const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles)
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

      if (rawFrame.png.get_height() % TILE_SIDE_LENGTH != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frameName, "height",
                                           rawFrame.png.get_height());
      }
      if (rawFrame.png.get_width() % TILE_SIDE_LENGTH != 0) {
        error_animDimensionNotDivisibleBy8(ctx.err, rawFrame.animName, rawFrame.frameName, "width",
                                           rawFrame.png.get_width());
      }

      if (ctx.err.errCount > 0) {
        die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                       "anim frame source dimension not divisible by 8");
      }

      frameWidths.insert(rawFrame.png.get_width());
      frameHeights.insert(rawFrame.png.get_height());
      if (frameWidths.size() != 1) {
        fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                                            rawFrame.animName, rawFrame.frameName, "width",
                                                            rawFrame.png.get_width());
      }
      if (frameHeights.size() != 1) {
        fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                                            rawFrame.animName, rawFrame.frameName, "height",
                                                            rawFrame.png.get_height());
      }

      std::size_t pngWidthInTiles = rawFrame.png.get_width() / TILE_SIDE_LENGTH;
      std::size_t pngHeightInTiles = rawFrame.png.get_height() / TILE_SIDE_LENGTH;
      for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / pngWidthInTiles;
        std::size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        tile.type = TileType::ANIM;
        tile.anim = rawFrame.animName;
        tile.frame = rawFrame.frameName;
        tile.tileIndex = tileIndex;

        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
          std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
          std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
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

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorMaps(PtContext &ctx, std::ifstream &behaviorFile)
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
        fatalerror_invalidBehaviorValue(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode, behaviorName,
                                        behaviorValueString, processedUpToLine);
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

std::unordered_map<std::size_t, Attributes>
importAttributesFromCsv(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                        const std::string &filePath)
{
  std::unordered_map<std::size_t, Attributes> attributeMap{};
  std::unordered_map<std::size_t, std::size_t> lineFirstSeen{};
  io::CSVReader<4> in{filePath};
  try {
    in.read_header(io::ignore_missing_column, "id", "behavior", "terrainType", "encounterType");
  }
  catch (const std::exception &e) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode, filePath);
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
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode, filePath);
  }

  if (ctx.targetBaseGame == TargetBaseGame::FIRERED && (!hasTerrainType || !hasEncounterType)) {
    warn_tooFewAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
  }
  if ((ctx.targetBaseGame == TargetBaseGame::EMERALD || ctx.targetBaseGame == TargetBaseGame::RUBY) &&
      (hasTerrainType || hasEncounterType)) {
    warn_tooManyAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
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
    /*
     * TODO : instead of erroring, warn and input anyway, we want to support case where user did not provide a behavior
     */
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
      fatalerror_invalidIdInCsv(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode, filePath, id,
                                processedUpToLine);
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
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "errors generated during attributes CSV parsing");
  }

  return attributeMap;
}

static RGBA32 parseJascLine(const ErrorsAndWarnings &err, const std::string &jascLine)
{
  // FIXME : this logic duplicates cli_parser::parseRgbColor
  std::vector<std::string> colorComponents = split(jascLine, " ");
  if (colorComponents.size() != 3) {
    // FIXME : need fatalerror to also work for decompile mode
    throw std::runtime_error{"expected valid JASC line in pal file, saw " + jascLine};
  }

  if (colorComponents[0].at(colorComponents[0].size() - 1) == '\r') {
    colorComponents[0].pop_back();
  }
  if (colorComponents[1].at(colorComponents[1].size() - 1) == '\r') {
    colorComponents[1].pop_back();
  }
  if (colorComponents[2].at(colorComponents[2].size() - 1) == '\r') {
    colorComponents[2].pop_back();
  }

  int red = parseInteger<int>(colorComponents[0].c_str());
  int green = parseInteger<int>(colorComponents[1].c_str());
  int blue = parseInteger<int>(colorComponents[2].c_str());

  if (red < 0 || red > 255) {
    // FIXME : need fatalerror to also work for decompile mode
    throw std::runtime_error{"invalid red component: range must be 0 <= red <= 255"};
  }
  if (green < 0 || green > 255) {
    // FIXME : need fatalerror to also work for decompile mode
    throw std::runtime_error{"invalid green component: range must be 0 <= green <= 255"};
  }
  if (blue < 0 || blue > 255) {
    // FIXME : need fatalerror to also work for decompile mode
    throw std::runtime_error{"invalid blue component: range must be 0 <= blue <= 255"};
  }

  return RGBA32{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                ALPHA_OPAQUE};
}

static std::vector<GBAPalette> importCompiledPalettes(PtContext &ctx,
                                                      const std::vector<std::shared_ptr<std::ifstream>> &paletteFiles)
{
  std::vector<GBAPalette> palettes{};

  for (std::shared_ptr<std::ifstream> stream : paletteFiles) {
    std::string line;

    std::getline(*stream, line);
    if (line.size() == 0) {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"invalid blank line in pal file"};
    }
    line.pop_back();
    if (line != "JASC-PAL") {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"expected 'JASC-PAL' in pal file, saw " + line};
    }

    std::getline(*stream, line);
    if (line.size() == 0) {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"invalid blank line in pal file"};
    }
    line.pop_back();
    if (line != "0100") {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"expected '0100' in pal file, saw " + line};
    }

    std::getline(*stream, line);
    if (line.size() == 0) {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"invalid blank line in pal file"};
    }
    line.pop_back();
    if (line != "16") {
      // FIXME : need fatalerror to also work for decompile mode
      throw std::runtime_error{"expected '16' in pal file, saw " + line};
    }

    GBAPalette palette{};
    /*
     * Set palette size to 16, there is really no way to truly tell a compiled palette's size, since 0 could have been
     * an intentional black, or it could mean the color was unset. For decompilation, we don't really care anyway.
     */
    palette.size = 16;
    std::size_t colorIndex = 0;
    while (std::getline(*stream, line)) {
      BGR15 bgr = rgbaToBgr(parseJascLine(ctx.err, line));
      palette.colors.at(colorIndex) = bgr;
      colorIndex++;
    }
    palettes.push_back(palette);
  }

  return palettes;
}

static std::vector<GBATile> importCompiledTiles(PtContext &ctx, const png::image<png::index_pixel> &tiles)
{
  std::vector<GBATile> gbaTiles{};

  std::size_t widthInTiles = tiles.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t heightInTiles = tiles.get_height() / porytiles::TILE_SIDE_LENGTH;

  for (std::size_t tileIndex = 0; tileIndex < widthInTiles * heightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / widthInTiles;
    std::size_t tileCol = tileIndex % widthInTiles;
    GBATile tile{};
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      tile.colorIndexes.at(pixelIndex) = tiles[pixelRow][pixelCol];
    }
    gbaTiles.push_back(tile);
  }

  return gbaTiles;
}

static std::vector<Assignment>
importCompiledMetatiles(PtContext &ctx, std::ifstream &metatilesBin,
                        std::unordered_map<std::size_t, Attributes> &attributesMap,
                        const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap)
{
  std::vector<Assignment> assignments{};

  std::vector<unsigned char> metatileDataBuf{std::istreambuf_iterator<char>(metatilesBin), {}};

  /*
   * Each subtile is 2 bytes (u16), so our byte total should be either a multiple of 16 or 24. 16 for dual-layer, since
   * there are 8 subtiles per metatile. 24 for triple layer, since there are 12 subtiles per metatile.
   */
  if (metatileDataBuf.size() % 16 != 0 && metatileDataBuf.size() % 24 != 0) {
    // FIXME : need fatalerror to also work for decompile mode
    throw std::runtime_error{"decompiler input metatiles.bin corrupted, not valid uint16 data"};
  }

  bool tripleLayer = (metatileDataBuf.size() / 24 == attributesMap.size());

  std::size_t metatileIndex = 0;
  for (std::size_t metatileBinByteIndex = 0; metatileBinByteIndex < metatileDataBuf.size(); metatileBinByteIndex += 2) {
    Assignment assignment{};

    // Compute the actual metatileIndex
    metatileIndex = tripleLayer ? metatileBinByteIndex / 24 : metatileBinByteIndex / 16;

    std::uint16_t lowerByte = metatileDataBuf.at(metatileBinByteIndex);
    std::uint16_t upperByte = metatileDataBuf.at(metatileBinByteIndex + 1);
    std::uint16_t metatileEntry = (upperByte << 8) | lowerByte;
    assignment.tileIndex = metatileEntry & 0x03FF;
    assignment.hFlip = (metatileEntry >> 10) & 0x0001;
    assignment.vFlip = (metatileEntry >> 11) & 0x0001;
    assignment.paletteIndex = (metatileEntry >> 12) & 0x000F;

    assignment.attributes.baseGame = ctx.targetBaseGame;
    if (tripleLayer) {
      assignment.attributes.layerType = LayerType::TRIPLE;
    }
    else {
      assignment.attributes.layerType = attributesMap.at(metatileIndex).layerType;
    }
    assignment.attributes.metatileBehavior = attributesMap.at(metatileIndex).metatileBehavior;
    assignment.attributes.encounterType = attributesMap.at(metatileIndex).encounterType;
    assignment.attributes.terrainType = attributesMap.at(metatileIndex).terrainType;

    std::string behaviorString = std::to_string(assignment.attributes.metatileBehavior);
    if (behaviorReverseMap.contains(assignment.attributes.metatileBehavior)) {
      behaviorString = behaviorReverseMap.at(assignment.attributes.metatileBehavior);
    }

    if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
      pt_logln(ctx, stderr,
               "found Assignment[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}, "
               "terrainType: {}, encounterType: {}]]",
               assignment.tileIndex, assignment.hFlip, assignment.vFlip, assignment.paletteIndex, behaviorString,
               layerTypeString(assignment.attributes.layerType), terrainTypeString(assignment.attributes.terrainType),
               encounterTypeString(assignment.attributes.encounterType));
    }
    else {
      pt_logln(ctx, stderr,
               "found Assignment[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}]]",
               assignment.tileIndex, assignment.hFlip, assignment.vFlip, assignment.paletteIndex, behaviorString,
               layerTypeString(assignment.attributes.layerType));
    }

    assignments.push_back(assignment);
  }

  return assignments;
}

static std::unordered_map<std::size_t, Attributes>
importCompiledMetatileAttributes(PtContext &ctx, std::ifstream &metatileAttributesBin)
{
  std::vector<unsigned char> attributesDataBuf{std::istreambuf_iterator<char>(metatileAttributesBin), {}};

  std::unordered_map<std::size_t, Attributes> attributesMap{};

  std::size_t metatileCount;
  if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
    if (attributesDataBuf.size() % 4 != 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
                 "decompiler input 'metatile_attributes.bin' corrupted, not valid uint32 data");
    }
    metatileCount = attributesDataBuf.size() / 4;
  }
  else {
    if (attributesDataBuf.size() % 2 != 0) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
                 "decompiler input 'metatile_attributes.bin' corrupted, not valid uint16 data");
    }
    metatileCount = attributesDataBuf.size() / 2;
  }

  for (std::size_t metatileIndex = 0; metatileIndex < metatileCount; metatileIndex++) {
    Attributes attributes{};
    if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
      std::uint32_t byte0 = attributesDataBuf.at((metatileIndex * 4));
      std::uint32_t byte1 = attributesDataBuf.at((metatileIndex * 4) + 1);
      std::uint32_t byte2 = attributesDataBuf.at((metatileIndex * 4) + 2);
      std::uint32_t byte3 = attributesDataBuf.at((metatileIndex * 4) + 3);
      std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
      attributes.metatileBehavior = attribute & 0x000001FF;
      attributes.terrainType = terrainTypeFromInt((attribute >> 9) & 0x0000001F);
      attributes.encounterType = encounterTypeFromInt((attribute >> 24) & 0x00000007);
      attributes.layerType = layerTypeFromInt((attribute >> 29) & 0x00000003);
    }
    else {
      std::uint16_t byte0 = attributesDataBuf.at((metatileIndex * 2));
      std::uint16_t byte1 = attributesDataBuf.at((metatileIndex * 2) + 1);
      std::uint16_t attribute = (byte1 << 8) | byte0;
      attributes.metatileBehavior = attribute & 0x00FF;
      attributes.layerType = layerTypeFromInt((attribute >> 12) & 0x000F);
    }
    attributesMap.insert(std::pair{metatileIndex, attributes});
  }
  return attributesMap;
}

static std::vector<CompiledAnimation>
importCompiledAnimations(PtContext &ctx, const std::vector<std::vector<AnimationPng<png::index_pixel>>> &rawAnims)
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

      if (animPng.png.get_height() % TILE_SIDE_LENGTH != 0) {
        // TODO : fill in real error
        throw std::runtime_error{"TODO : frame.png.get_height() % TILE_SIDE_LENGTH != 0"};
        // error_animDimensionNotDivisibleBy8(ctx.err, frame.animName, frame.frameName, "height",
        // frame.png.get_height());
      }
      if (animPng.png.get_width() % TILE_SIDE_LENGTH != 0) {
        // TODO : fill in real error
        throw std::runtime_error{"TODO : frame.png.get_width() % TILE_SIDE_LENGTH != 0"};
        // error_animDimensionNotDivisibleBy8(ctx.err, frame.animName, frame.frameName, "width", frame.png.get_width());
      }

      frameWidths.insert(animPng.png.get_width());
      frameHeights.insert(animPng.png.get_height());
      if (frameWidths.size() != 1) {
        // TODO : fill in real error
        throw std::runtime_error{"TODO : frameWidths.size() != 1"};
        // fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        //                                                     frame.animName, frame.frameName, "width",
        //                                                     frame.png.get_width());
      }
      if (frameHeights.size() != 1) {
        // TODO : fill in real error
        throw std::runtime_error{"TODO : frameHeights.size() != 1"};
        // fatalerror_animFrameDimensionsDoNotMatchOtherFrames(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        //                                                     frame.animName, frame.frameName, "height",
        //                                                     frame.png.get_height());
      }

      std::size_t pngWidthInTiles = animPng.png.get_width() / TILE_SIDE_LENGTH;
      std::size_t pngHeightInTiles = animPng.png.get_height() / TILE_SIDE_LENGTH;
      for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / pngWidthInTiles;
        std::size_t tileCol = tileIndex % pngWidthInTiles;
        GBATile tile{};
        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
          std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
          std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
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
importCompiledTileset(PtContext &ctx, std::ifstream &metatiles, std::ifstream &attributes,
                      const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap,
                      const png::image<png::index_pixel> &tilesheetPng,
                      const std::vector<std::shared_ptr<std::ifstream>> &paletteFiles,
                      const std::vector<std::vector<AnimationPng<png::index_pixel>>> &compiledAnims)
{
  CompiledTileset tileset{};

  tileset.tiles = importCompiledTiles(ctx, tilesheetPng);
  tileset.palettes = importCompiledPalettes(ctx, paletteFiles);
  auto attributesMap = importCompiledMetatileAttributes(ctx, attributes);
  tileset.assignments = importCompiledMetatiles(ctx, metatiles, attributesMap, behaviorReverseMap);
  tileset.anims = importCompiledAnimations(ctx, compiledAnims);

  /*
   * TODO : perform key frame inference here. We have to determine the key frame in order to
   * determine which palette each anim is actually using. If key frame inference fails, skip
   * decompilation of this anim?
   */

  return {tileset, attributesMap};
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

  porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottom, middle, top);

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

TEST_CASE("importLayeredTilesFromPngs should correctly import a dual layer tileset via layer type inference")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.tripleLayer = false;

  REQUIRE(std::filesystem::exists("res/tests/dual_layer_metatiles_1/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/dual_layer_metatiles_1/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/dual_layer_metatiles_1/top.png"));

  png::image<png::rgba_pixel> bottom{"res/tests/dual_layer_metatiles_1/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/dual_layer_metatiles_1/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/dual_layer_metatiles_1/top.png"};

  porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(
      ctx, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottom, middle, top);

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

TEST_CASE("importMetatileBehaviorMaps should parse metatile behaviors as expected")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  std::ifstream behaviorFile{"res/tests/metatile_behaviors.h"};
  auto [behaviorMap, behaviorReverseMap] = porytiles::importMetatileBehaviorMaps(ctx, behaviorFile);
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
  porytiles::PtContext ctx{};
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("It should parse an Emerald-style attributes CSV correctly")
  {
    auto attributesMap = porytiles::importAttributesFromCsv(ctx, behaviorMap, "res/tests/csv/correct_1.csv");
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
    auto attributesMap = porytiles::importAttributesFromCsv(ctx, behaviorMap, "res/tests/csv/correct_2.csv");
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
  porytiles::PtContext compileCtx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  compileCtx.output.path = parentDir;
  compileCtx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  compileCtx.err.printErrors = false;
  compileCtx.compilerConfig.assignAlgorithm = porytiles::AssignAlgorithm::DEPTH_FIRST;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary"));
  compileCtx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  porytiles::drive(compileCtx);

  porytiles::PtContext decompileCtx{};
  decompileCtx.decompilerSrcPaths.primarySourcePath = parentDir;

  std::ifstream metatiles{decompileCtx.decompilerSrcPaths.primaryMetatilesBin(), std::ios::binary};
  std::ifstream attributes{decompileCtx.decompilerSrcPaths.primaryAttributesBin(), std::ios::binary};
  png::image<png::index_pixel> tilesheetPng{decompileCtx.decompilerSrcPaths.primaryTilesPng()};
  std::vector<std::shared_ptr<std::ifstream>> paletteFiles{};
  for (std::size_t index = 0; index < decompileCtx.fieldmapConfig.numPalettesTotal; index++) {
    std::ostringstream filename;
    if (index < 10) {
      filename << "0";
    }
    filename << index << ".pal";
    std::filesystem::path paletteFile = decompileCtx.decompilerSrcPaths.primaryPalettes() / filename.str();
    paletteFiles.push_back(std::make_shared<std::ifstream>(paletteFile));
  }
  // TODO : actually test anims import
  auto [importedTileset, attributesMap] = porytiles::importCompiledTileset(
      decompileCtx, metatiles, attributes, std::unordered_map<std::uint8_t, std::string>{}, tilesheetPng, paletteFiles,
      std::vector<std::vector<porytiles::AnimationPng<png::index_pixel>>>{});
  metatiles.close();
  attributes.close();
  std::for_each(paletteFiles.begin(), paletteFiles.end(),
                [](std::shared_ptr<std::ifstream> stream) { stream->close(); });

  CHECK((compileCtx.compilerContext.resultTileset)->tiles.size() == importedTileset.tiles.size());
  CHECK((compileCtx.compilerContext.resultTileset)->tiles == importedTileset.tiles);

  CHECK((compileCtx.compilerContext.resultTileset)->assignments.size() == importedTileset.assignments.size());
  for (std::size_t assignmentIndex = 0; assignmentIndex < importedTileset.assignments.size(); assignmentIndex++) {
    const porytiles::Assignment &expectedAssignment =
        (compileCtx.compilerContext.resultTileset)->assignments.at(assignmentIndex);
    const porytiles::Assignment &actualAssignment = importedTileset.assignments.at(assignmentIndex);
    CHECK(expectedAssignment.tileIndex == actualAssignment.tileIndex);
    CHECK(expectedAssignment.hFlip == actualAssignment.hFlip);
    CHECK(expectedAssignment.vFlip == actualAssignment.vFlip);
    CHECK(expectedAssignment.paletteIndex == actualAssignment.paletteIndex);
    CHECK(expectedAssignment.attributes.metatileBehavior == actualAssignment.attributes.metatileBehavior);
    CHECK(expectedAssignment.attributes.layerType == actualAssignment.attributes.layerType);
  }

  for (std::size_t palIndex = 0; palIndex < (compileCtx.compilerContext.resultTileset)->palettes.size(); palIndex++) {
    auto origPal = (compileCtx.compilerContext.resultTileset)->palettes.at(palIndex).colors;
    auto decompPal = importedTileset.palettes.at(palIndex).colors;
    for (std::size_t colorIndex = 0; colorIndex < 16; colorIndex++) {
      CHECK(origPal.at(colorIndex) == decompPal.at(colorIndex));
    }
  }

  // TODO : test impl check attributes map

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("importCompiledTileset should import a dual-layer pokefirered tileset correctly")
{
  // TODO : test impl importCompiledTileset should import a dual-layer pokefirered tileset correctly
}
