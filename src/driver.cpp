#include "driver.h"

#include <cstdio>
#include <doctest.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <png.hpp>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "compiler.h"
#include "decompiler.h"
#include "emitter.h"
#include "importer.h"
#include "logger.h"
#include "porytiles_context.h"
#include "porytiles_exception.h"
#include "utilities.h"

namespace porytiles {

static void validateCompileInputs(PorytilesContext &ctx, CompilerMode compilerMode)
{
  if (!std::filesystem::exists(ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode)) ||
      !std::filesystem::is_directory(ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: source path did not exist or is not a directory",
                           ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: file did not exist",
                           ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: exists but was not a regular file",
                           ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: file did not exist",
                           ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: exists but was not a regular file",
                           ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode))) {
    fatalerror(
        ctx.err, ctx.compilerSrcPaths, compilerMode,
        fmt::format("{}: file did not exist", ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string()));
  }
  if (!std::filesystem::is_regular_file(ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode))) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: exists but was not a regular file",
                           ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string()));
  }

  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode)};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{} is not a valid PNG file",
                           ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode)};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{} is not a valid PNG file",
                           ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string()));
  }
  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode)};
  }
  catch (const std::exception &exception) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{} is not a valid PNG file",
                           ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string()));
  }
}

static void validateDecompileInputs(PorytilesContext &ctx, DecompilerMode decompilerMode)
{
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode)) ||
      !std::filesystem::is_directory(ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode))) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
               fmt::format("{}: source path did not exist or is not a directory",
                           ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.modeBasedMetatilesPath(decompilerMode))) {
    fatalerror(
        ctx.err, ctx.decompilerSrcPaths, decompilerMode,
        fmt::format("{}: file did not exist", ctx.decompilerSrcPaths.modeBasedMetatilesPath(decompilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.modeBasedAttributePath(decompilerMode))) {
    fatalerror(
        ctx.err, ctx.decompilerSrcPaths, decompilerMode,
        fmt::format("{}: file did not exist", ctx.decompilerSrcPaths.modeBasedAttributePath(decompilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode))) {
    fatalerror(
        ctx.err, ctx.decompilerSrcPaths, decompilerMode,
        fmt::format("{}: file did not exist", ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode).string()));
  }
  if (!std::filesystem::exists(ctx.decompilerSrcPaths.modeBasedPalettePath(decompilerMode))) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
               fmt::format("{}: directory did not exist",
                           ctx.decompilerSrcPaths.modeBasedPalettePath(decompilerMode).string()));
  }

  try {
    // We do this here so if the source is not a PNG, we can catch and give a better error
    png::image<png::rgba_pixel> tilesheetPng{ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode)};
  }
  catch (const std::exception &exception) {
    fatalerror(
        ctx.err, ctx.decompilerSrcPaths, decompilerMode,
        fmt::format("{} is not a valid PNG file", ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode).string()));
  }

  if (!std::filesystem::exists(ctx.decompilerSrcPaths.metatileBehaviors) ||
      !std::filesystem::is_regular_file(ctx.decompilerSrcPaths.metatileBehaviors)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, decompilerMode,
               fmt::format("{}: behaviors header did not exist or was not a regular file",
                           ctx.decompilerSrcPaths.metatileBehaviors));
  }
}

static void validateCompileOutputs(PorytilesContext &ctx, CompilerMode compilerMode,
                                   std::filesystem::path &attributesPath, std::filesystem::path &tilesetPath,
                                   std::filesystem::path &metatilesPath, std::filesystem::path &palettesPath,
                                   std::filesystem::path &animsPath)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (std::filesystem::exists(attributesPath) && !std::filesystem::is_regular_file(attributesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("`{}' exists but is not a file", attributesPath.string()));
  }
  if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("`{}' exists but is not a file", tilesetPath.string()));
  }
  if (std::filesystem::exists(metatilesPath) && !std::filesystem::is_regular_file(metatilesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("`{}' exists but is not a file", metatilesPath.string()));
  }
  if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("`{}' exists but is not a directory", palettesPath.string()));
  }
  if (std::filesystem::exists(animsPath) && !std::filesystem::is_directory(animsPath)) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("`{}' exists but is not a directory", animsPath.string()));
  }

  try {
    std::filesystem::create_directories(palettesPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("could not create `{}': {}", palettesPath.string(), e.what()));
  }
  try {
    std::filesystem::create_directories(animsPath);
  }
  catch (const std::exception &e) {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("could not create `{}': {}", animsPath.string(), e.what()));
  }
}

static void validateDecompileOutputs(PorytilesContext &ctx, DecompilerMode mode, std::filesystem::path &outputPath,
                                     std::filesystem::path &attributesPath, std::filesystem::path &bottomPath,
                                     std::filesystem::path &middlePath, std::filesystem::path &topPath)
{
  if (std::filesystem::exists(ctx.output.path) && !std::filesystem::is_directory(ctx.output.path)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               fmt::format("{}: exists but is not a directory", ctx.output.path));
  }
  if (std::filesystem::exists(attributesPath) && !std::filesystem::is_regular_file(attributesPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               fmt::format("`{}' exists in output directory but is not a file", attributesPath.string()));
  }
  if (std::filesystem::exists(bottomPath) && !std::filesystem::is_regular_file(bottomPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               fmt::format("`{}' exists in output directory but is not a file", bottomPath.string()));
  }
  if (std::filesystem::exists(middlePath) && !std::filesystem::is_regular_file(middlePath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               fmt::format("`{}' exists in output directory but is not a file", middlePath.string()));
  }
  if (std::filesystem::exists(topPath) && !std::filesystem::is_regular_file(topPath)) {
    fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
               fmt::format("`{}' exists in output directory but is not a file", topPath.string()));
  }

  if (!outputPath.empty()) {
    try {
      std::filesystem::create_directories(outputPath);
    }
    catch (const std::exception &e) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                 fmt::format("could not create `{}': {}", outputPath.string(), e.what()));
    }
  }
}

// TODO : uncomment this when we implement animation decompilation
// static std::vector<std::vector<AnimationPng<png::index_pixel>>>
// prepareCompiledAnimsForImport(PorytilesContext &ctx, std::filesystem::path animationPath)
// {
//   std::vector<std::vector<AnimationPng<png::index_pixel>>> animations{};

