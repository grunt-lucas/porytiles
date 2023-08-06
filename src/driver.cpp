#include "driver.h"

#include <cstdio>
#include <doctest.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <png.hpp>
#include <regex>
#include <unordered_map>

#include "compiler.h"
#include "emitter.h"
#include "importer.h"
#include "logger.h"
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

static void emitAnims(PtContext &ctx, const std::vector<CompiledAnimation> &compiledAnims,
                      const std::filesystem::path &animsPath)
{
  // TODO : impl
}

static void importAnimations(PtContext &ctx, DecompiledTileset &decompTiles, std::filesystem::path animationPath)
{
  pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
  if (!std::filesystem::exists(animationPath) || !std::filesystem::is_directory(animationPath)) {
    pt_logln(ctx, stderr, "path `{}' does not exist, skipping anims import", animationPath.string());
    return;
  }
  std::vector<std::filesystem::path> animationDirectories;
  std::copy(std::filesystem::directory_iterator(animationPath), std::filesystem::directory_iterator(),
            std::back_inserter(animationDirectories));
  std::sort(animationDirectories.begin(), animationDirectories.end());
  std::vector<std::vector<NamedRgbaPng>> animations{};
  for (const auto &animDir : animationDirectories) {
    if (!std::filesystem::is_directory(animDir)) {
      pt_logln(ctx, stderr, "skipping regular file: {}", animDir.string());
      continue;
    }

    // collate all possible animation frame files
    pt_logln(ctx, stderr, "found animation: {}", animDir.string());
    std::unordered_map<std::size_t, std::filesystem::path> frames{};
    for (const auto &frameFile : std::filesystem::directory_iterator(animDir)) {
      std::string fileName = frameFile.path().filename().string();
      std::string extension = frameFile.path().extension().string();
      if (!std::regex_match(fileName, std::regex("^[0-9][0-9]\\.png$"))) {
        pt_logln(ctx, stderr, "skipping invalid anim frame file: {}", frameFile.path().string());
        continue;
      }
      std::size_t index = std::stoi(fileName, 0, 10);
      frames.insert(std::pair{index, frameFile});
      pt_logln(ctx, stderr, "found frame file: {}, index={}", frameFile.path().string(), index);
    }

    std::vector<NamedRgbaPng> framePngs{};
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (!frames.contains(i)) {
        fatalerror_missingRequiredAnimFrameFile(animDir.filename().string(), i);
      }

      try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> png{frames.at(i)};
        NamedRgbaPng namedPng{png, animDir.filename().string() + "::" + frames.at(i).filename().string()};
        framePngs.push_back(namedPng);
      }
      catch (const std::exception &exception) {
        error_animFrameWasNotAPng(ctx.err, animDir.filename().string(), frames.at(i).filename().string());
      }
    }

    animations.push_back(framePngs);
  }
  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err);
  }

  importAnimTiles(animations, decompTiles);
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

static void driveCompileFreestanding(PtContext &ctx) {}

