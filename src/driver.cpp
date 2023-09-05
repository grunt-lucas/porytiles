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
#include "decompiler.h"
#include "emitter.h"
#include "importer.h"
#include "logger.h"
#include "ptcontext.h"
#include "ptexception.h"
#include "utilities.h"

namespace porytiles {

static void drivePaletteEmit(PtContext &ctx, const CompiledTileset &compiledTiles,
                             const std::filesystem::path &palettesPath)
{
  // TODO : move this function's functionality into emitter?
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

static void driveTilesEmit(PtContext &ctx, const CompiledTileset &compiledTiles,
                           const std::filesystem::path &tilesetPath)
{
  // TODO : move this function's functionality into emitter?
  const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
  const std::size_t imageHeight =
      porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES));
  png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight)};

  emitTilesPng(ctx, tilesPng, compiledTiles);
  tilesPng.write(tilesetPath);
}

static void driveAnimEmit(PtContext &ctx, const std::vector<CompiledAnimation> &compiledAnims,
                          const std::vector<GBAPalette> &palettes, const std::filesystem::path &animsPath)
{
  // TODO : move this function's functionality into emitter?
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

static void driveAnimsImport(PtContext &ctx, DecompiledTileset &decompTiles, std::filesystem::path animationPath)
{
  // TODO : move this function's functionality into importer?
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
      fatalerror_missingKeyFrameFile(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode, animDir.filename().string());
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
      fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                                              animDir.filename().string(), 0);
    }
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (!frames.contains(i)) {
        fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                                                animDir.filename().string(), i - 1);
      }

      try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
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
    die_errorCount(ctx.err, ctx.srcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "found anim frame that was not a png");
  }

  importAnimTiles(ctx, animations, decompTiles);
}

static std::unordered_map<std::size_t, Attributes>
buildAttributesMap(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                   std::filesystem::path attributesCsvPath)
{
  // TODO : move this function's functionality into importer?
  pt_logln(ctx, stderr, "importing attributes from {}", attributesCsvPath.string());
  if (!std::filesystem::exists(attributesCsvPath) || !std::filesystem::is_regular_file(attributesCsvPath)) {
    pt_logln(ctx, stderr, "path `{}' does not exist, skipping attributes import", attributesCsvPath.string());
    warn_attributesFileNotFound(ctx.err, attributesCsvPath);
    return std::unordered_map<std::size_t, Attributes>{};
  }

  return importAttributesFromCsv(ctx, behaviorMap, attributesCsvPath.string());
}

static void driveDecompile(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    throw std::runtime_error{"TODO : support decompile-secondary"};
  }
  if (!std::filesystem::exists(ctx.srcPaths.primarySourcePath) ||
      !std::filesystem::is_directory(ctx.srcPaths.primarySourcePath)) {
    fatalerror_invalidSourcePath(ctx.err, ctx.srcPaths, ctx.decompilerConfig.mode, ctx.srcPaths.primarySourcePath);
  }

  // TODO : actually check for existence of files, refactor importer so it receives ifstreams
  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    throw std::runtime_error{"TODO : support decompile-secondary"};
  }

  png::image<png::rgba_pixel> bottomPrimaryPng{128, 1024};
  png::image<png::rgba_pixel> middlePrimaryPng{128, 1024};
  png::image<png::rgba_pixel> topPrimaryPng{128, 1024};

  CompiledTileset compiled = importCompiledTileset(ctx, ctx.srcPaths.primarySourcePath);
  auto decompiled = decompile(ctx, compiled);
  porytiles::emitDecompiled(ctx, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng, *decompiled);

  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::create_directories(outputPath);

  bottomPrimaryPng.write(ctx.output.path + "/bottom.png");
  middlePrimaryPng.write(ctx.output.path + "/middle.png");
  topPrimaryPng.write(ctx.output.path + "/top.png");
}