//   pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
//   if (!std::filesystem::exists(animationPath) || !std::filesystem::is_directory(animationPath)) {
//     pt_logln(ctx, stderr, "path `{}' did not exist, skipping animations import", animationPath.string());
//     return animations;
//   }
//   std::vector<std::filesystem::path> animationDirectories;
//   std::copy(std::filesystem::directory_iterator(animationPath), std::filesystem::directory_iterator(),
//             std::back_inserter(animationDirectories));
//   std::sort(animationDirectories.begin(), animationDirectories.end());
//   for (const auto &animDir : animationDirectories) {
//     if (!std::filesystem::is_directory(animDir)) {
//       pt_logln(ctx, stderr, "skipping regular file: {}", animDir.string());
//       continue;
//     }

//     // collate all possible animation frame files
//     pt_logln(ctx, stderr, "found animation: {}", animDir.string());
//     std::unordered_map<std::size_t, std::filesystem::path> frames{};
//     for (const auto &frameFile : std::filesystem::directory_iterator(animDir)) {
//       std::string fileName = frameFile.path().filename().string();
//       std::string extension = frameFile.path().extension().string();
//       if (!std::regex_match(fileName, std::regex("^[0-9][0-9]*\\.png$"))) {
//         pt_logln(ctx, stderr, "skipping file: {}", frameFile.path().string());
//         continue;
//       }
//       std::size_t index = std::stoi(fileName, nullptr, 10) + 1;
//       frames.insert(std::pair{index, frameFile.path()});
//       pt_logln(ctx, stderr, "found frame file: {}, index={}", frameFile.path().string(), index);
//     }

//     std::vector<AnimationPng<png::index_pixel>> framePngs{};
//     if (frames.size() == 0) {
//       // FIXME : real error message here
//       throw std::runtime_error{"TODO : error for import decompiled anims frames.size() == 0"};
//       // fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
//       //                                         animDir.filename().string(), 0);
//     }
//     for (std::size_t i = 1; i <= frames.size(); i++) {
//       if (!frames.contains(i)) {
//         // FIXME : real error message here
//         throw std::runtime_error{"TODO : error for import decompiled anims !frames.contains(i)"};
//         // fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
//         //                                         animDir.filename().string(), i - 1);
//       }

//       try {
//         // We do this here so if the source is not a PNG, we can catch and give a better error
//         png::image<png::index_pixel> png{frames.at(i)};
//         AnimationPng<png::index_pixel> animPng{png, animDir.filename().string(), frames.at(i).filename().string()};
//         framePngs.push_back(animPng);
//       }
//       catch (const std::exception &exception) {
//         // FIXME : real error message here
//         throw std::runtime_error{
//             fmt::format("TODO : error for import decompiled anims, frame index {} was not PNG", i)};
//         // error_animFrameWasNotAPng(ctx.err, animDir.filename().string(), frames.at(i).filename().string());
//       }
//     }

//     animations.push_back(framePngs);
//   }

//   return animations;
// }

static std::vector<std::vector<AnimationPng<png::rgba_pixel>>>
prepareDecompiledAnimsForImport(PorytilesContext &ctx, CompilerMode compilerMode, std::filesystem::path animationPath)
{
  std::vector<std::vector<AnimationPng<png::rgba_pixel>>> animations{};

  pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
  if (!std::filesystem::exists(animationPath) || !std::filesystem::is_directory(animationPath)) {
    pt_logln(ctx, stderr, "path `{}' did not exist, skipping animations import", animationPath.string());
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
    std::filesystem::path keyFrameFile = animDir / std::filesystem::path{"key.png"};
    if (!std::filesystem::exists(keyFrameFile) || !std::filesystem::is_regular_file(keyFrameFile)) {
      fatalerror_missingKeyFrameFile(ctx.err, ctx.compilerSrcPaths, compilerMode, animDir.filename().string());
    }
    frames.insert(std::pair{0, keyFrameFile});
    pt_logln(ctx, stderr, "found key frame file: {}, index=0", keyFrameFile.string());
    for (const auto &frameFile : std::filesystem::directory_iterator(animDir)) {
      std::string fileName = frameFile.path().filename().string();
      std::string extension = frameFile.path().extension().string();
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
      fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, compilerMode, animDir.filename().string(),
                                              0);
    }
    for (std::size_t i = 0; i < frames.size(); i++) {
      if (!frames.contains(i)) {
        fatalerror_missingRequiredAnimFrameFile(ctx.err, ctx.compilerSrcPaths, compilerMode,
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
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), "found anim frame that was not a png");
  }

  return animations;
}

