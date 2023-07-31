#include "driver.h"

#include <cstdio>
#include <doctest.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <png.hpp>

#include "compiler.h"
#include "emitter.h"
#include "importer.h"
#include "ptcontext.h"
#include "ptexception.h"
#include "tmpfiles.h"

namespace porytiles {

static void emitPalettes(PtContext &ctx, const CompiledTileset &compiledTiles,
                         const std::filesystem::path &palettesPath)
{
  for (std::size_t i = 0; i < ctx.fieldmapConfig.numPalettesTotal; i++) {
    std::string fileName = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
    fileName += ".pal";
    std::filesystem::path paletteFile = palettesPath / fileName;
    std::ofstream outPal{paletteFile.string()};
    if (i < compiledTiles.palettes.size()) {
      emitPalette(ctx, outPal, compiledTiles.palettes.at(i));
    }
    else {
      emitZeroedPalette(ctx, outPal);
    }
    outPal.close();
  }
}

static void emitTilesPng(PtContext &ctx, const CompiledTileset &compiledTiles, const std::filesystem::path &tilesetPath)
{
  const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
  const std::size_t imageHeight =
      porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES) + 1);
  png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight)};

  emitTilesPng(ctx, tilesPng, compiledTiles);
  tilesPng.write(tilesetPath);
}

// static void driveCompileRaw(PtContext &ctx)
// {
//   if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
//       throw PtException{ctx.output.path + ": exists but is not a directory"};
//   }
//   if (config.secondary) {
//       if (!std::filesystem::exists(config.rawSecondaryTilesheetPath)) {
//       throw PtException{config.rawSecondaryTilesheetPath + ": file does not exist"};
//       }
//       if (!std::filesystem::is_regular_file(config.rawSecondaryTilesheetPath)) {
//           throw PtException{config.rawSecondaryTilesheetPath + ": exists but was not a regular file"};
//       }
//   }
//   if (!std::filesystem::exists(config.rawPrimaryTilesheetPath)) {
//       throw PtException{config.rawPrimaryTilesheetPath + ": file does not exist"};
//   }
//   if (!std::filesystem::is_regular_file(config.rawPrimaryTilesheetPath)) {
//       throw PtException{config.rawPrimaryTilesheetPath + ": exists but was not a regular file"};
//   }

//   if (config.secondary) {
//       try {
//       // We do this here so if the input is not a PNG, we can catch and give a better error
//           png::image<png::rgba_pixel> tilesheetPng{config.rawSecondaryTilesheetPath};
//       }
//       catch(const std::exception& exception) {
//           throw PtException{config.rawSecondaryTilesheetPath + " is not a valid PNG file"};
//       }
//   }
//   try {
//       // We do this here so if the input is not a PNG, we can catch and give a better error
//       png::image<png::rgba_pixel> tilesheetPng{config.rawPrimaryTilesheetPath};
//   }
//   catch(const std::exception& exception) {
//       throw PtException{config.rawPrimaryTilesheetPath + " is not a valid PNG file"};
//   }

//   // confirmed image was a PNG, open it again
//   png::image<png::rgba_pixel> tilesheetPng{config.rawTilesheetPath};
//   DecompiledTileset decompiledTiles = importRawTilesFromPng(tilesheetPng);
//   porytiles::CompilerContext context{config, porytiles::CompilerMode::RAW};
//   // TODO : change this over to compile once it supports RAW mode
//   CompiledTileset compiledTiles = compilePrimary(context, decompiledTiles);

//   std::filesystem::path outputPath(ctx.output.path);
//   std::filesystem::path palettesDir("palettes");
//   std::filesystem::path tilesetFile("tiles.png");
//   std::filesystem::path tilesetPath = ctx.output.path / tilesetFile;
//   std::filesystem::path palettesPath = ctx.output.path / palettesDir;

//   if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
//       throw PtException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
//   }
//   if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
//       throw PtException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
//   }
//   std::filesystem::create_directories(palettesPath);