static void driveCompile(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if (!std::filesystem::exists(ctx.srcPaths.secondarySourcePath) ||
        !std::filesystem::is_directory(ctx.srcPaths.secondarySourcePath)) {
      fatalerror_invalidSourcePath(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode, ctx.srcPaths.secondarySourcePath);
    }
    if (!std::filesystem::exists(ctx.srcPaths.bottomSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.srcPaths.bottomSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.srcPaths.bottomSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.srcPaths.bottomSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::exists(ctx.srcPaths.middleSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.srcPaths.middleSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.srcPaths.middleSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.srcPaths.middleSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::exists(ctx.srcPaths.topSecondaryTilesheetPath())) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.srcPaths.topSecondaryTilesheetPath().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.srcPaths.topSecondaryTilesheetPath())) {
      fatalerror(
          ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.srcPaths.topSecondaryTilesheetPath().string()));
    }
  }
  if (!std::filesystem::exists(ctx.srcPaths.primarySourcePath) ||
      !std::filesystem::is_directory(ctx.srcPaths.primarySourcePath)) {
    fatalerror_invalidSourcePath(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode, ctx.srcPaths.primarySourcePath);
  }
  if (!std::filesystem::exists(ctx.srcPaths.bottomPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.srcPaths.bottomPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.srcPaths.bottomPrimaryTilesheetPath())) {
    fatalerror(
        ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.srcPaths.bottomPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::exists(ctx.srcPaths.middlePrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.srcPaths.middlePrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.srcPaths.middlePrimaryTilesheetPath())) {
    fatalerror(
        ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.srcPaths.middlePrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::exists(ctx.srcPaths.topPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.srcPaths.topPrimaryTilesheetPath().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.srcPaths.topPrimaryTilesheetPath())) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: exists but was not a regular file", ctx.srcPaths.topPrimaryTilesheetPath().string()));
  }

  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.bottomSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.srcPaths.bottomSecondaryTilesheetPath().string()));
    }
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.middleSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.srcPaths.middleSecondaryTilesheetPath().string()));
    }
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.topSecondaryTilesheetPath()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.srcPaths.topSecondaryTilesheetPath().string()));
    }
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.bottomPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.srcPaths.bottomPrimaryTilesheetPath().string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.middlePrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.srcPaths.middlePrimaryTilesheetPath().string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.srcPaths.topPrimaryTilesheetPath()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.srcPaths.topPrimaryTilesheetPath().string()));
  }

  /*
   * We only read the linked behavior header file from the primary set source. It never makes sense for the secondary
   * set to have a different behavior map than the paired primary, so the user does not need to specify this header in
   * the secondary set source.
   */
  std::unordered_map<std::string, std::uint8_t> behaviorMap{};
  std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};
  if (std::filesystem::exists(ctx.srcPaths.primaryMetatileBehaviors())) {
    std::ifstream behaviorFile{ctx.srcPaths.primaryMetatileBehaviors()};
    if (behaviorFile.fail()) {
      fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: could not open for reading", ctx.srcPaths.primaryMetatileBehaviors().string()));
    }
    auto [map, reverse] = importMetatileBehaviorMaps(ctx, behaviorFile);
    behaviorFile.close();
    behaviorMap = map;
    behaviorReverseMap = reverse;
  }
  else {
    warn_behaviorsHeaderNotSpecified(ctx.err, ctx.srcPaths.primaryMetatileBehaviors());
  }

  std::unique_ptr<CompiledTileset> compiledTiles;
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.srcPaths.primarySourcePath);
    png::image<png::rgba_pixel> bottomPrimaryPng{ctx.srcPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePrimaryPng{ctx.srcPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPrimaryPng{ctx.srcPaths.topPrimaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

    auto primaryAttributesMap = buildAttributesMap(ctx, behaviorMap, ctx.srcPaths.primaryAttributesPath());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.srcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during primary attributes import");
    }

    DecompiledTileset decompiledPrimaryTiles =
        importLayeredTilesFromPngs(ctx, primaryAttributesMap, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
    driveAnimsImport(ctx, decompiledPrimaryTiles, ctx.srcPaths.primaryAnimPath());
    auto partnerPrimaryTiles = compile(ctx, decompiledPrimaryTiles);

    pt_logln(ctx, stderr, "importing secondary tiles from {}", ctx.srcPaths.secondarySourcePath);
    png::image<png::rgba_pixel> bottomPng{ctx.srcPaths.bottomSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.srcPaths.middleSecondaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.srcPaths.topSecondaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;

    auto secondaryAttributesMap = buildAttributesMap(ctx, behaviorMap, ctx.srcPaths.secondaryAttributesPath());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.srcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during secondary attributes import");
    }

    DecompiledTileset decompiledTiles =
        importLayeredTilesFromPngs(ctx, secondaryAttributesMap, bottomPng, middlePng, topPng);
    driveAnimsImport(ctx, decompiledTiles, ctx.srcPaths.secondaryAnimPath());
    ctx.compilerContext.pairedPrimaryTileset = std::move(partnerPrimaryTiles);
    compiledTiles = compile(ctx, decompiledTiles);
    ctx.compilerContext.resultTileset = std::move(compiledTiles);
  }
  else {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.srcPaths.primarySourcePath);
    png::image<png::rgba_pixel> bottomPng{ctx.srcPaths.bottomPrimaryTilesheetPath()};
    png::image<png::rgba_pixel> middlePng{ctx.srcPaths.middlePrimaryTilesheetPath()};
    png::image<png::rgba_pixel> topPng{ctx.srcPaths.topPrimaryTilesheetPath()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

    auto primaryAttributesMap = buildAttributesMap(ctx, behaviorMap, ctx.srcPaths.primaryAttributesPath());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.srcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during primary attributes import");
    }

    DecompiledTileset decompiledTiles =
        importLayeredTilesFromPngs(ctx, primaryAttributesMap, bottomPng, middlePng, topPng);
    driveAnimsImport(ctx, decompiledTiles, ctx.srcPaths.primaryAnimPath());
    compiledTiles = compile(ctx, decompiledTiles);
    ctx.compilerContext.resultTileset = std::move(compiledTiles);
  }

  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path palettesDir("palettes");
  std::filesystem::path animsDir("anims");
  std::filesystem::path tilesetFile("tiles.png");
  std::filesystem::path metatilesFile("metatiles.bin");
  std::filesystem::path attributesFile("metatile_attributes.bin");
  std::filesystem::path tilesetPath = ctx.output.path / tilesetFile;
  std::filesystem::path metatilesPath = ctx.output.path / metatilesFile;
  std::filesystem::path palettesPath = ctx.output.path / palettesDir;
  std::filesystem::path animsPath = ctx.output.path / animsDir;
  std::filesystem::path attribtuesPath = ctx.output.path / attributesFile;

  if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", tilesetPath.string()));
  }
  if (std::filesystem::exists(metatilesPath) && !std::filesystem::is_regular_file(metatilesPath)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", metatilesPath.string()));
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a directory", palettesDir.string()));
  }
  if (std::filesystem::exists(animsPath) && !std::filesystem::is_directory(animsPath)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a directory", animsDir.string()));
  }
  if (std::filesystem::exists(attribtuesPath) && !std::filesystem::is_regular_file(attribtuesPath)) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", attribtuesPath.string()));
  }

  try {
    std::filesystem::create_directories(palettesPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("could not create '{}': {}", palettesPath.string(), e.what()));
  }
  try {
    std::filesystem::create_directories(animsPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.srcPaths, ctx.compilerConfig.mode,
               fmt::format("could not create '{}': {}", animsPath.string(), e.what()));
  }

  drivePaletteEmit(ctx, *(ctx.compilerContext.resultTileset), palettesPath);
  driveTilesEmit(ctx, *(ctx.compilerContext.resultTileset), tilesetPath);
  driveAnimEmit(ctx, (ctx.compilerContext.resultTileset)->anims, (ctx.compilerContext.resultTileset)->palettes,
                animsPath);

  std::ofstream outMetatiles{metatilesPath.string()};
  emitMetatilesBin(ctx, outMetatiles, *(ctx.compilerContext.resultTileset));
  outMetatiles.close();

  std::ofstream outAttributes{attribtuesPath.string()};
  emitAttributes(ctx, outAttributes, behaviorReverseMap, *(ctx.compilerContext.resultTileset));
  outAttributes.close();
}