static std::unordered_map<std::size_t, Attributes>
prepareDecompiledAttributesForImport(PorytilesContext &ctx, CompilerMode compilerMode,
                                     const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                                     std::filesystem::path attributesCsvPath)
{
  pt_logln(ctx, stderr, "importing attributes from {}", attributesCsvPath.string());
  if (!std::filesystem::exists(attributesCsvPath) || !std::filesystem::is_regular_file(attributesCsvPath)) {
    pt_logln(ctx, stderr, "path `{}' did not exist, skipping attributes import", attributesCsvPath.string());
    warn_attributesFileNotFound(ctx.err, attributesCsvPath);
    return std::unordered_map<std::size_t, Attributes>{};
  }

  return importAttributesFromCsv(ctx, compilerMode, behaviorMap, attributesCsvPath.string());
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImportHelper(PorytilesContext &ctx, const CompilerMode *compilerMode,
                                      const DecompilerMode *decompilerMode, std::string behaviorHeaderPath)
{
  std::ifstream behaviorFile{behaviorHeaderPath};
  if (behaviorFile.fail()) {
    if (compilerMode != nullptr) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, *compilerMode,
                 fmt::format("{}: could not open for reading", behaviorHeaderPath));
    }
    else if (decompilerMode != nullptr) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, *decompilerMode,
                 fmt::format("{}: could not open for reading", behaviorHeaderPath));
    }
    else {
      internalerror("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
    }
  }
  auto [behaviorMap, behaviorReverseMap] = std::invoke(
      [&]() -> std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>> {
        if (compilerMode != nullptr) {
          return importMetatileBehaviorHeader(ctx, *compilerMode, behaviorFile);
        }
        else if (decompilerMode != nullptr) {
          return importMetatileBehaviorHeader(ctx, *decompilerMode, behaviorFile);
        }
        else {
          internalerror("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
        }
        // unreachable, here for compiler
        throw std::runtime_error("driver::prepareBehaviorsHeaderForImportHelper reached unreachable code path");
      });
  behaviorFile.close();
  if (behaviorMap.size() == 0) {
    if (compilerMode != nullptr) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, *compilerMode,
                 fmt::format("{}: behavior header did not contain any valid mappings", behaviorHeaderPath));
    }
    else if (decompilerMode != nullptr) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, *decompilerMode,
                 fmt::format("{}: behavior header did not contain any valid mappings", behaviorHeaderPath));
    }
    else {
      internalerror("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
    }
  }
  return std::pair{behaviorMap, behaviorReverseMap};
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImport(PorytilesContext &ctx, CompilerMode compilerMode, std::string behaviorHeaderPath)
{
  return prepareBehaviorsHeaderForImportHelper(ctx, &compilerMode, nullptr, behaviorHeaderPath);
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImport(PorytilesContext &ctx, DecompilerMode decompilerMode, std::string behaviorHeaderPath)
{
  return prepareBehaviorsHeaderForImportHelper(ctx, nullptr, &decompilerMode, behaviorHeaderPath);
}

static std::vector<RGBATile> preparePalettePrimersForImport(PorytilesContext &ctx, CompilerMode compilerMode,
                                                            std::filesystem::path palettePrimersPath)
{
  std::vector<RGBATile> primerTiles{};

  pt_logln(ctx, stderr, "importing palette primers from {}", palettePrimersPath.string());
  if (!std::filesystem::exists(palettePrimersPath) || !std::filesystem::is_directory(palettePrimersPath)) {
    pt_logln(ctx, stderr, "path `{}' did not exist, skipping palette primers import", palettePrimersPath.string());
    return primerTiles;
  }

  std::vector<std::filesystem::path> primerFiles;
  std::copy(std::filesystem::directory_iterator(palettePrimersPath), std::filesystem::directory_iterator(),
            std::back_inserter(primerFiles));

  for (const auto &primerFile : primerFiles) {
    if (!std::filesystem::is_regular_file(primerFile)) {
      pt_logln(ctx, stderr, "skipping {} as it is not a regular file", primerFile.string());
      continue;
    }
    std::ifstream fileStream{primerFile};
    pt_logln(ctx, stderr, "found palette primer file {}", primerFile.string());
    // TODO : instead of throwing fatal errors in this function, throw regular errors so we can fail later
    RGBATile primerTile = importPalettePrimer(ctx, compilerMode, fileStream);
    primerTile.primer = primerFile.filename().string();
    primerTiles.push_back(primerTile);
    fileStream.close();
  }

  return primerTiles;
}

static void driveEmitCompiledPalettes(PorytilesContext &ctx, const CompiledTileset &compiledTiles,
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

static void driveEmitCompiledTiles(PorytilesContext &ctx, const CompiledTileset &compiledTiles,
                                   const std::filesystem::path &tilesetPath)
{
  const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH_PIX * porytiles::TILES_PNG_WIDTH_IN_TILES;
  const std::size_t imageHeight =
      porytiles::TILE_SIDE_LENGTH_PIX * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES));
  png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight)};

  emitTilesPng(ctx, tilesPng, compiledTiles);
  tilesPng.write(tilesetPath);
}

static void driveEmitCompiledAnims(PorytilesContext &ctx, const std::vector<CompiledAnimation> &compiledAnims,
                                   const std::vector<GBAPalette> &palettes, const std::filesystem::path &animsPath)
{
  for (const auto &compiledAnim : compiledAnims) {
    std::filesystem::path animPath = animsPath / compiledAnim.animName;
    std::filesystem::create_directories(animPath);
    const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH_PIX * compiledAnim.keyFrame().tiles.size();
    const std::size_t imageHeight = porytiles::TILE_SIDE_LENGTH_PIX;
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

static void driveEmitAssignCache(PorytilesContext &ctx, CompilerMode compilerMode,
                                 const std::filesystem::path &assignCfgPath)
{
  std::ofstream outAssignCache{assignCfgPath.string()};
  if (outAssignCache.good()) {
    emitAssignCache(ctx, compilerMode, outAssignCache);
  }
  else {
    fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
               fmt::format("{}: cache write failed, please make sure the file is writable", assignCfgPath.string()));
  }
  outAssignCache.close();
}

static void driveEmitCompiledTileset(PorytilesContext &ctx, CompilerMode compilerMode, const CompiledTileset &tileset,
                                     const std::unordered_map<size_t, Attributes> &attributesMap,
                                     const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap)
{
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

  validateCompileOutputs(ctx, compilerMode, attributesPath, tilesetPath, metatilesPath, palettesPath, animsPath);

  driveEmitCompiledPalettes(ctx, tileset, palettesPath);
  driveEmitCompiledTiles(ctx, tileset, tilesetPath);
  driveEmitCompiledAnims(ctx, tileset.anims, tileset.palettes, animsPath);

  if (!ctx.output.disableMetatileGeneration) {
    std::ofstream outMetatiles{metatilesPath.string()};
    emitMetatilesBin(ctx, outMetatiles, tileset);
    outMetatiles.close();
  }

  if (!ctx.output.disableAttributeGeneration) {
    std::ofstream outAttributes{attributesPath.string()};
    emitAttributes(ctx, outAttributes, behaviorReverseMap, tileset);
    outAttributes.close();
  }
}

static void driveEmitDecompiledTileset(PorytilesContext &ctx, DecompilerMode mode, const DecompiledTileset &tileset,
                                       const std::unordered_map<size_t, Attributes> &attributesMap,
                                       const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap)
{
  std::filesystem::path outputPath(ctx.output.path);
  std::filesystem::path attributesCsvPath("attributes.csv");
  std::filesystem::path bottomPngPath("bottom.png");
  std::filesystem::path middlePngPath("middle.png");
  std::filesystem::path topPngPath("top.png");
  std::filesystem::path attributesPath = ctx.output.path / attributesCsvPath;
  std::filesystem::path bottomPath = ctx.output.path / bottomPngPath;
  std::filesystem::path middlePath = ctx.output.path / middlePngPath;
  std::filesystem::path topPath = ctx.output.path / topPngPath;

  validateDecompileOutputs(ctx, mode, outputPath, attributesPath, bottomPath, middlePath, topPath);

  std::ostringstream outAttributesContent{};
  std::size_t metatileCount = attributesMap.size();
  std::size_t imageHeight = std::ceil(metatileCount / 8.0) * 16;
  png::image<png::rgba_pixel> bottomPng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
  png::image<png::rgba_pixel> middlePng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
  png::image<png::rgba_pixel> topPng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
  porytiles::emitDecompiled(ctx, mode, bottomPng, middlePng, topPng, outAttributesContent, tileset, attributesMap,
                            behaviorReverseMap);

  std::ofstream outAttributes{attributesPath.string()};
  outAttributes << outAttributesContent.str();
  outAttributes.close();
  bottomPng.write(bottomPath);
  middlePng.write(middlePath);
  topPng.write(topPath);
}

static std::pair<CompiledTileset, std::unordered_map<size_t, Attributes>>
driveCompiledTilesetImport(PorytilesContext &ctx, DecompilerMode mode,
                           std::unordered_map<std::string, uint8_t> &behaviorMap,
                           std::unordered_map<uint8_t, std::string> &behaviorReverseMap)
{
  pt_logln(ctx, stderr, "importing {} compiled tileset from {}", decompilerModeString(mode),
           ctx.decompilerSrcPaths.primarySourcePath);

  /*
   * Set up file stream objects
   */
  std::ifstream metatilesIfStream{ctx.decompilerSrcPaths.modeBasedMetatilesPath(mode), std::ios::binary};
  std::ifstream attributesIfStream{ctx.decompilerSrcPaths.modeBasedAttributePath(mode), std::ios::binary};
  png::image<png::index_pixel> tilesheetPng{ctx.decompilerSrcPaths.modeBasedTilesPath(mode)};
  std::vector<std::unique_ptr<std::ifstream>> paletteFiles{};
  for (std::size_t index = 0; index < ctx.fieldmapConfig.numPalettesTotal; index++) {
    std::ostringstream filename;
    if (index < 10) {
      filename << "0";
    }
    filename << index << ".pal";
    std::filesystem::path paletteFile = ctx.decompilerSrcPaths.modeBasedPalettePath(mode) / filename.str();
    if (!std::filesystem::exists(paletteFile)) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode, fmt::format("{}: file did not exist", paletteFile.string()));
    }
    paletteFiles.push_back(std::make_unique<std::ifstream>(paletteFile));
  }
  // TODO : bring this back to implement anim decompilation
  // auto compiledAnims = prepareCompiledAnimsForImport(ctx, ctx.decompilerSrcPaths.modeBasedAnimPath(mode));

  /*
   * Import the compiled tileset into our data types
   */
  // TODO : last param is empty atm, replace it with imported compiledAnims
  auto [compiledTileset, attributesMap] = importCompiledTileset(ctx, mode, metatilesIfStream, attributesIfStream,
                                                                behaviorReverseMap, tilesheetPng, paletteFiles, {});

  /*
   * Close file stream objects
   */
  metatilesIfStream.close();
  attributesIfStream.close();
  std::for_each(paletteFiles.begin(), paletteFiles.end(),
                [](const std::unique_ptr<std::ifstream> &stream) { stream->close(); });

  return std::pair{compiledTileset, attributesMap};
}

