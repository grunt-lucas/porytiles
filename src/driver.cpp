#include "driver.h"

#include <cstdio>
#include <doctest.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <png.hpp>
#include <regex>
#include <sstream>
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

static void validateCompileInputs(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if (!std::filesystem::exists(ctx.compilerSrcPaths.secondarySourcePath) ||
        !std::filesystem::is_directory(ctx.compilerSrcPaths.secondarySourcePath)) {
      fatalerror_invalidSourcePath(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                   ctx.compilerSrcPaths.secondarySourcePath);
    }
    if (!std::filesystem::exists(ctx.compilerSrcPaths.bottomSecondaryTilesheet())) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.compilerSrcPaths.bottomSecondaryTilesheet().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.bottomSecondaryTilesheet())) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: exists but was not a regular file",
                             ctx.compilerSrcPaths.bottomSecondaryTilesheet().string()));
    }
    if (!std::filesystem::exists(ctx.compilerSrcPaths.middleSecondaryTilesheet())) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.compilerSrcPaths.middleSecondaryTilesheet().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.middleSecondaryTilesheet())) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: exists but was not a regular file",
                             ctx.compilerSrcPaths.middleSecondaryTilesheet().string()));
    }
    if (!std::filesystem::exists(ctx.compilerSrcPaths.topSecondaryTilesheet())) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: file does not exist", ctx.compilerSrcPaths.topSecondaryTilesheet().string()));
    }
    if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.topSecondaryTilesheet())) {
      fatalerror(
          ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
          fmt::format("{}: exists but was not a regular file", ctx.compilerSrcPaths.topSecondaryTilesheet().string()));
    }
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.primarySourcePath) ||
      !std::filesystem::is_directory(ctx.compilerSrcPaths.primarySourcePath)) {
    fatalerror_invalidSourcePath(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                 ctx.compilerSrcPaths.primarySourcePath);
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.bottomPrimaryTilesheet())) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.compilerSrcPaths.bottomPrimaryTilesheet().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.bottomPrimaryTilesheet())) {
    fatalerror(
        ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.compilerSrcPaths.bottomPrimaryTilesheet().string()));
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.middlePrimaryTilesheet())) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.compilerSrcPaths.middlePrimaryTilesheet().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.middlePrimaryTilesheet())) {
    fatalerror(
        ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.compilerSrcPaths.middlePrimaryTilesheet().string()));
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.topPrimaryTilesheet())) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.compilerSrcPaths.topPrimaryTilesheet().string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.topPrimaryTilesheet())) {
    fatalerror(
        ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        fmt::format("{}: exists but was not a regular file", ctx.compilerSrcPaths.topPrimaryTilesheet().string()));
  }

  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.bottomSecondaryTilesheet()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.bottomSecondaryTilesheet().string()));
    }
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.middleSecondaryTilesheet()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.middleSecondaryTilesheet().string()));
    }
    try {
      // We do this here so if the source is not a PNG, we can catch and give a better error
      png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.topSecondaryTilesheet()};
    }
    catch (const std::exception &exception) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.topSecondaryTilesheet().string()));
    }
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.bottomPrimaryTilesheet()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.bottomPrimaryTilesheet().string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.middlePrimaryTilesheet()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.middlePrimaryTilesheet().string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.topPrimaryTilesheet()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.compilerSrcPaths.topPrimaryTilesheet().string()));
  }
}

static void validateDecompileInputs(PtContext &ctx)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    // FEATURE : check input paths for decompile-secondary
    throw std::runtime_error{"FEATURE : support decompile-secondary"};
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.primarySourcePath) ||
      !std::filesystem::is_directory(ctx.decompilerSrcPaths.primarySourcePath)) {
    fatalerror_invalidSourcePath(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
                                 ctx.decompilerSrcPaths.primarySourcePath);
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.primaryMetatilesBin())) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.decompilerSrcPaths.primaryMetatilesBin().string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.primaryAttributesBin())) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.decompilerSrcPaths.primaryAttributesBin().string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.primaryTilesPng())) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: file does not exist", ctx.decompilerSrcPaths.primaryTilesPng().string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.primaryPalettes())) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: directory does not exist", ctx.decompilerSrcPaths.primaryPalettes().string()));
  }

  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.decompilerSrcPaths.primaryTilesPng()};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{} is not a valid PNG file", ctx.decompilerSrcPaths.primaryTilesPng().string()));
  }

  if (!std::filesystem::exists(ctx.decompilerSrcPaths.metatileBehaviors) ||
      !std::filesystem::is_regular_file(ctx.decompilerSrcPaths.metatileBehaviors)) {
    // TODO : skip this check if user did not supply a behaviors header
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: behaviors header did not exist or was not a regular file",
                           ctx.decompilerSrcPaths.metatileBehaviors));
  }
}