//   emitPalettes(config, compiledTiles, palettesPath);
//   emitTilesPng(config, compiledTiles, tilesetPath);
// }

static void driveCompile(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    throw PtException{ctx.output.path + ": exists but is not a directory"};
  }
  if (ctx.secondary) {
    if (!std::filesystem::exists(ctx.inputPaths.bottomSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(ctx.inputPaths.middleSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.middleSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(ctx.inputPaths.topSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.topSecondaryTilesheetPath)) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath + ": exists but was not a regular file"};
    }
  }
  if (!std::filesystem::exists(ctx.inputPaths.bottomPrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomPrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath + ": exists but was not a regular file"};
  }
  if (!std::filesystem::exists(ctx.inputPaths.middlePrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.middlePrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath + ": exists but was not a regular file"};
  }
  if (!std::filesystem::exists(ctx.inputPaths.topPrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.topPrimaryTilesheetPath)) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath + ": exists but was not a regular file"};
  }

  if (ctx.secondary) {
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomSecondaryTilesheetPath};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath + " is not a valid PNG file"};
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middleSecondaryTilesheetPath};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath + " is not a valid PNG file"};
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topSecondaryTilesheetPath};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath + " is not a valid PNG file"};
    }
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomPrimaryTilesheetPath};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath + " is not a valid PNG file"};
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middlePrimaryTilesheetPath};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath + " is not a valid PNG file"};
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topPrimaryTilesheetPath};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath + " is not a valid PNG file"};
  }

  std::unique_ptr<CompiledTileset> compiledTiles;
  if (ctx.secondary) {
    png::image<png::rgba_pixel> bottomPrimaryPng{ctx.inputPaths.bottomPrimaryTilesheetPath};
    png::image<png::rgba_pixel> middlePrimaryPng{ctx.inputPaths.middlePrimaryTilesheetPath};
    png::image<png::rgba_pixel> topPrimaryPng{ctx.inputPaths.topPrimaryTilesheetPath};
    DecompiledTileset decompiledPrimaryTiles =
        importLayeredTilesFromPngs(bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    auto compiledPrimaryTiles = compile(ctx, decompiledPrimaryTiles);

    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomSecondaryTilesheetPath};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middleSecondaryTilesheetPath};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topSecondaryTilesheetPath};
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
    ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
    ctx.compilerContext.pairedPrimaryTiles = std::move(compiledPrimaryTiles);
    compiledTiles = compile(ctx, decompiledTiles);
  }
  else {
    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomPrimaryTilesheetPath};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middlePrimaryTilesheetPath};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topPrimaryTilesheetPath};
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    compiledTiles = compile(ctx, decompiledTiles);
  }

  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path palettesDir("palettes");
  std::filesystem::path tilesetFile("tiles.png");
  std::filesystem::path metatilesFile("metatiles.bin");
  std::filesystem::path tilesetPath = ctx.output.path / tilesetFile;
  std::filesystem::path metatilesPath = ctx.output.path / metatilesFile;
  std::filesystem::path palettesPath = ctx.output.path / palettesDir;

  if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    throw PtException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    throw PtException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
  }
  std::filesystem::create_directories(palettesPath);

  emitPalettes(ctx, *compiledTiles, palettesPath);
  emitTilesPng(ctx, *compiledTiles, tilesetPath);

  std::ofstream outMetatiles{metatilesPath.string()};
  emitMetatilesBin(ctx, outMetatiles, *compiledTiles);
  outMetatiles.close();
}

void drive(PtContext &ctx)
{
  switch (ctx.subcommand) {
  case DECOMPILE:
    throw std::runtime_error{"TODO : support decompile command"};
    break;
  case COMPILE:
    driveCompile(ctx);
    break;
  default:
    throw std::runtime_error{"unknown subcommand setting: " + std::to_string(ctx.subcommand)};
  }
}

} // namespace porytiles