static std::pair<std::unique_ptr<CompiledTileset>, std::unordered_map<size_t, Attributes>>
driveCompileTileset(PorytilesContext &ctx, CompilerMode compilerMode, CompilerMode parentCompilerMode,
                    std::unordered_map<std::string, uint8_t> &behaviorMap,
                    std::unordered_map<uint8_t, std::string> &behaviorReverseMap)
{
  auto compiledTileset = std::make_unique<CompiledTileset>();

  pt_logln(ctx, stderr, "importing {} tiles from {}", compilerModeString(compilerMode),
           ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string());
  png::image<png::rgba_pixel> bottomPng{ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode)};
  png::image<png::rgba_pixel> middlePng{ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode)};
  png::image<png::rgba_pixel> topPng{ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode)};

  auto attributesMap = prepareDecompiledAttributesForImport(ctx, compilerMode, behaviorMap,
                                                            ctx.compilerSrcPaths.modeBasedAttributePath(compilerMode));
  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                   fmt::format("errors generated during {} attributes import", compilerModeString(compilerMode)));
  }

  DecompiledTileset decompiledTiles =
      importLayeredTilesFromPngs(ctx, compilerMode, attributesMap, bottomPng, middlePng, topPng);
  auto animations =
      prepareDecompiledAnimsForImport(ctx, compilerMode, ctx.compilerSrcPaths.modeBasedAnimPath(compilerMode));
  importAnimTiles(ctx, compilerMode, animations, decompiledTiles);
  std::vector<RGBATile> palettePrimers =
      preparePalettePrimersForImport(ctx, compilerMode, ctx.compilerSrcPaths.modeBasedPalettePrimerPath(compilerMode));
  if (std::filesystem::exists(ctx.compilerSrcPaths.modeBasedAssignCachePath(compilerMode))) {
    std::ifstream assignCacheFile{ctx.compilerSrcPaths.modeBasedAssignCachePath(compilerMode)};
    if (assignCacheFile.fail()) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, compilerMode,
                 fmt::format("{}: could not open for reading",
                             ctx.compilerSrcPaths.modeBasedAssignCachePath(compilerMode).c_str()));
    }
    importAssignmentCache(ctx, compilerMode, parentCompilerMode, assignCacheFile);
    assignCacheFile.close();
  }
  compiledTileset = compile(ctx, compilerMode, decompiledTiles, palettePrimers);
  if (ctx.compilerConfig.cacheAssign) {
    driveEmitAssignCache(ctx, compilerMode, ctx.compilerSrcPaths.modeBasedAssignCachePath(compilerMode));
  }

  return std::pair{std::move(compiledTileset), attributesMap};
}

static std::pair<std::unique_ptr<DecompiledTileset>, std::unordered_map<size_t, Attributes>>
driveDecompileTileset(PorytilesContext &ctx, DecompilerMode mode, std::unordered_map<std::string, uint8_t> &behaviorMap,
                      std::unordered_map<uint8_t, std::string> &behaviorReverseMap)
{
  auto decompiled = std::make_unique<DecompiledTileset>();

  /*
   * Import the compiled tileset and attributes map from the given input paths.
   */
  auto [compiledTileset, attributesMap] = driveCompiledTilesetImport(ctx, mode, behaviorMap, behaviorReverseMap);

  /*
   * Decompile the compiled tiles
   */
  decompiled = decompile(ctx, mode, compiledTileset, attributesMap);

  return std::pair{std::move(decompiled), attributesMap};
}