static void validateCompileOutputs(PtContext &ctx, std::filesystem::path &attributesPath,
                                   std::filesystem::path &tilesetPath, std::filesystem::path &metatilesPath,
                                   std::filesystem::path &palettesPath, std::filesystem::path &animsPath)
{
  if (std::filesystem::exists(attributesPath) && !std::filesystem::is_regular_file(attributesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists but is not a file", attributesPath.string()));
  }
  if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists but is not a file", tilesetPath.string()));
  }
  if (std::filesystem::exists(metatilesPath) && !std::filesystem::is_regular_file(metatilesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists but is not a file", metatilesPath.string()));
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists but is not a directory", palettesPath.string()));
  }
  if (std::filesystem::exists(animsPath) && !std::filesystem::is_directory(animsPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("'{}' exists but is not a directory", animsPath.string()));
  }

  try {
    std::filesystem::create_directories(palettesPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("could not create '{}': {}", palettesPath.string(), e.what()));
  }
  try {
    std::filesystem::create_directories(animsPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("could not create '{}': {}", animsPath.string(), e.what()));
  }
}

static void validateDecompileOutputs(PtContext &ctx, std::filesystem::path &outputPath,
                                     std::filesystem::path &attributesPath, std::filesystem::path &bottomPath,
                                     std::filesystem::path &middlePath, std::filesystem::path &topPath)
{
  if (std::filesystem::exists(attributesPath) && !std::filesystem::is_regular_file(attributesPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", attributesPath.string()));
  }
  if (std::filesystem::exists(bottomPath) && !std::filesystem::is_regular_file(bottomPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", bottomPath.string()));
  }
  if (std::filesystem::exists(middlePath) && !std::filesystem::is_regular_file(middlePath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", middlePath.string()));
  }
  if (std::filesystem::exists(topPath) && !std::filesystem::is_regular_file(topPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("'{}' exists in output directory but is not a file", topPath.string()));
  }

  if (!outputPath.empty()) {
    try {
      std::filesystem::create_directories(outputPath);
    }
    catch (const std::exception &e) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
                 fmt::format("could not create '{}': {}", outputPath.string(), e.what()));
    }
  }
}

static std::vector<std::vector<AnimationPng<png::rgba_pixel>>>
prepareDecompiledAnimsForImport(PtContext &ctx, std::filesystem::path animationPath)
{
  std::vector<std::vector<AnimationPng<png::rgba_pixel>>> animations{};

  pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
  if (!std::filesystem::exists(animationPath) || !std::filesystem::is_directory(animationPath)) {
    pt_logln(ctx, stderr, "path `{}' does not exist, skipping animations import", animationPath.string());
    return animations;
  }
  std::vector<std::filesystem::path> animationDirectories;
  std::copy(std::filesystem::directory_iterator(animationPath), std::filesystem::directory_iterator(),
            std::back_inserter(animationDirectories));
  std::sort(animationDirectories.begin(), animationDirectories.end());
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
      fatalerror_missingKeyFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                     animDir.filename().string());
    }
    frames.insert(std::pair{0, keyFrameFile});
    pt_logln(ctx, stderr, "found key frame file: {}, index=0", keyFrameFile.string());
    for (const auto &frameFile : std::filesystem::directory_iterator(animDir)) {
      std::string fileName = frameFile.path().filename().string();
      std::string extension = frameFile.path().extension().string();
      /*
       * FIXME : format is actually 0.png, not 00.png. We used 00.png so that default alphabetical
       * order would also yield the frame order. However, the frames don't actually follow this
       * format. It would be better to read and then sort, especially since the decompiler has to
       * use the 0.png format.
       * FIXME : I think this is fixed now, and we can remove the above message, confirm this
       */
      if (!std::regex_match(fileName, std::regex("^[0-9][0-9]*\\.png$"))) {
        if (fileName != "key.png") {
          pt_logln(ctx, stderr, "skipping file: {}", frameFile.path().string());
        }
        continue;
      }
      std::size_t index = std::stoi(fileName, nullptr, 10) + 1;
      frames.insert(std::pair{index, frameFile.path()});
      pt_logln(ctx, stderr, "found frame file: {}, index={}", frameFile.path().string(), index);
    }

    std::vector<AnimationPng<png::rgba_pixel>> framePngs{};
    if (frames.size() == 1) {
      fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                                              animDir.filename().string(), 0);
    }
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (!frames.contains(i)) {
        fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
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
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                   "found anim frame that was not a png");
  }

  return animations;
}

