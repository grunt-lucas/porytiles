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
                      const std::vector<GBAPalette> &palettes, const std::filesystem::path &animsPath)
{
  for (const auto &compiledAnim : compiledAnims) {
    std::filesystem::path animPath = animsPath / compiledAnim.animName;
    std::filesystem::create_directories(animPath);
    const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH * compiledAnim.keyFrame().tiles.size();
    const std::size_t imageHeight = porytiles::TILE_SIDE_LENGTH;
    std::vector<png::image<png::index_pixel>> outFrames{};
    for (std::size_t frameIndex = 0; frameIndex < compiledAnim.frames.size(); frameIndex++) {
      outFrames.emplace_back(static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight));
    }
    emitAnim(ctx, outFrames, compiledAnim, palettes);
    // Index starts at 1 here so we don't actually save a key.png compiled file, not necessary
    for (std::size_t frameIndex = 1; frameIndex < compiledAnim.frames.size(); frameIndex++) {
      auto &frame = outFrames.at(frameIndex);
      std::filesystem::path framePngPath = animPath / compiledAnim.frames.at(frameIndex).frameName;
      frame.write(framePngPath);
    }
  }
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
  std::vector<std::vector<AnimationPng<png::rgba_pixel>>> animations{};
  for (const auto &animDir : animationDirectories) {
    if (!std::filesystem::is_directory(animDir)) {
      pt_logln(ctx, stderr, "skipping regular file: {}", animDir.string());
      continue;
    }

    // collate all possible animation frame files
    pt_logln(ctx, stderr, "found animation: {}", animDir.string());
    std::unordered_map<std::size_t, std::filesystem::path> frames{};
    std::filesystem::path keyFrameFile = animDir / "key.png";
    if (!std::filesystem::exists(keyFrameFile) || !std::filesystem::is_regular_file(keyFrameFile)) {
      fatalerror_missingKeyFrameFile(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, animDir.filename().string());
    }
    frames.insert(std::pair{0, keyFrameFile});
    pt_logln(ctx, stderr, "found key frame file: {}, index=0", keyFrameFile.string());
    for (const auto &frameFile : std::filesystem::directory_iterator(animDir)) {
      std::string fileName = frameFile.path().filename().string();
      std::string extension = frameFile.path().extension().string();
      if (!std::regex_match(fileName, std::regex("^[0-9][0-9]\\.png$"))) {
        if (fileName != "key.png") {
          pt_logln(ctx, stderr, "skipping file: {}", frameFile.path().string());
        }
        continue;
      }
      std::size_t index = std::stoi(fileName, 0, 10) + 1;
      frames.insert(std::pair{index, frameFile.path()});
      pt_logln(ctx, stderr, "found frame file: {}, index={}", frameFile.path().string(), index);
    }

    std::vector<AnimationPng<png::rgba_pixel>> framePngs{};
    if (frames.size() == 1) {
      fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                                              animDir.filename().string(), 0);
    }
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (!frames.contains(i)) {
        fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                                                animDir.filename().string(), i - 1);
      }

      try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> png{frames.at(i)};
        AnimationPng<png::rgba_pixel> animPng{png, animDir.filename().string(), frames.at(i).filename().string()};
        framePngs.push_back(animPng);
      }
      catch (const std::exception &exception) {
        error_animFrameWasNotAPng(ctx.err, animDir.filename().string(), frames.at(i).filename().string());
      }
    }

    animations.push_back(framePngs);
  }
  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "found anim frame that was not a png");
  }

  importAnimTiles(ctx, animations, decompTiles);
}

static void driveCompileFreestanding(PtContext &ctx) {}