static void driveDecompilePrimary(PorytilesContext &ctx)
{
  validateDecompileInputs(ctx, DecompilerMode::PRIMARY);

  /*
   * Import behavior header, if it was supplied
   */
  auto [behaviorMap, behaviorReverseMap] =
      prepareBehaviorsHeaderForImport(ctx, DecompilerMode::PRIMARY, ctx.decompilerSrcPaths.metatileBehaviors);

  /*
   * Decompile the compiled primary tiles
   */
  auto [decompiled, attributesMap] =
      driveDecompileTileset(ctx, DecompilerMode::PRIMARY, behaviorMap, behaviorReverseMap);

  /*
   * Emit the decompiled primary tileset.
   */
  driveEmitDecompiledTileset(ctx, DecompilerMode::PRIMARY, *decompiled, attributesMap, behaviorReverseMap);
}

static void driveDecompileSecondary(PorytilesContext &ctx)
{
  validateDecompileInputs(ctx, DecompilerMode::SECONDARY);
  validateDecompileInputs(ctx, DecompilerMode::PRIMARY);

  /*
   * Import behavior header, if it was supplied
   */
  auto [behaviorMap, behaviorReverseMap] =
      prepareBehaviorsHeaderForImport(ctx, DecompilerMode::SECONDARY, ctx.decompilerSrcPaths.metatileBehaviors);

  /*
   * Import the paired primary tileset.
   */
  auto [primaryCompiledTileset, primaryAttributesMap] =
      driveCompiledTilesetImport(ctx, DecompilerMode::PRIMARY, behaviorMap, behaviorReverseMap);

  /*
   * Decompile the compiled secondary tiles
   */
  ctx.decompilerContext.pairedPrimaryTileset = std::make_unique<CompiledTileset>(primaryCompiledTileset);
  auto [decompiled, attributesMap] =
      driveDecompileTileset(ctx, DecompilerMode::SECONDARY, behaviorMap, behaviorReverseMap);

  /*
   * Emit the decompiled secondary tileset.
   */
  driveEmitDecompiledTileset(ctx, DecompilerMode::SECONDARY, *decompiled, attributesMap, behaviorReverseMap);
}

static void driveCompilePrimary(PorytilesContext &ctx)
{
  /*
   * Checks that the compiler input folder contents exist as expected.
   */
  validateCompileInputs(ctx, CompilerMode::PRIMARY);

  /*
   * Import behavior header. If the supplied path does not point to a valid file, bail now.
   */
  std::unordered_map<std::string, std::uint8_t> behaviorMap{};
  std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};
  if (std::filesystem::exists(ctx.compilerSrcPaths.metatileBehaviors)) {
    auto [map, reverse] =
        prepareBehaviorsHeaderForImport(ctx, CompilerMode::PRIMARY, ctx.compilerSrcPaths.metatileBehaviors);
    behaviorMap = map;
    behaviorReverseMap = reverse;
  }
  else {
    fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY,
               fmt::format("{}: file did not exist", ctx.compilerSrcPaths.metatileBehaviors));
  }

  /*
   * Now that we have imported the behavior header, let's parse the arguments to the -default-X options if they were
   * supplied. If the user provided an integer, just use that. Otherwise, if the user provided a label string, check
   * it against the behavior header or terrain/encounter type tables and replace that string with the integral value.
   */
  // FIXME : default behavior/encounter/terrain parsing code is duped
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided a behavior label and try to parse that based on the mappings
     * from the behaviors header.
     */
    if (!behaviorMap.contains(ctx.compilerConfig.defaultBehavior)) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY,
                 fmt::format("supplied default behavior `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultBehavior, fmt::emphasis::bold)));
    }
    ctx.compilerConfig.defaultBehavior = std::to_string(behaviorMap.at(ctx.compilerConfig.defaultBehavior));
  }
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided an encounter label and try to parse that based on the
     * mappings from the encounter table.
     */
    try {
      EncounterType type = stringToEncounterType(ctx.compilerConfig.defaultEncounterType);
      ctx.compilerConfig.defaultEncounterType = std::to_string(encounterTypeValue(type));
    }
    catch (const std::exception &e1) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY,
                 fmt::format("supplied default EncounterType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultEncounterType, fmt::emphasis::bold)));
    }
  }
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided an terrain label and try to parse that based on the
     * mappings from the terrain table.
     */
    try {
      TerrainType type = stringToTerrainType(ctx.compilerConfig.defaultTerrainType);
      ctx.compilerConfig.defaultTerrainType = std::to_string(terrainTypeValue(type));
    }
    catch (const std::exception &e1) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY,
                 fmt::format("supplied default TerrainType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultTerrainType, fmt::emphasis::bold)));
    }
  }

  auto [compiledTileset, attributesMap] =
      driveCompileTileset(ctx, CompilerMode::PRIMARY, CompilerMode::PRIMARY, behaviorMap, behaviorReverseMap);

  ctx.compilerContext.resultTileset = std::move(compiledTileset);

  driveEmitCompiledTileset(ctx, CompilerMode::PRIMARY, *(ctx.compilerContext.resultTileset), attributesMap,
                           behaviorReverseMap);
}