static std::vector<std::vector<AnimationPng<png::index_pixel>>>
prepareCompiledAnimsForImport(PtContext &ctx, std::filesystem::path animationPath)
{
  std::vector<std::vector<AnimationPng<png::index_pixel>>> animations{};

  pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
  if (!std::filesystem::exists(animationPath) || !std::filesystem::is_directory(animationPath)) {
    pt_logln(ctx, stderr, "path `{}' does not exist, skipping animations import", animationPath.string());
    return animations;
  }
  std::vector<std::filesystem::path> animationDirectories;
  std::copy(std::filesystem::directory_iterator(animationPath), std::filesystem::directory_iterator(),
            std::back_inserter(animationDirectories));
  std::sort(animationDirectories.begin(), animationDirectories.end());
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
      if (!std::regex_match(fileName, std::regex("^[0-9][0-9]*\\.png$"))) {
        pt_logln(ctx, stderr, "skipping file: {}", frameFile.path().string());
        continue;
      }
      std::size_t index = std::stoi(fileName, nullptr, 10) + 1;
      frames.insert(std::pair{index, frameFile.path()});
      pt_logln(ctx, stderr, "found frame file: {}, index={}", frameFile.path().string(), index);
    }

    std::vector<AnimationPng<png::index_pixel>> framePngs{};
    if (frames.size() == 0) {
      // TODO : better error
      throw std::runtime_error{"TODO : error for import decompiled anims frames.size() == 0"};
      // fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
      //                                         animDir.filename().string(), 0);
    }
    for (std::size_t i = 1; i <= frames.size(); i++) {
      if (!frames.contains(i)) {
        // TODO : better error
        throw std::runtime_error{"TODO : error for import decompiled anims !frames.contains(i)"};
        // fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
        //                                         animDir.filename().string(), i - 1);
      }

      try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::index_pixel> png{frames.at(i)};
        AnimationPng<png::index_pixel> animPng{png, animDir.filename().string(), frames.at(i).filename().string()};
        framePngs.push_back(animPng);
      }
      catch (const std::exception &exception) {
        // TODO : better error
        throw std::runtime_error{
            fmt::format("TODO : error for import decompiled anims, frame index {} was not PNG", i)};
        // error_animFrameWasNotAPng(ctx.err, animDir.filename().string(), frames.at(i).filename().string());
      }
    }

    animations.push_back(framePngs);
  }

  return animations;
}

static std::unordered_map<std::size_t, Attributes>
importDecompiledAttributes(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                           std::filesystem::path attributesCsvPath)
{
  pt_logln(ctx, stderr, "importing attributes from {}", attributesCsvPath.string());
  if (!std::filesystem::exists(attributesCsvPath) || !std::filesystem::is_regular_file(attributesCsvPath)) {
    pt_logln(ctx, stderr, "path `{}' does not exist, skipping attributes import", attributesCsvPath.string());
    warn_attributesFileNotFound(ctx.err, attributesCsvPath);
    return std::unordered_map<std::size_t, Attributes>{};
  }

  return importAttributesFromCsv(ctx, behaviorMap, attributesCsvPath.string());
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importBehaviorsHeader(PtContext &ctx, std::string behaviorHeaderPath)
{
  std::ifstream behaviorFile{behaviorHeaderPath};
  if (behaviorFile.fail()) {
    // FIXME : handle compiler vs decompiler
    fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
               fmt::format("{}: could not open for reading", behaviorHeaderPath));
  }
  auto [behaviorMap, behaviorReverseMap] = importMetatileBehaviorMaps(ctx, behaviorFile);
  behaviorFile.close();
  if (behaviorMap.size() == 0) {
    // TODO : warn user that behavior map size is 0
  }
  return std::pair{behaviorMap, behaviorReverseMap};
}