void drive(PtContext &ctx)
{
  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE_PRIMARY:
  case Subcommand::DECOMPILE_SECONDARY:
    driveDecompile(ctx);
    break;
  case Subcommand::COMPILE_PRIMARY:
  case Subcommand::COMPILE_SECONDARY:
    driveCompile(ctx);
    break;
  default:
    internalerror("driver::drive unknown subcommand setting");
  }
}

} // namespace porytiles

TEST_CASE("drive should emit all expected files for anim_metatiles_2 primary set")
{
  porytiles::PtContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.err.printErrors = false;
  ctx.compilerConfig.assignAlgorithm = porytiles::AssignAlgorithm::DEPTH_FIRST;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary"));
  ctx.srcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";

  porytiles::drive(ctx);

  // TODO : test impl check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_2/primary/expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/anim_metatiles_2/primary/expected_metatiles.bin", "r");
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

  // Check metatile_attributes.bin

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_metatile_attributes.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatile_attributes.bin"));

  expected = fopen("res/tests/anim_metatiles_2/primary/expected_metatile_attributes.bin", "r");
  if (expected == NULL) {
    FAIL("std::FILE `expected' was null");
  }
  actual = fopen((parentDir / "metatile_attributes.bin").c_str(), "r");
  if (actual == NULL) {
    fclose(expected);
    FAIL("std::FILE `expected' was null");
  }
  fseek(expected, 0, SEEK_END);
  expectedSize = ftell(expected);
  rewind(expected);
  fseek(actual, 0, SEEK_END);
  actualSize = ftell(actual);
  rewind(actual);
  CHECK(expectedSize == actualSize);

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

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anims/flower_white/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anims/flower_white/01.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anims/flower_white/02.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anims/water/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anims/water/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_white/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_white/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_white/02.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/water/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/water/01.png"));

  png::image<png::index_pixel> expected_flower_white_00{
      "res/tests/anim_metatiles_2/primary/expected_anims/flower_white/00.png"};
  png::image<png::index_pixel> actual_flower_white_00{parentDir / "anims/flower_white/00.png"};
  expectedWidthInTiles = expected_flower_white_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_white_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_white_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_white_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_white_00[pixelRow][pixelCol] == actual_flower_white_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_white_01{
      "res/tests/anim_metatiles_2/primary/expected_anims/flower_white/01.png"};
  png::image<png::index_pixel> actual_flower_white_01{parentDir / "anims/flower_white/01.png"};
  expectedWidthInTiles = expected_flower_white_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_white_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_white_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_white_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_white_01[pixelRow][pixelCol] == actual_flower_white_01[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_white_02{
      "res/tests/anim_metatiles_2/primary/expected_anims/flower_white/02.png"};
  png::image<png::index_pixel> actual_flower_white_02{parentDir / "anims/flower_white/02.png"};
  expectedWidthInTiles = expected_flower_white_02.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_white_02.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_white_02.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_white_02.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_white_02[pixelRow][pixelCol] == actual_flower_white_02[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_water_00{"res/tests/anim_metatiles_2/primary/expected_anims/water/00.png"};
  png::image<png::index_pixel> actual_water_00{parentDir / "anims/water/00.png"};
  expectedWidthInTiles = expected_water_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_water_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_water_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_water_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_water_00[pixelRow][pixelCol] == actual_water_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_water_01{"res/tests/anim_metatiles_2/primary/expected_anims/water/01.png"};
  png::image<png::index_pixel> actual_water_01{parentDir / "anims/water/01.png"};
  expectedWidthInTiles = expected_water_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_water_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_water_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_water_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_water_01[pixelRow][pixelCol] == actual_water_01[pixelRow][pixelCol]);
    }
  }

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for anim_metatiles_2 secondary set")
{
  porytiles::PtContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.err.printErrors = false;
  ctx.compilerConfig.assignAlgorithm = porytiles::AssignAlgorithm::DEPTH_FIRST;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary"));
  ctx.srcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary"));
  ctx.srcPaths.secondarySourcePath = "res/tests/anim_metatiles_2/secondary";

  porytiles::drive(ctx);

  // TODO : test impl check pal files

  // Check tiles.png

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_tiles.png"));
  REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_2/secondary/expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_metatiles.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
  std::FILE *expected;
  std::FILE *actual;

  expected = fopen("res/tests/anim_metatiles_2/secondary/expected_metatiles.bin", "r");
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

  // Check metatile_attributes.bin
  // Secondary set doesn't provide a metatile_behaviors.h or an attributes.csv, so default values are used

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_metatile_attributes.bin"));
  REQUIRE(std::filesystem::exists(parentDir / "metatile_attributes.bin"));

  expected = fopen("res/tests/anim_metatiles_2/secondary/expected_metatile_attributes.bin", "r");
  if (expected == NULL) {
    FAIL("std::FILE `expected' was null");
  }
  actual = fopen((parentDir / "metatile_attributes.bin").c_str(), "r");
  if (actual == NULL) {
    fclose(expected);
    FAIL("std::FILE `expected' was null");
  }
  fseek(expected, 0, SEEK_END);
  expectedSize = ftell(expected);
  rewind(expected);
  fseek(actual, 0, SEEK_END);
  actualSize = ftell(actual);
  rewind(actual);
  CHECK(expectedSize == actualSize);

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

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/01.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/02.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_red/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_red/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anims/flower_red/02.png"));

  png::image<png::index_pixel> expected_flower_red_00{
      "res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/00.png"};
  png::image<png::index_pixel> actual_flower_red_00{parentDir / "anims/flower_red/00.png"};
  expectedWidthInTiles = expected_flower_red_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_red_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_red_00.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_red_00.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_red_00[pixelRow][pixelCol] == actual_flower_red_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_red_01{
      "res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/01.png"};
  png::image<png::index_pixel> actual_flower_red_01{parentDir / "anims/flower_red/01.png"};
  expectedWidthInTiles = expected_flower_red_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_red_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_red_01.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_red_01.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_red_01[pixelRow][pixelCol] == actual_flower_red_01[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_red_02{
      "res/tests/anim_metatiles_2/secondary/expected_anims/flower_red/02.png"};
  png::image<png::index_pixel> actual_flower_red_02{parentDir / "anims/flower_red/02.png"};
  expectedWidthInTiles = expected_flower_red_02.get_width() / porytiles::TILE_SIDE_LENGTH;
  expectedHeightInTiles = expected_flower_red_02.get_height() / porytiles::TILE_SIDE_LENGTH;
  actualWidthInTiles = actual_flower_red_02.get_width() / porytiles::TILE_SIDE_LENGTH;
  actualHeightInTiles = actual_flower_red_02.get_height() / porytiles::TILE_SIDE_LENGTH;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
      std::size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
      CHECK(expected_flower_red_02[pixelRow][pixelCol] == actual_flower_red_02[pixelRow][pixelCol]);
    }
  }

  std::filesystem::remove_all(parentDir);
}