static void driveCompileSecondary(PorytilesContext &ctx)
{
  /*
   * Checks that the compiler input folder contents exist as expected.
   */
  validateCompileInputs(ctx, CompilerMode::SECONDARY);
  validateCompileInputs(ctx, CompilerMode::PRIMARY);

  /*
   * Import behavior header. If the supplied path does not point to a valid file, bail now.
   */
  std::unordered_map<std::string, std::uint8_t> behaviorMap{};
  std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};
  if (std::filesystem::exists(ctx.compilerSrcPaths.metatileBehaviors)) {
    auto [map, reverse] =
        prepareBehaviorsHeaderForImport(ctx, CompilerMode::SECONDARY, ctx.compilerSrcPaths.metatileBehaviors);
    behaviorMap = map;
    behaviorReverseMap = reverse;
  }
  else {
    fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
               fmt::format("{}: file did not exist", ctx.compilerSrcPaths.metatileBehaviors));
  }

  /*
   * Now that we have imported the behavior header, let's parse the arguments to the -default-X options if they were
   * supplied. If the user provided an integer, just use that. Otherwise, if the user provided a label string, check
   * it against the behavior header or terrain/encounter type tables and replace that string with the integral value.
   */
  // FIXME : default behavior/encounter/terrain parsing code is duped
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided a behavior label and try to parse that based on the mappings
     * from the behaviors header.
     */
    if (!behaviorMap.contains(ctx.compilerConfig.defaultBehavior)) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
                 fmt::format("supplied default behavior `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultBehavior, fmt::emphasis::bold)));
    }
    ctx.compilerConfig.defaultBehavior = std::to_string(behaviorMap.at(ctx.compilerConfig.defaultBehavior));
  }
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided an encounter label and try to parse that based on the
     * mappings from the encounter table.
     */
    try {
      EncounterType type = stringToEncounterType(ctx.compilerConfig.defaultEncounterType);
      ctx.compilerConfig.defaultEncounterType = std::to_string(encounterTypeValue(type));
    }
    catch (const std::exception &e1) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
                 fmt::format("supplied default EncounterType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultEncounterType, fmt::emphasis::bold)));
    }
  }
  try {
    parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
  }
  catch (const std::exception &e) {
    /*
     * If the integer parse fails, assume the user provided an terrain label and try to parse that based on the
     * mappings from the terrain table.
     */
    try {
      TerrainType type = stringToTerrainType(ctx.compilerConfig.defaultTerrainType);
      ctx.compilerConfig.defaultTerrainType = std::to_string(terrainTypeValue(type));
    }
    catch (const std::exception &e1) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
                 fmt::format("supplied default TerrainType `{}' was not valid",
                             fmt::styled(ctx.compilerConfig.defaultTerrainType, fmt::emphasis::bold)));
    }
  }

  auto [compiledPairedPrimaryTileset, pairedPrimaryAttributesMap] =
      driveCompileTileset(ctx, CompilerMode::PRIMARY, CompilerMode::SECONDARY, behaviorMap, behaviorReverseMap);
  ctx.compilerContext.pairedPrimaryTileset = std::move(compiledPairedPrimaryTileset);

  auto [compiledTileset, attributesMap] =
      driveCompileTileset(ctx, CompilerMode::SECONDARY, CompilerMode::SECONDARY, behaviorMap, behaviorReverseMap);

  ctx.compilerContext.resultTileset = std::move(compiledTileset);

  driveEmitCompiledTileset(ctx, CompilerMode::SECONDARY, *(ctx.compilerContext.resultTileset), attributesMap,
                           behaviorReverseMap);
}

void drive(PorytilesContext &ctx)
{
  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE_PRIMARY:
    driveDecompilePrimary(ctx);
    break;
  case Subcommand::DECOMPILE_SECONDARY:
    driveDecompileSecondary(ctx);
    break;
  case Subcommand::COMPILE_PRIMARY:
    driveCompilePrimary(ctx);
    break;
  case Subcommand::COMPILE_SECONDARY:
    driveCompileSecondary(ctx);
    break;
  default:
    internalerror("driver::drive unknown subcommand setting");
  }
}

} // namespace porytiles