static void emitCompiledPalettes(PtContext &ctx, const CompiledTileset &compiledTiles,
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

static void emitCompiledTiles(PtContext &ctx, const CompiledTileset &compiledTiles,
                              const std::filesystem::path &tilesetPath)
{
  const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
  const std::size_t imageHeight =
      porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES));
  png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight)};

  emitTilesPng(ctx, tilesPng, compiledTiles);
  tilesPng.write(tilesetPath);
}

static void emitCompiledAnims(PtContext &ctx, const std::vector<CompiledAnimation> &compiledAnims,
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

static void driveDecompile(PtContext &ctx)
{
  validateDecompileInputs(ctx);

  /*
   * Import behavior header, if it was supplied
   */
  auto [behaviorMap, behaviorReverseMap] = importBehaviorsHeader(ctx, ctx.decompilerSrcPaths.metatileBehaviors);

  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    throw std::runtime_error{"FEATURE : support decompile-secondary"};
  }

  /*
   * Set up file stream objects
   */
  std::ifstream metatiles{ctx.decompilerSrcPaths.primaryMetatilesBin(), std::ios::binary};
  std::ifstream attributes{ctx.decompilerSrcPaths.primaryAttributesBin(), std::ios::binary};
  png::image<png::index_pixel> tilesheetPng{ctx.decompilerSrcPaths.primaryTilesPng()};
  std::vector<std::shared_ptr<std::ifstream>> paletteFiles{};
  for (std::size_t index = 0; index < ctx.fieldmapConfig.numPalettesTotal; index++) {
    std::ostringstream filename;
    if (index < 10) {
      filename << "0";
    }
    filename << index << ".pal";
    std::filesystem::path paletteFile = ctx.decompilerSrcPaths.primaryPalettes() / filename.str();
    if (!std::filesystem::exists(paletteFile)) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, ctx.decompilerConfig.mode,
                 fmt::format("{}: file does not exist", paletteFile.string()));
    }
    paletteFiles.push_back(std::make_shared<std::ifstream>(paletteFile));
  }
  auto compiledAnims = prepareCompiledAnimsForImport(ctx, ctx.decompilerSrcPaths.primaryAnims());

  /*
   * Import the compiled tileset into our data types
   */
  auto [compiled, attributesMap] =
      importCompiledTileset(ctx, metatiles, attributes, behaviorReverseMap, tilesheetPng, paletteFiles, compiledAnims);

  /*
   * Close file stream objects
   */
  metatiles.close();
  attributes.close();
  std::for_each(paletteFiles.begin(), paletteFiles.end(),
                [](std::shared_ptr<std::ifstream> stream) { stream->close(); });

  /*
   * Decompile the compiled tiles
   */
  auto decompiled = decompile(ctx, compiled);

  /*
   * Emit output
   */
  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path attributesCsv("attributes.csv");
  std::filesystem::path bottomPng("bottom.png");
  std::filesystem::path middlePng("middle.png");
  std::filesystem::path topPng("top.png");
  std::filesystem::path attributesPath = ctx.output.path / attributesCsv;
  std::filesystem::path bottomPath = ctx.output.path / bottomPng;
  std::filesystem::path middlePath = ctx.output.path / middlePng;
  std::filesystem::path topPath = ctx.output.path / topPng;

  validateDecompileOutputs(ctx, outputPath, attributesPath, bottomPath, middlePath, topPath);

  std::ofstream outAttributes{attributesPath.string()};
  std::size_t metatileCount = attributesMap.size();
  std::size_t imageHeight = std::ceil(metatileCount / 8.0) * 16;
  png::image<png::rgba_pixel> bottomPrimaryPng{128, static_cast<png::uint_32>(imageHeight)};
  png::image<png::rgba_pixel> middlePrimaryPng{128, static_cast<png::uint_32>(imageHeight)};
  png::image<png::rgba_pixel> topPrimaryPng{128, static_cast<png::uint_32>(imageHeight)};
  // FIXME : if any errors are thrown in this method, it will partially emit the attr file which is not intuitive
  porytiles::emitDecompiled(ctx, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng, outAttributes, *decompiled,
                            attributesMap, behaviorReverseMap);
  outAttributes.close();
  bottomPrimaryPng.write(bottomPath);
  middlePrimaryPng.write(middlePath);
  topPrimaryPng.write(topPath);
}