static void driveCompile(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if (!std::filesystem::exists(ctx.inputPaths.bottomSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.inputPaths.bottomSecondaryTilesheetPath().string()));
    }
    // TODO : throw if ctx.inputPaths.secondaryInputPath DNE or is not a directory
    if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.inputPaths.bottomSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::exists(ctx.inputPaths.middleSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.inputPaths.middleSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.middleSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.inputPaths.middleSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::exists(ctx.inputPaths.topSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.inputPaths.topSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.inputPaths.topSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.inputPaths.topSecondaryTilesheetPath().string()));
    }
  }
  // TODO : throw if ctx.inputPaths.primaryInputPath DNE or is not a directory
  if (!std::filesystem::exists(ctx.inputPaths.bottomPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.inputPaths.bottomPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.bottomPrimaryTilesheetPath())) {
    fatalerror(
        ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.inputPaths.bottomPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::exists(ctx.inputPaths.middlePrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.inputPaths.middlePrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.middlePrimaryTilesheetPath())) {
    fatalerror(
        ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.inputPaths.middlePrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::exists(ctx.inputPaths.topPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.inputPaths.topPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.inputPaths.topPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{}: exists but was not a regular file", ctx.inputPaths.topPrimaryTilesheetPath().string()));
  }

  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.inputPaths.bottomSecondaryTilesheetPath().string()));
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middleSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.inputPaths.middleSecondaryTilesheetPath().string()));
    }
    try {
      // We do this here so if the input is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.inputPaths.topSecondaryTilesheetPath().string()));
    }
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.inputPaths.bottomPrimaryTilesheetPath().string()));
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.middlePrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.inputPaths.middlePrimaryTilesheetPath().string()));
  }
  try {
    // We do this here so if the input is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.inputPaths.topPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.inputPaths.topPrimaryTilesheetPath().string()));
  }

  std::unique_ptr<CompiledTileset> compiledTiles;
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.inputPaths.primaryInputPath);
    png::image<png::rgba_pixel> bottomPrimaryPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePrimaryPng{ctx.inputPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPrimaryPng{ctx.inputPaths.topPrimaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    DecompiledTileset decompiledPrimaryTiles =
        importLayeredTilesFromPngs(ctx, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
    importAnimations(ctx, decompiledPrimaryTiles, ctx.inputPaths.primaryAnimPath());
    auto compiledPrimaryTiles = compile(ctx, decompiledPrimaryTiles);

    pt_logln(ctx, stderr, "importing secondary tiles from {}", ctx.inputPaths.secondaryInputPath);
    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middleSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topSecondaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(ctx, bottomPng, middlePng, topPng);
    importAnimations(ctx, decompiledTiles, ctx.inputPaths.secondaryAnimPath());
    ctx.compilerContext.pairedPrimaryTiles = std::move(compiledPrimaryTiles);
    compiledTiles = compile(ctx, decompiledTiles);
  }
  else {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.inputPaths.primaryInputPath);
    png::image<png::rgba_pixel> bottomPng{ctx.inputPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.inputPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.inputPaths.topPrimaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
    DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(ctx, bottomPng, middlePng, topPng);
    importAnimations(ctx, decompiledTiles, ctx.inputPaths.primaryAnimPath());
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
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", tilesetPath.string()));
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a directory", palettesDir.string()));
  }
  if (std::filesystem::exists(animsPath) && !std::filesystem::is_directory(animsPath)) {
    fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a directory", animsDir.string()));
  }
  std::filesystem::create_directories(palettesPath);
  std::filesystem::create_directories(animsPath);

  emitPalettes(ctx, *compiledTiles, palettesPath);
  emitTilesPng(ctx, *compiledTiles, tilesetPath);
  emitAnims(ctx, compiledTiles->anims, compiledTiles->palettes, animsPath);

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
    driveCompileFreestanding(ctx);
    break;
  default:
    internalerror("driver::drive unknown subcommand setting");
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

  // TODO : test impl check pal files

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

  // TODO : test impl check pal files

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

TEST_CASE("drive should emit all expected files for anim_metatiles_2 primary set")
{
  // TODO : test impl drive should emit all expected files for anim_metatiles_2 primary set
}

TEST_CASE("drive should emit all expected files for anim_metatiles_2 secondary set")
{
  // TODO : test impl drive should emit all expected files for anim_metatiles_2 secondary set
}