TEST_CASE("drive should emit all expected files for anim_metatiles_2 primary set")
{
  porytiles::PorytilesContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary"}));
  ctx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/metatile_behaviors.h"}));
  ctx.compilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

  porytiles::drive(ctx);

  // TODO tests : (drive should emit all expected files...) test palette files are correct

  // Check tiles.png

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_tiles.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"tiles.png"}));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_2/primary/expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / std::filesystem::path{"tiles.png"}};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin
  porytiles::doctestAssertFileBytesIdentical(
      std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_metatiles.bin"},
      parentDir / std::filesystem::path{"metatiles.bin"});

  // Check metatile_attributes.bin
  porytiles::doctestAssertFileBytesIdentical(
      std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_metatile_attributes.bin"},
      parentDir / std::filesystem::path{"metatile_attributes.bin"});

  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"}));
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"}));
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"}));
  REQUIRE(
      std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_anim/water/00.png"}));
  REQUIRE(
      std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary/expected_anim/water/01.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/00.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/01.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/02.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/water/00.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/water/01.png"}));

  png::image<png::index_pixel> expected_flower_white_00{
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"};
  png::image<png::index_pixel> actual_flower_white_00{parentDir / std::filesystem::path{"anim/flower_white/00.png"}};
  expectedWidthInTiles = expected_flower_white_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_white_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_white_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_white_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_white_00[pixelRow][pixelCol] == actual_flower_white_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_white_01{
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"};
  png::image<png::index_pixel> actual_flower_white_01{parentDir / std::filesystem::path{"anim/flower_white/01.png"}};
  expectedWidthInTiles = expected_flower_white_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_white_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_white_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_white_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_white_01[pixelRow][pixelCol] == actual_flower_white_01[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_white_02{
      "res/tests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"};
  png::image<png::index_pixel> actual_flower_white_02{parentDir / std::filesystem::path{"anim/flower_white/02.png"}};
  expectedWidthInTiles = expected_flower_white_02.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_white_02.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_white_02.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_white_02.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_white_02[pixelRow][pixelCol] == actual_flower_white_02[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_water_00{"res/tests/anim_metatiles_2/primary/expected_anim/water/00.png"};
  png::image<png::index_pixel> actual_water_00{parentDir / std::filesystem::path{"anim/water/00.png"}};
  expectedWidthInTiles = expected_water_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_water_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_water_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_water_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_water_00[pixelRow][pixelCol] == actual_water_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_water_01{"res/tests/anim_metatiles_2/primary/expected_anim/water/01.png"};
  png::image<png::index_pixel> actual_water_01{parentDir / std::filesystem::path{"anim/water/01.png"}};
  expectedWidthInTiles = expected_water_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_water_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_water_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_water_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_water_01[pixelRow][pixelCol] == actual_water_01[pixelRow][pixelCol]);
    }
  }

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for anim_metatiles_2 secondary set")
{
  porytiles::PorytilesContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/primary"}));
  ctx.compilerSrcPaths.primarySourcePath = "res/tests/anim_metatiles_2/primary";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/secondary"}));
  ctx.compilerSrcPaths.secondarySourcePath = "res/tests/anim_metatiles_2/secondary";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/metatile_behaviors.h"}));
  ctx.compilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

  porytiles::drive(ctx);

  // TODO tests : (drive should emit all expected files...) test palette files are correct

  // Check tiles.png

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_tiles.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"tiles.png"}));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_2/secondary/expected_tiles.png"};
  png::image<png::index_pixel> actualPng{parentDir / std::filesystem::path{"tiles.png"}};

  std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedWidthInTiles == actualWidthInTiles);
  CHECK(expectedHeightInTiles == actualHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
    }
  }

  // Check metatiles.bin
  porytiles::doctestAssertFileBytesIdentical(
      std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_metatiles.bin"},
      parentDir / std::filesystem::path{"metatiles.bin"});

  // Check metatile_attributes.bin
  porytiles::doctestAssertFileBytesIdentical(
      std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_metatile_attributes.bin"},
      parentDir / std::filesystem::path{"metatile_attributes.bin"});

  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"}));
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"}));
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/00.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/01.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/02.png"}));

  png::image<png::index_pixel> expected_flower_red_00{
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"};
  png::image<png::index_pixel> actual_flower_red_00{parentDir / std::filesystem::path{"anim/flower_red/00.png"}};
  expectedWidthInTiles = expected_flower_red_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_red_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_red_00.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_red_00.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_red_00[pixelRow][pixelCol] == actual_flower_red_00[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_red_01{
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"};
  png::image<png::index_pixel> actual_flower_red_01{parentDir / std::filesystem::path{"anim/flower_red/01.png"}};
  expectedWidthInTiles = expected_flower_red_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_red_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_red_01.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_red_01.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_red_01[pixelRow][pixelCol] == actual_flower_red_01[pixelRow][pixelCol]);
    }
  }
  png::image<png::index_pixel> expected_flower_red_02{
      "res/tests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"};
  png::image<png::index_pixel> actual_flower_red_02{parentDir / std::filesystem::path{"anim/flower_red/02.png"}};
  expectedWidthInTiles = expected_flower_red_02.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  expectedHeightInTiles = expected_flower_red_02.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualWidthInTiles = actual_flower_red_02.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  actualHeightInTiles = actual_flower_red_02.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualWidthInTiles;
    std::size_t tileCol = tileIndex % actualWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expected_flower_red_02[pixelRow][pixelCol] == actual_flower_red_02[pixelRow][pixelCol]);
    }
  }

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for compiled_emerald_general")
{
  porytiles::PorytilesContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::DECOMPILE_PRIMARY;
  ctx.err.printErrors = false;
  ctx.decompilerConfig.normalizeTransparency = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/compiled_emerald_general"}));
  ctx.decompilerSrcPaths.primarySourcePath = "res/tests/compiled_emerald_general";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/metatile_behaviors.h"}));
  ctx.decompilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

  porytiles::drive(ctx);

  // Check bottom.png
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/compiled_emerald_general/expected_decompiled/bottom.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"bottom.png"}));
  png::image<png::rgba_pixel> expectedBottomPng{"res/tests/compiled_emerald_general/expected_decompiled/bottom.png"};
  png::image<png::rgba_pixel> actualBottomPng{parentDir / std::filesystem::path{"bottom.png"}};

  std::size_t expectedBottomWidthInTiles = expectedBottomPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedBottomHeightInTiles = expectedBottomPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualBottomWidthInTiles = actualBottomPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualBottomHeightInTiles = actualBottomPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedBottomWidthInTiles == actualBottomWidthInTiles);
  CHECK(expectedBottomHeightInTiles == actualBottomHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualBottomWidthInTiles * actualBottomHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualBottomWidthInTiles;
    std::size_t tileCol = tileIndex % actualBottomWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedBottomPng[pixelRow][pixelCol].red == actualBottomPng[pixelRow][pixelCol].red);
      CHECK(expectedBottomPng[pixelRow][pixelCol].green == actualBottomPng[pixelRow][pixelCol].green);
      CHECK(expectedBottomPng[pixelRow][pixelCol].blue == actualBottomPng[pixelRow][pixelCol].blue);
      CHECK(expectedBottomPng[pixelRow][pixelCol].alpha == actualBottomPng[pixelRow][pixelCol].alpha);
    }
  }

  // Check middle.png
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/compiled_emerald_general/expected_decompiled/middle.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"middle.png"}));
  png::image<png::rgba_pixel> expectedMiddlePng{"res/tests/compiled_emerald_general/expected_decompiled/middle.png"};
  png::image<png::rgba_pixel> actualMiddlePng{parentDir / std::filesystem::path{"middle.png"}};

  std::size_t expectedMiddleWidthInTiles = expectedMiddlePng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedMiddleHeightInTiles = expectedMiddlePng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualMiddleWidthInTiles = actualMiddlePng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualMiddleHeightInTiles = actualMiddlePng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedMiddleWidthInTiles == actualMiddleWidthInTiles);
  CHECK(expectedMiddleHeightInTiles == actualMiddleHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualMiddleWidthInTiles * actualMiddleHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualMiddleWidthInTiles;
    std::size_t tileCol = tileIndex % actualMiddleWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].red == actualMiddlePng[pixelRow][pixelCol].red);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].green == actualMiddlePng[pixelRow][pixelCol].green);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].blue == actualMiddlePng[pixelRow][pixelCol].blue);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].alpha == actualMiddlePng[pixelRow][pixelCol].alpha);
    }
  }

  // Check top.png
  REQUIRE(
      std::filesystem::exists(std::filesystem::path{"res/tests/compiled_emerald_general/expected_decompiled/top.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"top.png"}));
  png::image<png::rgba_pixel> expectedTopPng{"res/tests/compiled_emerald_general/expected_decompiled/top.png"};
  png::image<png::rgba_pixel> actualTopPng{parentDir / std::filesystem::path{"top.png"}};

  std::size_t expectedTopWidthInTiles = expectedTopPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedTopHeightInTiles = expectedTopPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualTopWidthInTiles = actualTopPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualTopHeightInTiles = actualTopPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedTopWidthInTiles == actualTopWidthInTiles);
  CHECK(expectedTopHeightInTiles == actualTopHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualTopWidthInTiles * actualTopHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualTopWidthInTiles;
    std::size_t tileCol = tileIndex % actualTopWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedTopPng[pixelRow][pixelCol].red == actualTopPng[pixelRow][pixelCol].red);
      CHECK(expectedTopPng[pixelRow][pixelCol].green == actualTopPng[pixelRow][pixelCol].green);
      CHECK(expectedTopPng[pixelRow][pixelCol].blue == actualTopPng[pixelRow][pixelCol].blue);
      CHECK(expectedTopPng[pixelRow][pixelCol].alpha == actualTopPng[pixelRow][pixelCol].alpha);
    }
  }

  // Check attributes.csv
  porytiles::doctestAssertFileLinesIdentical(
      std::filesystem::path{"res/tests/compiled_emerald_general/expected_decompiled/attributes.csv"},
      parentDir / std::filesystem::path{"attributes.csv"});

  // TODO tests : (drive should emit all expected files) test animations once we implement anim decomp

  std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for compiled_emerald_lilycove")
{
  porytiles::PorytilesContext ctx{};
  std::filesystem::path parentDir = porytiles::createTmpdir();
  ctx.output.path = parentDir;
  ctx.subcommand = porytiles::Subcommand::DECOMPILE_SECONDARY;
  ctx.err.printErrors = false;
  ctx.decompilerConfig.normalizeTransparency = false;

  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/compiled_emerald_general"}));
  ctx.decompilerSrcPaths.primarySourcePath = "res/tests/compiled_emerald_general";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/compiled_emerald_lilycove"}));
  ctx.decompilerSrcPaths.secondarySourcePath = "res/tests/compiled_emerald_lilycove";
  REQUIRE(std::filesystem::exists(std::filesystem::path{"res/tests/metatile_behaviors.h"}));
  ctx.decompilerSrcPaths.metatileBehaviors = "res/tests/metatile_behaviors.h";

  porytiles::drive(ctx);

  // Check bottom.png
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/compiled_emerald_lilycove/expected_decompiled/bottom.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"bottom.png"}));
  png::image<png::rgba_pixel> expectedBottomPng{"res/tests/compiled_emerald_lilycove/expected_decompiled/bottom.png"};
  png::image<png::rgba_pixel> actualBottomPng{parentDir / std::filesystem::path{"bottom.png"}};

  std::size_t expectedBottomWidthInTiles = expectedBottomPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedBottomHeightInTiles = expectedBottomPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualBottomWidthInTiles = actualBottomPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualBottomHeightInTiles = actualBottomPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedBottomWidthInTiles == actualBottomWidthInTiles);
  CHECK(expectedBottomHeightInTiles == actualBottomHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualBottomWidthInTiles * actualBottomHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualBottomWidthInTiles;
    std::size_t tileCol = tileIndex % actualBottomWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedBottomPng[pixelRow][pixelCol].red == actualBottomPng[pixelRow][pixelCol].red);
      CHECK(expectedBottomPng[pixelRow][pixelCol].green == actualBottomPng[pixelRow][pixelCol].green);
      CHECK(expectedBottomPng[pixelRow][pixelCol].blue == actualBottomPng[pixelRow][pixelCol].blue);
      CHECK(expectedBottomPng[pixelRow][pixelCol].alpha == actualBottomPng[pixelRow][pixelCol].alpha);
    }
  }

  // Check middle.png
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/compiled_emerald_lilycove/expected_decompiled/middle.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"middle.png"}));
  png::image<png::rgba_pixel> expectedMiddlePng{"res/tests/compiled_emerald_lilycove/expected_decompiled/middle.png"};
  png::image<png::rgba_pixel> actualMiddlePng{parentDir / std::filesystem::path{"middle.png"}};

  std::size_t expectedMiddleWidthInTiles = expectedMiddlePng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedMiddleHeightInTiles = expectedMiddlePng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualMiddleWidthInTiles = actualMiddlePng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualMiddleHeightInTiles = actualMiddlePng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedMiddleWidthInTiles == actualMiddleWidthInTiles);
  CHECK(expectedMiddleHeightInTiles == actualMiddleHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualMiddleWidthInTiles * actualMiddleHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualMiddleWidthInTiles;
    std::size_t tileCol = tileIndex % actualMiddleWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].red == actualMiddlePng[pixelRow][pixelCol].red);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].green == actualMiddlePng[pixelRow][pixelCol].green);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].blue == actualMiddlePng[pixelRow][pixelCol].blue);
      CHECK(expectedMiddlePng[pixelRow][pixelCol].alpha == actualMiddlePng[pixelRow][pixelCol].alpha);
    }
  }

  // Check top.png
  REQUIRE(std::filesystem::exists(
      std::filesystem::path{"res/tests/compiled_emerald_lilycove/expected_decompiled/top.png"}));
  REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"top.png"}));
  png::image<png::rgba_pixel> expectedTopPng{"res/tests/compiled_emerald_lilycove/expected_decompiled/top.png"};
  png::image<png::rgba_pixel> actualTopPng{parentDir / std::filesystem::path{"top.png"}};

  std::size_t expectedTopWidthInTiles = expectedTopPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t expectedTopHeightInTiles = expectedTopPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualTopWidthInTiles = actualTopPng.get_width() / porytiles::TILE_SIDE_LENGTH_PIX;
  std::size_t actualTopHeightInTiles = actualTopPng.get_height() / porytiles::TILE_SIDE_LENGTH_PIX;

  CHECK(expectedTopWidthInTiles == actualTopWidthInTiles);
  CHECK(expectedTopHeightInTiles == actualTopHeightInTiles);

  for (std::size_t tileIndex = 0; tileIndex < actualTopWidthInTiles * actualTopHeightInTiles; tileIndex++) {
    std::size_t tileRow = tileIndex / actualTopWidthInTiles;
    std::size_t tileCol = tileIndex % actualTopWidthInTiles;
    for (std::size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
      std::size_t pixelRow =
          (tileRow * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles::TILE_SIDE_LENGTH_PIX);
      std::size_t pixelCol =
          (tileCol * porytiles::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles::TILE_SIDE_LENGTH_PIX);
      CHECK(expectedTopPng[pixelRow][pixelCol].red == actualTopPng[pixelRow][pixelCol].red);
      CHECK(expectedTopPng[pixelRow][pixelCol].green == actualTopPng[pixelRow][pixelCol].green);
      CHECK(expectedTopPng[pixelRow][pixelCol].blue == actualTopPng[pixelRow][pixelCol].blue);
      CHECK(expectedTopPng[pixelRow][pixelCol].alpha == actualTopPng[pixelRow][pixelCol].alpha);
    }
  }

  // Check attributes.csv
  porytiles::doctestAssertFileLinesIdentical(
      std::filesystem::path{"res/tests/compiled_emerald_lilycove/expected_decompiled/attributes.csv"},
      parentDir / std::filesystem::path{"attributes.csv"});

  std::filesystem::remove_all(parentDir);
}