static void driveCompile(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    throw PtException{ctx.output.path + ": exists but is not a directory"};
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if (!std::filesystem::exists(ctx.inputPaths.bottomSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath().string() + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath().string() + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(ctx.inputPaths.middleSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath().string() + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.middleSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath().string() + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(ctx.inputPaths.topSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath().string() + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.topSecondaryTilesheetPath())) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath().string() + ": exists but was not a regular file"};
    }
  }
  if (!std::filesystem::exists(ctx.inputPaths.bottomPrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath().string() + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomPrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath().string() + ": exists but was not a regular file"};
  }
  if (!std::filesystem::exists(ctx.inputPaths.middlePrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath().string() + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.middlePrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath().string() + ": exists but was not a regular file"};
  }
  if (!std::filesystem::exists(ctx.inputPaths.topPrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath().string() + ": file does not exist"};
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.topPrimaryTilesheetPath())) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath().string() + ": exists but was not a regular file"};
  }

  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.bottomSecondaryTilesheetPath().string() + " is not a valid PNG file"};
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middleSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.middleSecondaryTilesheetPath().string() + " is not a valid PNG file"};
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      throw PtException{ctx.inputPaths.topSecondaryTilesheetPath().string() + " is not a valid PNG file"};
    }
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.bottomPrimaryTilesheetPath().string() + " is not a valid PNG file"};
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middlePrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.middlePrimaryTilesheetPath().string() + " is not a valid PNG file"};
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    throw PtException{ctx.inputPaths.topPrimaryTilesheetPath().string() + " is not a valid PNG file"};
  }

  std::unique_ptr<CompiledTileset> compiledTiles;
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    png::image<png::rgba_pixel> bottomPrimaryPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePrimaryPng{ctx.inputPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPrimaryPng{ctx.inputPaths.topPrimaryTilesheetPath()};
    DecompiledTileset decompiledPrimaryTiles =
        importLayeredTilesFromPngs(ctx, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
    importAnimations(ctx, decompiledPrimaryTiles, ctx.inputPaths.primaryAnimPath());
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    auto compiledPrimaryTiles = compile(ctx, decompiledPrimaryTiles);

    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middleSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topSecondaryTilesheetPath()};
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(ctx, bottomPng, middlePng, topPng);
    importAnimations(ctx, decompiledTiles, ctx.inputPaths.secondaryAnimPath());
    ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
    ctx.compilerContext.pairedPrimaryTiles = std::move(compiledPrimaryTiles);
    compiledTiles = compile(ctx, decompiledTiles);
  }
  else {
    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topPrimaryTilesheetPath()};
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(ctx, bottomPng, middlePng, topPng);
    importAnimations(ctx, decompiledTiles, ctx.inputPaths.primaryAnimPath());
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    compiledTiles = compile(ctx, decompiledTiles);
  }

  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path palettesDir("palettes");
  std::filesystem::path animsDir("anims");
  std::filesystem::path tilesetFile("tiles.png");
  std::filesystem::path metatilesFile("metatiles.bin");
  std::filesystem::path tilesetPath = ctx.output.path / tilesetFile;
  std::filesystem::path metatilesPath = ctx.output.path / metatilesFile;
  std::filesystem::path palettesPath = ctx.output.path / palettesDir;
  std::filesystem::path animsPath = ctx.output.path / animsDir;

  if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    throw PtException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    throw PtException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
  }
  if (std::filesystem::exists(animsPath) && !std::filesystem::is_directory(animsPath)) {
    throw PtException{"`" + animsDir.string() + "' exists in output directory but is not a directory"};
  }
  std::filesystem::create_directories(palettesPath);
  std::filesystem::create_directories(animsPath);

  emitPalettes(ctx, *compiledTiles, palettesPath);
  emitTilesPng(ctx, *compiledTiles, tilesetPath);
  emitAnims(ctx, compiledTiles->anims, animsPath);

  std::ofstream outMetatiles{metatilesPath.string()};
  emitMetatilesBin(ctx, outMetatiles, *compiledTiles);
  outMetatiles.close();
}

void drive(PtContext &ctx)
{
  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE:
    throw std::runtime_error{"TODO : support decompile command"};
    break;
  case Subcommand::COMPILE_PRIMARY:
  case Subcommand::COMPILE_SECONDARY:
    driveCompile(ctx);
    break;
  case Subcommand::COMPILE_FREESTANDING:
    driveCompileFreestanding(ctx);
    break;
  default:
    throw std::runtime_error{"driver::drive unknown subcommand setting"};
  }
}

} // namespace porytiles

TEST_CASE("drive should emit all expected files for simple_metatiles_2 primary set")
{
  porytiles::PtContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary"));
  ctx.inputPaths.primaryInputPath = "res/tests/simple_metatiles_2/primary";

  porytiles::drive(ctx);

  // TODO : check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary/expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/primary/expected_tiles.png"};
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

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary/expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/simple_metatiles_2/primary/expected_metatiles.bin", "r");
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
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary"));

  ctx.inputPaths.primaryInputPath = "res/tests/simple_metatiles_2/primary";
  ctx.inputPaths.secondaryInputPath = "res/tests/simple_metatiles_2/secondary";

  porytiles::drive(ctx);

  // TODO : check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary/expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/secondary/expected_tiles.png"};
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

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary/expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/simple_metatiles_2/secondary/expected_metatiles.bin", "r");
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