static void driveCompile(PtContext &ctx)
{
  validateCompileInputs(ctx);

  /*
   * Import behavior header. If the supplied path does not point to a valid file, bail now.
   *
   * We only read the linked behavior header file from the primary set source. It never makes sense for the secondary
   * set to have a different behavior map than the paired primary, so the user does not need to specify this header in
   * the secondary set source.
   */
  // TODO : this should handle either reading the header from the source folder, or reading it from the CLI
  std::unordered_map<std::string, std::uint8_t> behaviorMap{};
  std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};
  if (std::filesystem::exists(ctx.compilerSrcPaths.metatileBehaviors)) {
    auto [map, reverse] = importBehaviorsHeader(ctx, ctx.compilerSrcPaths.metatileBehaviors);
    behaviorMap = map;
    behaviorReverseMap = reverse;
  }
  else {
    fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
               fmt::format("{}: supplied behaviors header did not exist", ctx.compilerSrcPaths.metatileBehaviors));
  }

  std::unique_ptr<CompiledTileset> compiledTiles;
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.compilerSrcPaths.primarySourcePath);
    png::image<png::rgba_pixel> bottomPrimaryPng{ctx.compilerSrcPaths.bottomPrimaryTilesheet()};
    png::image<png::rgba_pixel> middlePrimaryPng{ctx.compilerSrcPaths.middlePrimaryTilesheet()};
    png::image<png::rgba_pixel> topPrimaryPng{ctx.compilerSrcPaths.topPrimaryTilesheet()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

    auto primaryAttributesMap = importDecompiledAttributes(ctx, behaviorMap, ctx.compilerSrcPaths.primaryAttributes());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during primary attributes import");
    }

    DecompiledTileset decompiledPrimaryTiles =
        importLayeredTilesFromPngs(ctx, primaryAttributesMap, bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
    auto primaryAnimations = prepareDecompiledAnimsForImport(ctx, ctx.compilerSrcPaths.primaryAnims());
    importAnimTiles(ctx, primaryAnimations, decompiledPrimaryTiles);
    auto partnerPrimaryTiles = compile(ctx, decompiledPrimaryTiles);

    pt_logln(ctx, stderr, "importing secondary tiles from {}", ctx.compilerSrcPaths.secondarySourcePath);
    png::image<png::rgba_pixel> bottomPng{ctx.compilerSrcPaths.bottomSecondaryTilesheet()};
    png::image<png::rgba_pixel> middlePng{ctx.compilerSrcPaths.middleSecondaryTilesheet()};
    png::image<png::rgba_pixel> topPng{ctx.compilerSrcPaths.topSecondaryTilesheet()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;

    auto secondaryAttributesMap =
        importDecompiledAttributes(ctx, behaviorMap, ctx.compilerSrcPaths.secondaryAttributes());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during secondary attributes import");
    }

    DecompiledTileset decompiledSecondaryTiles =
        importLayeredTilesFromPngs(ctx, secondaryAttributesMap, bottomPng, middlePng, topPng);
    auto secondaryAnimations = prepareDecompiledAnimsForImport(ctx, ctx.compilerSrcPaths.secondaryAnims());
    importAnimTiles(ctx, secondaryAnimations, decompiledSecondaryTiles);
    ctx.compilerContext.pairedPrimaryTileset = std::move(partnerPrimaryTiles);
    compiledTiles = compile(ctx, decompiledSecondaryTiles);
    ctx.compilerContext.resultTileset = std::move(compiledTiles);
  }
  else {
    pt_logln(ctx, stderr, "importing primary tiles from {}", ctx.compilerSrcPaths.primarySourcePath);
    png::image<png::rgba_pixel> bottomPng{ctx.compilerSrcPaths.bottomPrimaryTilesheet()};
    png::image<png::rgba_pixel> middlePng{ctx.compilerSrcPaths.middlePrimaryTilesheet()};
    png::image<png::rgba_pixel> topPng{ctx.compilerSrcPaths.topPrimaryTilesheet()};
    ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

    auto primaryAttributesMap = importDecompiledAttributes(ctx, behaviorMap, ctx.compilerSrcPaths.primaryAttributes());
    if (ctx.err.errCount > 0) {
      die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(ctx.compilerConfig.mode),
                     "errors generated during primary attributes import");
    }

    DecompiledTileset decompiledTiles =
        importLayeredTilesFromPngs(ctx, primaryAttributesMap, bottomPng, middlePng, topPng);
    auto animations = prepareDecompiledAnimsForImport(ctx, ctx.compilerSrcPaths.primaryAnims());
    importAnimTiles(ctx, animations, decompiledTiles);
    compiledTiles = compile(ctx, decompiledTiles);
    ctx.compilerContext.resultTileset = std::move(compiledTiles);
  }

  /*
   * Emit output
   */
  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path palettesDir("palettes");
  std::filesystem::path animsDir("anim");
  std::filesystem::path tilesPng("tiles.png");
  std::filesystem::path metatilesBin("metatiles.bin");
  std::filesystem::path attributesBin("metatile_attributes.bin");
  std::filesystem::path tilesetPath = ctx.output.path / tilesPng;
  std::filesystem::path metatilesPath = ctx.output.path / metatilesBin;
  std::filesystem::path palettesPath = ctx.output.path / palettesDir;
  std::filesystem::path animsPath = ctx.output.path / animsDir;
  std::filesystem::path attributesPath = ctx.output.path / attributesBin;

  validateCompileOutputs(ctx, attributesPath, tilesetPath, metatilesPath, palettesPath, animsPath);

  emitCompiledPalettes(ctx, *(ctx.compilerContext.resultTileset), palettesPath);
  emitCompiledTiles(ctx, *(ctx.compilerContext.resultTileset), tilesetPath);
  emitCompiledAnims(ctx, (ctx.compilerContext.resultTileset)->anims, (ctx.compilerContext.resultTileset)->palettes,
                    animsPath);

  std::ofstream outMetatiles{metatilesPath.string()};
  emitMetatilesBin(ctx, outMetatiles, *(ctx.compilerContext.resultTileset));
  outMetatiles.close();

  std::ofstream outAttributes{attributesPath.string()};
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
  ctx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists("res/tests/metatile_behaviors.h"));
  ctx.compilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

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

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anim/water/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/primary/expected_anim/water/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_white/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_white/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_white/02.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/water/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/water/01.png"));

  png::image<png::index_pixel> expected_flower_white_00{
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"};
  png::image<png::index_pixel> actual_flower_white_00{parentDir / "anim/flower_white/00.png"};
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
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"};
  png::image<png::index_pixel> actual_flower_white_01{parentDir / "anim/flower_white/01.png"};
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
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"};
  png::image<png::index_pixel> actual_flower_white_02{parentDir / "anim/flower_white/02.png"};
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
  png::image<png::index_pixel> expected_water_00{"res/tests/anim_metatiles_2/primary/expected_anim/water/00.png"};
  png::image<png::index_pixel> actual_water_00{parentDir / "anim/water/00.png"};
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
  png::image<png::index_pixel> expected_water_01{"res/tests/anim_metatiles_2/primary/expected_anim/water/01.png"};
  png::image<png::index_pixel> actual_water_01{parentDir / "anim/water/01.png"};
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
  ctx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary"));
  ctx.compilerSrcPaths.secondarySourcePath = "res/tests/anim_metatiles_2/secondary";
  REQUIRE(std::filesystem::exists("res/tests/metatile_behaviors.h"));
  ctx.compilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

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

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_red/00.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_red/01.png"));
  REQUIRE(std::filesystem::exists(parentDir / "anim/flower_red/02.png"));

  png::image<png::index_pixel> expected_flower_red_00{
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"};
  png::image<png::index_pixel> actual_flower_red_00{parentDir / "anim/flower_red/00.png"};
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
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"};
  png::image<png::index_pixel> actual_flower_red_01{parentDir / "anim/flower_red/01.png"};
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
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"};
  png::image<png::index_pixel> actual_flower_red_02{parentDir / "anim/flower_red/02.png"};
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