TEST_CASE("drive should emit all expected files for simple_metatiles_2 primary set")
{
  porytiles::PtContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.secondary = false;
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::COMPILE;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_primary.png"));
  ctx.inputPaths.bottomPrimaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_primary.png";
  ctx.inputPaths.middlePrimaryTilesheetPath = "res/tests/simple_metatiles_2/middle_primary.png";
  ctx.inputPaths.topPrimaryTilesheetPath = "res/tests/simple_metatiles_2/top_primary.png";

  porytiles::drive(ctx);

  // TODO : check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary_expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/primary_expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    size_t tileRow = tileIndex / actualWidthInTiles;
    size_t tileCol = tileIndex % actualHeightInTiles;
    for (size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary_expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/simple_metatiles_2/primary_expected_metatiles.bin", "r");
  if (expected == NULL) {
    FAIL("std::FILE `expected' was null");
  }
  actual = fopen((parentDir / "metatiles.bin").c_str(), "r");
  if (actual == NULL) {
    fclose(expected);
    FAIL("std::FILE `expected' was null");
  }
  fseek(expected, 0, SEEK_END);
  long expectedSize = ftell(expected);
  rewind(expected);
  fseek(actual, 0, SEEK_END);
  long actualSize = ftell(actual);
  rewind(actual);
  CHECK(expectedSize == actualSize);

  std::uint8_t expectedByte;
  std::uint8_t actualByte;
  std::size_t bytesRead;
  for (long i = 0; i < actualSize; i++) {
    bytesRead = fread(&expectedByte, 1, 1, expected);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    bytesRead = fread(&actualByte, 1, 1, actual);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    CHECK(expectedByte == actualByte);
  }

  fclose(expected);
  fclose(actual);
  std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for simple_metatiles_2 secondary set")
{
  porytiles::PtContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.secondary = true;
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::COMPILE;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_primary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_secondary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_secondary.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_secondary.png"));

  ctx.inputPaths.bottomPrimaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_primary.png";
  ctx.inputPaths.middlePrimaryTilesheetPath = "res/tests/simple_metatiles_2/middle_primary.png";
  ctx.inputPaths.topPrimaryTilesheetPath = "res/tests/simple_metatiles_2/top_primary.png";
  ctx.inputPaths.bottomSecondaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_secondary.png";
  ctx.inputPaths.middleSecondaryTilesheetPath = "res/tests/simple_metatiles_2/middle_secondary.png";
  ctx.inputPaths.topSecondaryTilesheetPath = "res/tests/simple_metatiles_2/top_secondary.png";

  porytiles::drive(ctx);

  // TODO : check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary_expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/secondary_expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    size_t tileRow = tileIndex / actualWidthInTiles;
    size_t tileCol = tileIndex % actualHeightInTiles;
    for (size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary_expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/simple_metatiles_2/secondary_expected_metatiles.bin", "r");
  if (expected == NULL) {
    FAIL("std::FILE `expected' was null");
  }
  actual = fopen((parentDir / "metatiles.bin").c_str(), "r");
  if (actual == NULL) {
    fclose(expected);
    FAIL("std::FILE `expected' was null");
  }
  fseek(expected, 0, SEEK_END);
  long expectedSize = ftell(expected);
  rewind(expected);
  fseek(actual, 0, SEEK_END);
  long actualSize = ftell(actual);
  rewind(actual);
  CHECK(expectedSize == actualSize);

  std::uint8_t expectedByte;
  std::uint8_t actualByte;
  std::size_t bytesRead;
  for (long i = 0; i < actualSize; i++) {
    bytesRead = fread(&expectedByte, 1, 1, expected);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    bytesRead = fread(&actualByte, 1, 1, actual);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    CHECK(expectedByte == actualByte);
  }

  fclose(expected);
  fclose(actual);
  std::filesystem::remove_all(parentDir);
}
