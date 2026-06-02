#include "legacy/driver.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <png.hpp>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "legacy/compiler.h"
#include "legacy/decompiler.h"
#include "legacy/emitter.h"
#include "legacy/importer.h"
#include "legacy/logger.h"
#include "legacy/porytiles_context.h"
#include "legacy/porytiles_exception.h"
#include "legacy/utilities.h"
#include "panic/panic.hpp"

namespace porytiles_legacy {

static void validateCompileInputs(const PorytilesContext &ctx, const CompilerMode compilerMode) {
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;

    if (!exists(ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode)) ||
        !is_directory(ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode))) {
        const auto msg = fmt::format("{}: source path did not exist or is not a directory",
                                     ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!exists(ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: file did not exist",
                                     ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!is_regular_file(ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: exists but was not a regular file",
                                     ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!exists(ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: file did not exist",
                                     ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!is_regular_file(ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: exists but was not a regular file",
                                     ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!exists(ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: file did not exist",
                                     ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (!is_regular_file(ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode))) {
        const auto msg = fmt::format("{}: exists but was not a regular file",
                                     ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }

    try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode)};
    } catch (std::exception &) {
        const auto msg = fmt::format("{} is not a valid PNG file",
                                     ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode)};
    } catch (std::exception &) {
        const auto msg = fmt::format("{} is not a valid PNG file",
                                     ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode)};
    } catch (std::exception &) {
        const auto msg = fmt::format("{} is not a valid PNG file",
                                     ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
}

static void validateDecompileInputs(PorytilesContext &ctx, const DecompilerMode decompilerMode) {
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;

    if (!exists(ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode)) ||
        !is_directory(ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode))) {
        const auto msg = fmt::format("{}: source path did not exist or is not a directory",
                                     ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }
    if (!exists(ctx.decompilerSrcPaths.modeBasedMetatilesPath(decompilerMode))) {
        const auto msg = fmt::format("{}: file did not exist",
                                     ctx.decompilerSrcPaths.modeBasedMetatilesPath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }
    if (!exists(ctx.decompilerSrcPaths.modeBasedAttributePath(decompilerMode))) {
        const auto msg = fmt::format("{}: file did not exist",
                                     ctx.decompilerSrcPaths.modeBasedAttributePath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }
    if (!exists(ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode))) {
        const auto msg =
            fmt::format("{}: file did not exist", ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }
    if (!exists(ctx.decompilerSrcPaths.modeBasedPalettePath(decompilerMode))) {
        const auto msg = fmt::format("{}: directory did not exist",
                                     ctx.decompilerSrcPaths.modeBasedPalettePath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }

    try {
        // We do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode)};
    } catch (std::exception &) {
        const auto msg = fmt::format("{} is not a valid PNG file",
                                     ctx.decompilerSrcPaths.modeBasedTilesPath(decompilerMode).string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }

    if (!exists(ctx.decompilerSrcPaths.metatileBehaviors) ||
        !is_regular_file(ctx.decompilerSrcPaths.metatileBehaviors)) {
        const auto msg = fmt::format("{}: behaviors header did not exist or was not a regular file",
                                     ctx.decompilerSrcPaths.metatileBehaviors);
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
    }
}

static void validateCompileOutputs(PorytilesContext &ctx, const CompilerMode compilerMode,
                                   const std::filesystem::path &attributesPath,
                                   const std::filesystem::path &tilesetPath, const std::filesystem::path &metatilesPath,
                                   const std::filesystem::path &palettesPath, const std::filesystem::path &animsPath) {
    using std::filesystem::create_directories;
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;

    if (exists(ctx.output.path) && !is_directory(ctx.output.path)) {
        const auto msg = fmt::format("{}: exists but is not a directory", ctx.output.path);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (exists(attributesPath) && !is_regular_file(attributesPath)) {
        const auto msg = fmt::format("'{}' exists but is not a file", attributesPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (exists(tilesetPath) && !is_regular_file(tilesetPath)) {
        const auto msg = fmt::format("'{}' exists but is not a file", tilesetPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (exists(metatilesPath) && !is_regular_file(metatilesPath)) {
        const auto msg = fmt::format("'{}' exists but is not a file", metatilesPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (exists(palettesPath) && !is_directory(palettesPath)) {
        const auto msg = fmt::format("'{}' exists but is not a directory", palettesPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (exists(animsPath) && !is_directory(animsPath)) {
        const auto msg = fmt::format("'{}' exists but is not a directory", animsPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }

    try {
        create_directories(palettesPath);
    } catch (const std::exception &e) {
        const auto msg = fmt::format("could not create '{}': {}", palettesPath.string(), e.what());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    try {
        create_directories(animsPath);
    } catch (const std::exception &e) {
        const auto msg = fmt::format("could not create '{}': {}", animsPath.string(), e.what());
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
}

static void validateDecompileOutputs(PorytilesContext &ctx, const DecompilerMode mode,
                                     const std::filesystem::path &outputPath,
                                     const std::filesystem::path &attributesPath,
                                     const std::filesystem::path &bottomPath, const std::filesystem::path &middlePath,
                                     const std::filesystem::path &topPath) {
    using std::filesystem::create_directories;
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;

    if (exists(ctx.output.path) && !is_directory(ctx.output.path)) {
        const auto msg = fmt::format("{}: exists but is not a directory", ctx.output.path);
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }
    if (exists(attributesPath) && !is_regular_file(attributesPath)) {
        const auto msg = fmt::format("'{}' exists in output directory but is not a file", attributesPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }
    if (exists(bottomPath) && !is_regular_file(bottomPath)) {
        const auto msg = fmt::format("'{}' exists in output directory but is not a file", bottomPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }
    if (exists(middlePath) && !is_regular_file(middlePath)) {
        const auto msg = fmt::format("'{}' exists in output directory but is not a file", middlePath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }
    if (exists(topPath) && !is_regular_file(topPath)) {
        const auto msg = fmt::format("'{}' exists in output directory but is not a file", topPath.string());
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }

    if (!outputPath.empty()) {
        try {
            create_directories(outputPath);
        } catch (const std::exception &e) {
            const auto msg = fmt::format("could not create '{}': {}", outputPath.string(), e.what());
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
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
//     pt_logln(ctx, stderr, "path '{}' did not exist, skipping animations import", animationPath.string());
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
prepareDecompiledAnimsForImport(PorytilesContext &ctx, const CompilerMode compilerMode,
                                const std::filesystem::path &animationPath) {
    using std::filesystem::create_directories;
    using std::filesystem::directory_iterator;
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;
    using std::filesystem::path;

    std::vector<std::vector<AnimationPng<png::rgba_pixel>>> animations{};

    pt_logln(ctx, stderr, "importing animations from {}", animationPath.string());
    if (!exists(animationPath) || !is_directory(animationPath)) {
        pt_logln(ctx, stderr, "path '{}' did not exist, skipping animations import", animationPath.string());
        return animations;
    }
    std::vector<path> animationDirectories;
    std::copy(directory_iterator(animationPath), directory_iterator(), std::back_inserter(animationDirectories));
    std::ranges::sort(animationDirectories);
    for (const auto &animDir : animationDirectories) {
        if (!is_directory(animDir)) {
            pt_logln(ctx, stderr, "skipping regular file: {}", animDir.string());
            continue;
        }

        // collate all possible animation frame files
        pt_logln(ctx, stderr, "found animation: {}", animDir.string());
        std::unordered_map<std::size_t, path> frames{};
        path keyFrameFile = animDir / path{"key.png"};
        if (!exists(keyFrameFile) || !is_regular_file(keyFrameFile)) {
            const auto msg =
                fmt::format("animation '{}' was missing key frame file", ctx.diag->Bold(animDir.filename().string()));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                      fmt::format("animation {} missing key frame file", animDir.filename().string()));
        }
        frames.insert(std::pair{0, keyFrameFile});
        pt_logln(ctx, stderr, "found key frame file: {}, index=0", keyFrameFile.string());
        for (const auto &frameFile : directory_iterator(animDir)) {
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
            const auto msg =
                fmt::format("animation '{}' was missing expected frame file '{}'",
                            ctx.diag->Bold(animDir.filename().string()), ctx.diag->Bold(palIndexToFileName(0)));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                      fmt::format("animation {} missing required anim frame file {}",
                                                  animDir.filename().string(), palIndexToFileName(0)));
        }
        for (std::size_t i = 0; i < frames.size(); i++) {
            if (!frames.contains(i)) {
                const auto msg =
                    fmt::format("animation '{}' was missing expected frame file '{}'",
                                ctx.diag->Bold(animDir.filename().string()), ctx.diag->Bold(palIndexToFileName(i - 1)));
                ctx.diag->Report(FatalGeneric, msg);
                die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                          fmt::format("animation {} missing required anim frame file {}",
                                                      animDir.filename().string(), palIndexToFileName(i - 1)));
            }

            try {
                // We do this here so if the source is not a PNG, we can catch and give a better error
                png::image<png::rgba_pixel> png{frames.at(i)};
                AnimationPng animPng{png, animDir.filename().string(), frames.at(i).filename().string()};
                framePngs.push_back(animPng);
            } catch ([[maybe_unused]] const std::exception &exception) {
                ctx.diag->Report(ErrGeneric, fmt::format("animation '{}' frame file '{}' was not a valid PNG file",
                                                         ctx.diag->Bold(animDir.filename().string()),
                                                         ctx.diag->Bold(frames.at(i).filename().string())));
            }
        }

        animations.push_back(framePngs);
    }
    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), "found anim frame that was not a png");
    }

    return animations;
}

static std::unordered_map<std::size_t, Attributes>
prepareDecompiledAttributesForImport(PorytilesContext &ctx, const CompilerMode compilerMode,
                                     const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                                     const std::filesystem::path &attributesCsvPath) {
    using std::filesystem::exists;
    using std::filesystem::is_regular_file;

    pt_logln(ctx, stderr, "importing attributes from {}", attributesCsvPath.string());
    if (!exists(attributesCsvPath) || !is_regular_file(attributesCsvPath)) {
        pt_logln(ctx, stderr, "path '{}' did not exist, skipping attributes import", attributesCsvPath.string());
        ctx.diag->Report(WarnMissingAttributesCsv, ctx.diag->Bold(attributesCsvPath.string()));
        ctx.diag->ReportPartner(WarnMissingAttributesCsv, 0);
        return {};
    }

    return importAttributesFromCsv(ctx, compilerMode, behaviorMap, attributesCsvPath.string());
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImportHelper(PorytilesContext &ctx, const CompilerMode *compilerMode,
                                      const DecompilerMode *decompilerMode, const std::string &behaviorHeaderPath) {
    std::ifstream behaviorFile{behaviorHeaderPath};
    if (behaviorFile.fail()) {
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("{}: could not open for reading", behaviorHeaderPath);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("{}: could not open for reading", behaviorHeaderPath);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
    }
    auto [behaviorMap, behaviorReverseMap] = std::invoke(
        [&]()
            -> std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>> {
            if (compilerMode != nullptr) {
                return importMetatileBehaviorHeader(ctx, *compilerMode, behaviorFile);
            }
            if (decompilerMode != nullptr) {
                return importMetatileBehaviorHeader(ctx, *decompilerMode, behaviorFile);
            }
            Panic("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
            // unreachable, here for compiler
            throw std::runtime_error("driver::prepareBehaviorsHeaderForImportHelper reached unreachable code path");
        });
    behaviorFile.close();
    if (behaviorMap.empty()) {
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("{}: behavior header did not contain any valid mappings", behaviorHeaderPath);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("{}: behavior header did not contain any valid mappings", behaviorHeaderPath);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("driver::prepareBehaviorsHeaderForImportHelper both mode parameters were null");
    }
    return std::pair{behaviorMap, behaviorReverseMap};
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImport(PorytilesContext &ctx, const CompilerMode compilerMode,
                                const std::string &behaviorHeaderPath) {
    return prepareBehaviorsHeaderForImportHelper(ctx, &compilerMode, nullptr, behaviorHeaderPath);
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
prepareBehaviorsHeaderForImport(PorytilesContext &ctx, const DecompilerMode decompilerMode,
                                const std::string &behaviorHeaderPath) {
    return prepareBehaviorsHeaderForImportHelper(ctx, nullptr, &decompilerMode, behaviorHeaderPath);
}

static std::vector<RGBATile> preparePalettePrimersForImport(PorytilesContext &ctx, CompilerMode compilerMode,
                                                            const std::filesystem::path &palettePrimersPath) {
    using std::filesystem::directory_iterator;
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;
    using std::filesystem::path;

    std::vector<RGBATile> primerTiles{};

    pt_logln(ctx, stderr, "importing palette primers from {}", palettePrimersPath.string());
    if (!exists(palettePrimersPath) || !is_directory(palettePrimersPath)) {
        pt_logln(ctx, stderr, "path '{}' did not exist, skipping palette primers import", palettePrimersPath.string());
        return primerTiles;
    }

    std::vector<path> primerFiles;
    std::copy(directory_iterator(palettePrimersPath), directory_iterator(), std::back_inserter(primerFiles));
    std::ranges::sort(primerFiles);

    for (const auto &primerFile : primerFiles) {
        const auto &fullPrimerFilename =
            ctx.compilerSrcPaths.modeBasedPalettePrimerPath(compilerMode).string() / primerFile.filename();

        // Check if the file is a regular file
        if (!is_regular_file(primerFile)) {
            pt_logln(ctx, stderr, "skipping {} as it is not a regular file", primerFile.string());
            continue;
        }

        // Check if the file has a .pal extension
        if (primerFile.extension() != ".pal") {
            pt_logln(ctx, stderr, "skipping {} as it does not have a .pal extension", primerFile.string());
            continue;
        }

        std::ifstream fileStream{primerFile};
        pt_logln(ctx, stderr, "found palette primer file {}", primerFile.string());
        RGBATile primerTile = importPalettePrimer(ctx, compilerMode, fileStream, fullPrimerFilename);
        primerTile.primerFilename =
            fullPrimerFilename.lexically_relative(ctx.compilerSrcPaths.modeBasedPalettePrimerPath(compilerMode))
                .string();
        primerTiles.push_back(primerTile);
        fileStream.close();
    }

    return primerTiles;
}

static std::pair<std::vector<RGBATile>, std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, BGR15>>>>
preparePaletteOverridesForImport(PorytilesContext &ctx, const CompilerMode compilerMode,
                                 const std::filesystem::path &paletteOverridesPath) {
    using std::filesystem::directory_iterator;
    using std::filesystem::exists;
    using std::filesystem::is_directory;
    using std::filesystem::is_regular_file;
    using std::filesystem::path;

    std::vector<RGBATile> overrideTiles{};
    std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, BGR15>>> palOverrides{};

    pt_logln(ctx, stderr, "importing palette overrides from {}", paletteOverridesPath.string());
    if (!exists(paletteOverridesPath) || !is_directory(paletteOverridesPath)) {
        pt_logln(ctx, stderr, "path '{}' did not exist, skipping palette overrides import",
                 paletteOverridesPath.string());
        return {overrideTiles, {}};
    }

    std::vector<path> overrideFiles;
    std::copy(directory_iterator(paletteOverridesPath), directory_iterator(), std::back_inserter(overrideFiles));
    std::ranges::sort(overrideFiles);

    for (const auto &overrideFile : overrideFiles) {
        const auto &fullOverrideFilename =
            ctx.compilerSrcPaths.modeBasedPaletteOverridePath(compilerMode).string() / overrideFile.filename();
        // Check if the file is a regular file
        if (!is_regular_file(overrideFile)) {
            pt_logln(ctx, stderr, "skipping {} as it is not a regular file", overrideFile.string());
            continue;
        }

        // Check if the file has a .pal extension
        if (overrideFile.extension() != ".pal") {
            pt_logln(ctx, stderr, "skipping {} as it does not have a .pal extension", overrideFile.string());
            continue;
        }

        // Make sure pal file name is in format e.g. 01.pal
        if (!checkFullStringMatch(overrideFile.stem().string(), "[0,1][0-9]")) {
            const auto msg = fmt::format("pal file {} at {}: name must match regex [0,1][0-9]",
                                         overrideFile.stem().string(), overrideFile.string());
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
        }

        std::size_t overridePaletteIndex{};
        try {
            const auto stem = overrideFile.stem();
            overridePaletteIndex = parseInteger<std::size_t>(stem.c_str(), 10);
        } catch ([[maybe_unused]] const std::exception &e) {
            Panic("driver::preparePaletteOverridesForImport parseInteger with invalid pal override file name");
        }

        // Throw fatal if user specifies an out-of-range palette index for their compilation mode
        if (compilerMode == CompilerMode::PRIMARY) {
            if (overridePaletteIndex >= ctx.fieldmapConfig.numPalettesInPrimary) {
                ctx.diag->Report(ErrGeneric, fmt::format("{} invalid palette index '{}': must be 0 <= index < {}",
                                                         ctx.diag->Bold(fullOverrideFilename.string() + ":"),
                                                         ctx.diag->Bold(overridePaletteIndex),
                                                         ctx.fieldmapConfig.numPalettesInPrimary));
            }
        } else if (compilerMode == CompilerMode::SECONDARY) {
            if (overridePaletteIndex < ctx.fieldmapConfig.numPalettesInPrimary ||
                overridePaletteIndex >= ctx.fieldmapConfig.numPalettesTotal) {
                ctx.diag->Report(ErrGeneric, fmt::format("{} invalid palette index '{}': must be {} <= index < {}",
                                                         ctx.diag->Bold(fullOverrideFilename.string() + ":"),
                                                         ctx.diag->Bold(overridePaletteIndex),
                                                         ctx.fieldmapConfig.numPalettesInPrimary,
                                                         ctx.fieldmapConfig.numPalettesTotal));
            }
        } else {
            Panic("driver::preparePaletteOverridesForImport called with invalid compiler mode");
        }

        std::ifstream fileStream{overrideFile};
        pt_logln(ctx, stderr, "found palette override file {}", overrideFile.string());
        auto [overrideTile, overriddenPalSlots] =
            importPaletteOverride(ctx, compilerMode, fileStream, fullOverrideFilename);
        overrideTile.overrideFilename =
            fullOverrideFilename.lexically_relative(ctx.compilerSrcPaths.modeBasedPaletteOverridePath(compilerMode))
                .string();
        overrideTile.overridePaletteIndex = overridePaletteIndex;
        overrideTiles.push_back(overrideTile);
        for (const auto &[palSlot, bgr] : overriddenPalSlots) {
            if (palOverrides.contains(overridePaletteIndex)) {
                palOverrides.at(overridePaletteIndex).emplace_back(palSlot, bgr);
            } else {
                palOverrides.insert({overridePaletteIndex, {{palSlot, bgr}}});
            }
        }
        fileStream.close();
    }

    return {overrideTiles, palOverrides};
}

static void driveEmitCompiledPalettes(PorytilesContext &ctx, const CompiledTileset &compiledTiles,
                                      const std::filesystem::path &palettesPath) {
    for (std::size_t i = 0; i < ctx.fieldmapConfig.numPalettesTotal; i++) {
        std::string fileName = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
        fileName += ".pal";
        std::filesystem::path paletteFile = palettesPath / fileName;
        std::ofstream outPal{paletteFile.string()};
        if (i < compiledTiles.palettes.size()) {
            emitPalette(ctx, outPal, compiledTiles.palettes.at(i));
        } else {
            emitZeroedPalette(ctx, outPal);
        }
        outPal.close();
    }
}

static void driveEmitCompiledTiles(PorytilesContext &ctx, const CompiledTileset &compiledTiles,
                                   const std::filesystem::path &tilesetPath) {
    const std::size_t imageWidth = TILE_SIDE_LENGTH_PIX * TILES_PNG_WIDTH_IN_TILES;
    const std::size_t imageHeight = TILE_SIDE_LENGTH_PIX * (compiledTiles.tiles.size() / TILES_PNG_WIDTH_IN_TILES);
    png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth),
                                          static_cast<png::uint_32>(imageHeight)};

    emitTilesPng(ctx, tilesPng, compiledTiles);
    tilesPng.write(tilesetPath);
}

static void driveEmitCompiledAnims(PorytilesContext &ctx, const std::vector<CompiledAnimation> &compiledAnims,
                                   const std::vector<GBAPalette> &palettes, const std::filesystem::path &animsPath) {
    using std::filesystem::create_directories;
    using std::filesystem::path;

    for (const auto &compiledAnim : compiledAnims) {
        path animPath = animsPath / compiledAnim.animName;
        create_directories(animPath);
        const std::size_t imageWidth = TILE_SIDE_LENGTH_PIX * compiledAnim.keyFrame().tiles.size();
        constexpr std::size_t imageHeight = TILE_SIDE_LENGTH_PIX;
        std::vector<png::image<png::index_pixel>> outFrames{};
        outFrames.reserve(compiledAnim.frames.size());
        for (std::size_t frameIndex = 0; frameIndex < compiledAnim.frames.size(); frameIndex++) {
            outFrames.emplace_back(static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight));
        }
        emitAnim(ctx, outFrames, compiledAnim, palettes);
        // Index starts at 1 here, so we don't save a key.png compiled file, not necessary
        for (std::size_t frameIndex = 1; frameIndex < compiledAnim.frames.size(); frameIndex++) {
            auto &frame = outFrames.at(frameIndex);
            path framePngPath = animPath / compiledAnim.frames.at(frameIndex).frameName;
            frame.write(framePngPath);
        }
    }
}

static void driveEmitCompiledTileset(PorytilesContext &ctx, CompilerMode compilerMode, const CompiledTileset &tileset,
                                     const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap) {
    using std::filesystem::path;

    /*
     * Emit output
     */
    path outputPath(ctx.output.path);
    path palettesDir("palettes");
    path animsDir("anim");
    path tilesPng("tiles.png");
    path metatilesBin("metatiles.bin");
    path attributesBin("metatile_attributes.bin");
    path tilesetPath = ctx.output.path / tilesPng;
    path metatilesPath = ctx.output.path / metatilesBin;
    path palettesPath = ctx.output.path / palettesDir;
    path animsPath = ctx.output.path / animsDir;
    path attributesPath = ctx.output.path / attributesBin;

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
                                       const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap) {
    using std::filesystem::path;

    path outputPath(ctx.output.path);
    path attributesCsvPath("attributes.csv");
    path bottomPngPath("bottom.png");
    path middlePngPath("middle.png");
    path topPngPath("top.png");
    path attributesPath = ctx.output.path / attributesCsvPath;
    path bottomPath = ctx.output.path / bottomPngPath;
    path middlePath = ctx.output.path / middlePngPath;
    path topPath = ctx.output.path / topPngPath;

    validateDecompileOutputs(ctx, mode, outputPath, attributesPath, bottomPath, middlePath, topPath);

    std::ostringstream outAttributesContent{};
    std::size_t metatileCount = attributesMap.size();
    std::size_t imageHeight = std::ceil(metatileCount / 8.0) * 16;
    png::image<png::rgba_pixel> bottomPng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
    png::image<png::rgba_pixel> middlePng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
    png::image<png::rgba_pixel> topPng{METATILE_SHEET_WIDTH, static_cast<png::uint_32>(imageHeight)};
    emitDecompiled(ctx, mode, bottomPng, middlePng, topPng, outAttributesContent, tileset, attributesMap,
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
                           std::unordered_map<uint8_t, std::string> &behaviorReverseMap) {
    using std::filesystem::exists;
    using std::filesystem::path;

    pt_logln(ctx, stderr, "importing {} compiled tileset from {}", decompilerModeString(mode),
             ctx.decompilerSrcPaths.primarySourcePath);

    /*
     * Set up file stream objects
     */
    std::ifstream metatilesIfStream{ctx.decompilerSrcPaths.modeBasedMetatilesPath(mode), std::ios::binary};
    std::ifstream attributesIfStream{ctx.decompilerSrcPaths.modeBasedAttributePath(mode), std::ios::binary};
    png::image<png::index_pixel> tilesheetPng{ctx.decompilerSrcPaths.modeBasedTilesPath(mode)};
    std::vector<std::unique_ptr<std::ifstream>> paletteFiles{};
    std::vector<std::string> paletteFileNames{};
    for (std::size_t index = 0; index < ctx.fieldmapConfig.numPalettesTotal; index++) {
        std::ostringstream filename;
        if (index < 10) {
            filename << "0";
        }
        filename << index << ".pal";
        path paletteFile = ctx.decompilerSrcPaths.modeBasedPalettePath(mode) / filename.str();
        if (!exists(paletteFile)) {
            const auto msg = fmt::format("{}: file did not exist", paletteFile.string());
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
        }
        paletteFiles.push_back(std::make_unique<std::ifstream>(paletteFile));
        paletteFileNames.emplace_back(paletteFile.c_str());
    }
    // TODO : bring this back to implement anim decompilation
    // auto compiledAnims = prepareCompiledAnimsForImport(ctx, ctx.decompilerSrcPaths.modeBasedAnimPath(mode));

    /*
     * Import the compiled tileset into our data types
     */
    // TODO : last param is empty atm, replace it with imported compiledAnims
    auto [compiledTileset, attributesMap] =
        importCompiledTileset(ctx, mode, metatilesIfStream, attributesIfStream, behaviorReverseMap, tilesheetPng,
                              paletteFiles, paletteFileNames, {});

    /*
     * Close file stream objects
     */
    metatilesIfStream.close();
    attributesIfStream.close();
    std::ranges::for_each(paletteFiles, [](const std::unique_ptr<std::ifstream> &stream) { stream->close(); });

    return std::pair{compiledTileset, attributesMap};
}

static std::pair<std::unique_ptr<CompiledTileset>, std::unordered_map<size_t, Attributes>>
driveCompileTileset(PorytilesContext &ctx, CompilerMode compilerMode, CompilerMode parentCompilerMode,
                    std::unordered_map<std::string, uint8_t> &behaviorMap) {
    auto compiledTileset = std::make_unique<CompiledTileset>();

    pt_logln(ctx, stderr, "importing {} tiles from {}", compilerModeString(compilerMode),
             ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string());
    png::image<png::rgba_pixel> bottomPng{ctx.compilerSrcPaths.modeBasedBottomTilesheetPath(compilerMode)};
    png::image<png::rgba_pixel> middlePng{ctx.compilerSrcPaths.modeBasedMiddleTilesheetPath(compilerMode)};
    png::image<png::rgba_pixel> topPng{ctx.compilerSrcPaths.modeBasedTopTilesheetPath(compilerMode)};

    auto attributesMap = prepareDecompiledAttributesForImport(
        ctx, compilerMode, behaviorMap, ctx.compilerSrcPaths.modeBasedAttributePath(compilerMode));
    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       fmt::format("errors generated during {} attributes import", compilerModeString(compilerMode)));
    }

    DecompiledTileset decompiledTiles =
        importLayeredTilesFromPngs(ctx, compilerMode, attributesMap, bottomPng, middlePng, topPng);

    auto animations =
        prepareDecompiledAnimsForImport(ctx, compilerMode, ctx.compilerSrcPaths.modeBasedAnimPath(compilerMode));
    importAnimTiles(ctx, compilerMode, animations, decompiledTiles);

    std::vector<RGBATile> palettePrimers = preparePalettePrimersForImport(
        ctx, compilerMode, ctx.compilerSrcPaths.modeBasedPalettePrimerPath(compilerMode));

    auto [paletteOverrides, paletteOverrideMap] = preparePaletteOverridesForImport(
        ctx, compilerMode, ctx.compilerSrcPaths.modeBasedPaletteOverridePath(compilerMode));

    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        const auto msg = "errors encountered while importing manual palettes";
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    compiledTileset = compile(ctx, compilerMode, decompiledTiles, palettePrimers, paletteOverrides, paletteOverrideMap);

    return std::pair{std::move(compiledTileset), attributesMap};
}

static std::pair<std::unique_ptr<DecompiledTileset>, std::unordered_map<size_t, Attributes>>
driveDecompileTileset(PorytilesContext &ctx, const DecompilerMode mode,
                      std::unordered_map<uint8_t, std::string> &behaviorReverseMap) {
    auto decompiled = std::make_unique<DecompiledTileset>();

    /*
     * Import the compiled tileset and attributes map from the given input paths.
     */
    auto [compiledTileset, attributesMap] = driveCompiledTilesetImport(ctx, mode, behaviorReverseMap);

    /*
     * Decompile the compiled tiles
     */
    decompiled = decompile(ctx, mode, compiledTileset, attributesMap);

    return std::pair{std::move(decompiled), attributesMap};
}

static void driveDecompilePrimary(PorytilesContext &ctx) {
    validateDecompileInputs(ctx, DecompilerMode::PRIMARY);

    /*
     * Import behavior header, if it was supplied
     */
    auto [behaviorMap, behaviorReverseMap] =
        prepareBehaviorsHeaderForImport(ctx, DecompilerMode::PRIMARY, ctx.decompilerSrcPaths.metatileBehaviors);

    /*
     * Decompile the compiled primary tiles
     */
    auto [decompiled, attributesMap] = driveDecompileTileset(ctx, DecompilerMode::PRIMARY, behaviorReverseMap);

    /*
     * Emit the decompiled primary tileset.
     */
    driveEmitDecompiledTileset(ctx, DecompilerMode::PRIMARY, *decompiled, attributesMap, behaviorReverseMap);
}

static void driveDecompileSecondary(PorytilesContext &ctx) {
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
        driveCompiledTilesetImport(ctx, DecompilerMode::PRIMARY, behaviorReverseMap);

    /*
     * Decompile the compiled secondary tiles
     */
    ctx.decompilerContext.pairedPrimaryTileset = std::make_unique<CompiledTileset>(primaryCompiledTileset);
    auto [decompiled, attributesMap] = driveDecompileTileset(ctx, DecompilerMode::SECONDARY, behaviorReverseMap);

    /*
     * Emit the decompiled secondary tileset.
     */
    driveEmitDecompiledTileset(ctx, DecompilerMode::SECONDARY, *decompiled, attributesMap, behaviorReverseMap);
}

static void driveCompilePrimary(PorytilesContext &ctx) {
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
    } else {
        const auto msg = fmt::format("{}: file did not exist", ctx.compilerSrcPaths.metatileBehaviors);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::PRIMARY), msg);
    }

    /*
     * Now that we have imported the behavior header, let's parse the arguments to the -default-X options if they were
     * supplied. If the user provided an integer, just use that. Otherwise, if the user provided a label string, check
     * it against the behavior header or terrain/encounter type tables and replace that string with the integral value.
     */
    // FIXME : default behavior/encounter/terrain parsing code is duped
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
    } catch ([[maybe_unused]] const std::exception &e) {
        /*
         * If the integer parse fails, assume the user provided a behavior label and try to parse that based on the
         * mappings from the behaviors header.
         */
        if (!behaviorMap.contains(ctx.compilerConfig.defaultBehavior)) {
            const auto msg = fmt::format("supplied default behavior '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultBehavior));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::PRIMARY), msg);
        }
        ctx.compilerConfig.defaultBehavior = std::to_string(behaviorMap.at(ctx.compilerConfig.defaultBehavior));
    }
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
    } catch ([[maybe_unused]] const std::exception &e) {
        /*
         * If the integer parse fails, assume the user provided an encounter label and try to parse that based on the
         * mappings from the encounter table.
         */
        try {
            const EncounterType type = stringToEncounterType(ctx.compilerConfig.defaultEncounterType);
            ctx.compilerConfig.defaultEncounterType = std::to_string(encounterTypeValue(type));
        } catch ([[maybe_unused]] const std::exception &e1) {
            const auto msg = fmt::format("supplied default EncounterType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultEncounterType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::PRIMARY), msg);
        }
    }
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
    } catch (std::exception &) {
        /*
         * If the integer parse fails, assume the user provided an terrain label and try to parse that based on the
         * mappings from the terrain table.
         */
        try {
            const TerrainType type = stringToTerrainType(ctx.compilerConfig.defaultTerrainType);
            ctx.compilerConfig.defaultTerrainType = std::to_string(terrainTypeValue(type));
        } catch ([[maybe_unused]] const std::exception &e1) {
            const auto msg = fmt::format("supplied default TerrainType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultTerrainType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::PRIMARY), msg);
        }
    }

    auto [compiledTileset, attributesMap] =
        driveCompileTileset(ctx, CompilerMode::PRIMARY, CompilerMode::PRIMARY, behaviorMap);

    ctx.compilerContext.resultTileset = std::move(compiledTileset);

    driveEmitCompiledTileset(ctx, CompilerMode::PRIMARY, *(ctx.compilerContext.resultTileset), behaviorReverseMap);
}

static void driveCompileSecondary(PorytilesContext &ctx) {
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
    } else {
        const auto msg = fmt::format("{}: file did not exist", ctx.compilerSrcPaths.metatileBehaviors);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::SECONDARY), msg);
    }

    /*
     * Now that we have imported the behavior header, let's parse the arguments to the -default-X options if they were
     * supplied. If the user provided an integer, just use that. Otherwise, if the user provided a label string, check
     * it against the behavior header or terrain/encounter type tables and replace that string with the integral value.
     */
    // FIXME : default behavior/encounter/terrain parsing code is duped
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
    } catch (std::exception &) {
        /*
         * If the integer parse fails, assume the user provided a behavior label and try to parse that based on the
         * mappings from the behaviors header.
         */
        if (!behaviorMap.contains(ctx.compilerConfig.defaultBehavior)) {
            const auto msg = fmt::format("supplied default behavior '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultBehavior));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::SECONDARY), msg);
        }
        ctx.compilerConfig.defaultBehavior = std::to_string(behaviorMap.at(ctx.compilerConfig.defaultBehavior));
    }
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
    } catch (const std::exception &e) {
        /*
         * If the integer parse fails, assume the user provided an encounter label and try to parse that based on the
         * mappings from the encounter table.
         */
        try {
            EncounterType type = stringToEncounterType(ctx.compilerConfig.defaultEncounterType);
            ctx.compilerConfig.defaultEncounterType = std::to_string(encounterTypeValue(type));
        } catch ([[maybe_unused]] const std::exception &e1) {
            const auto msg = fmt::format("supplied default EncounterType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultEncounterType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::SECONDARY), msg);
        }
    }
    try {
        parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
    } catch (const std::exception &e) {
        /*
         * If the integer parse fails, assume the user provided an terrain label and try to parse that based on the
         * mappings from the terrain table.
         */
        try {
            TerrainType type = stringToTerrainType(ctx.compilerConfig.defaultTerrainType);
            ctx.compilerConfig.defaultTerrainType = std::to_string(terrainTypeValue(type));
        } catch ([[maybe_unused]] const std::exception &e1) {
            const auto msg = fmt::format("supplied default TerrainType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultTerrainType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::SECONDARY), msg);
        }
    }

    auto [compiledPairedPrimaryTileset, pairedPrimaryAttributesMap] =
        driveCompileTileset(ctx, CompilerMode::PRIMARY, CompilerMode::SECONDARY, behaviorMap);
    ctx.compilerContext.pairedPrimaryTileset = std::move(compiledPairedPrimaryTileset);

    auto [compiledTileset, attributesMap] =
        driveCompileTileset(ctx, CompilerMode::SECONDARY, CompilerMode::SECONDARY, behaviorMap);

    ctx.compilerContext.resultTileset = std::move(compiledTileset);

    driveEmitCompiledTileset(ctx, CompilerMode::SECONDARY, *(ctx.compilerContext.resultTileset), behaviorReverseMap);
}

void drive(PorytilesContext &ctx) {
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
        Panic("driver::drive unknown subcommand setting");
    }
}

} // namespace porytiles_legacy

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("drive should emit all expected files for anim_metatiles_2 primary set") {
    porytiles_legacy::PorytilesContext ctx{};
    std::filesystem::path parentDir = porytiles_legacy::createTmpdir();
    ctx.output.path = parentDir;
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/anim_metatiles_2/primary"}));
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/anim_metatiles_2/primary";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/metatile_behaviors.h"}));
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";

    porytiles_legacy::drive(ctx);

    // TODO tests : (drive should emit all expected files...) test palette files are correct

    // Check tiles.png

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_tiles.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"tiles.png"}));
    png::image<png::index_pixel> expectedPng{"resources/doctests/anim_metatiles_2/primary/expected_tiles.png"};
    png::image<png::index_pixel> actualPng{parentDir / std::filesystem::path{"tiles.png"}};

    std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualWidthInTiles = actualPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualHeightInTiles = actualPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedWidthInTiles == actualWidthInTiles);
    CHECK(expectedHeightInTiles == actualHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
        }
    }

    // Check metatiles.bin
    porytiles_legacy::doctestAssertFileBytesIdentical(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_metatiles.bin"},
        parentDir / std::filesystem::path{"metatiles.bin"});

    // Check metatile_attributes.bin
    porytiles_legacy::doctestAssertFileBytesIdentical(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_metatile_attributes.bin"},
        parentDir / std::filesystem::path{"metatile_attributes.bin"});

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_anim/water/00.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/primary/expected_anim/water/01.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/00.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/01.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_white/02.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/water/00.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/water/01.png"}));

    png::image<png::index_pixel> expected_flower_white_00{
        "resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/00.png"};
    png::image<png::index_pixel> actual_flower_white_00{parentDir / std::filesystem::path{"anim/flower_white/00.png"}};
    expectedWidthInTiles = expected_flower_white_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_white_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_white_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_white_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_white_00[pixelRow][pixelCol] == actual_flower_white_00[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_flower_white_01{
        "resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/01.png"};
    png::image<png::index_pixel> actual_flower_white_01{parentDir / std::filesystem::path{"anim/flower_white/01.png"}};
    expectedWidthInTiles = expected_flower_white_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_white_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_white_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_white_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_white_01[pixelRow][pixelCol] == actual_flower_white_01[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_flower_white_02{
        "resources/doctests/anim_metatiles_2/primary/expected_anim/flower_white/02.png"};
    png::image<png::index_pixel> actual_flower_white_02{parentDir / std::filesystem::path{"anim/flower_white/02.png"}};
    expectedWidthInTiles = expected_flower_white_02.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_white_02.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_white_02.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_white_02.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_white_02[pixelRow][pixelCol] == actual_flower_white_02[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_water_00{
        "resources/doctests/anim_metatiles_2/primary/expected_anim/water/00.png"};
    png::image<png::index_pixel> actual_water_00{parentDir / std::filesystem::path{"anim/water/00.png"}};
    expectedWidthInTiles = expected_water_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_water_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_water_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_water_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_water_00[pixelRow][pixelCol] == actual_water_00[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_water_01{
        "resources/doctests/anim_metatiles_2/primary/expected_anim/water/01.png"};
    png::image<png::index_pixel> actual_water_01{parentDir / std::filesystem::path{"anim/water/01.png"}};
    expectedWidthInTiles = expected_water_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_water_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_water_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_water_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_water_01[pixelRow][pixelCol] == actual_water_01[pixelRow][pixelCol]);
        }
    }

    std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for anim_metatiles_2 secondary set") {
    porytiles_legacy::PorytilesContext ctx{};
    std::filesystem::path parentDir = porytiles_legacy::createTmpdir();
    ctx.output.path = parentDir;
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/anim_metatiles_2/primary"}));
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/anim_metatiles_2/primary";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary"}));
    ctx.compilerSrcPaths.secondarySourcePath = "resources/doctests/anim_metatiles_2/secondary";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/metatile_behaviors.h"}));
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";

    porytiles_legacy::drive(ctx);

    // TODO tests : (drive should emit all expected files...) test palette files are correct

    // Check tiles.png

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_tiles.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"tiles.png"}));
    png::image<png::index_pixel> expectedPng{"resources/doctests/anim_metatiles_2/secondary/expected_tiles.png"};
    png::image<png::index_pixel> actualPng{parentDir / std::filesystem::path{"tiles.png"}};

    std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualWidthInTiles = actualPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualHeightInTiles = actualPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedWidthInTiles == actualWidthInTiles);
    CHECK(expectedHeightInTiles == actualHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
        }
    }

    // Check metatiles.bin
    porytiles_legacy::doctestAssertFileBytesIdentical(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_metatiles.bin"},
        parentDir / std::filesystem::path{"metatiles.bin"});

    // Check metatile_attributes.bin
    porytiles_legacy::doctestAssertFileBytesIdentical(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_metatile_attributes.bin"},
        parentDir / std::filesystem::path{"metatile_attributes.bin"});

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"}));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/00.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/01.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"anim/flower_red/02.png"}));

    png::image<png::index_pixel> expected_flower_red_00{
        "resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/00.png"};
    png::image<png::index_pixel> actual_flower_red_00{parentDir / std::filesystem::path{"anim/flower_red/00.png"}};
    expectedWidthInTiles = expected_flower_red_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_red_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_red_00.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_red_00.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_red_00[pixelRow][pixelCol] == actual_flower_red_00[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_flower_red_01{
        "resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/01.png"};
    png::image<png::index_pixel> actual_flower_red_01{parentDir / std::filesystem::path{"anim/flower_red/01.png"}};
    expectedWidthInTiles = expected_flower_red_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_red_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_red_01.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_red_01.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_red_01[pixelRow][pixelCol] == actual_flower_red_01[pixelRow][pixelCol]);
        }
    }
    png::image<png::index_pixel> expected_flower_red_02{
        "resources/doctests/anim_metatiles_2/secondary/expected_anim/flower_red/02.png"};
    png::image<png::index_pixel> actual_flower_red_02{parentDir / std::filesystem::path{"anim/flower_red/02.png"}};
    expectedWidthInTiles = expected_flower_red_02.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    expectedHeightInTiles = expected_flower_red_02.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualWidthInTiles = actual_flower_red_02.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    actualHeightInTiles = actual_flower_red_02.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    for (std::size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualWidthInTiles;
        std::size_t tileCol = tileIndex % actualWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expected_flower_red_02[pixelRow][pixelCol] == actual_flower_red_02[pixelRow][pixelCol]);
        }
    }

    std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for compiled_emerald_general") {
    porytiles_legacy::PorytilesContext ctx{};
    std::filesystem::path parentDir = porytiles_legacy::createTmpdir();
    ctx.output.path = parentDir;
    ctx.subcommand = porytiles_legacy::Subcommand::DECOMPILE_PRIMARY;
    ctx.printDieMsg = false;
    ctx.decompilerConfig.normalizeTransparency = false;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/compiled_emerald_general"}));
    ctx.decompilerSrcPaths.primarySourcePath = "resources/doctests/compiled_emerald_general";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/metatile_behaviors.h"}));
    ctx.decompilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";

    porytiles_legacy::drive(ctx);

    // Check bottom.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_general/expected_decompiled/bottom.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"bottom.png"}));
    png::image<png::rgba_pixel> expectedBottomPng{
        "resources/doctests/compiled_emerald_general/expected_decompiled/bottom.png"};
    png::image<png::rgba_pixel> actualBottomPng{parentDir / std::filesystem::path{"bottom.png"}};

    std::size_t expectedBottomWidthInTiles = expectedBottomPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedBottomHeightInTiles = expectedBottomPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualBottomWidthInTiles = actualBottomPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualBottomHeightInTiles = actualBottomPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedBottomWidthInTiles == actualBottomWidthInTiles);
    CHECK(expectedBottomHeightInTiles == actualBottomHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualBottomWidthInTiles * actualBottomHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualBottomWidthInTiles;
        std::size_t tileCol = tileIndex % actualBottomWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedBottomPng[pixelRow][pixelCol].red == actualBottomPng[pixelRow][pixelCol].red);
            CHECK(expectedBottomPng[pixelRow][pixelCol].green == actualBottomPng[pixelRow][pixelCol].green);
            CHECK(expectedBottomPng[pixelRow][pixelCol].blue == actualBottomPng[pixelRow][pixelCol].blue);
            CHECK(expectedBottomPng[pixelRow][pixelCol].alpha == actualBottomPng[pixelRow][pixelCol].alpha);
        }
    }

    // Check middle.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_general/expected_decompiled/middle.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"middle.png"}));
    png::image<png::rgba_pixel> expectedMiddlePng{
        "resources/doctests/compiled_emerald_general/expected_decompiled/middle.png"};
    png::image<png::rgba_pixel> actualMiddlePng{parentDir / std::filesystem::path{"middle.png"}};

    std::size_t expectedMiddleWidthInTiles = expectedMiddlePng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedMiddleHeightInTiles = expectedMiddlePng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualMiddleWidthInTiles = actualMiddlePng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualMiddleHeightInTiles = actualMiddlePng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedMiddleWidthInTiles == actualMiddleWidthInTiles);
    CHECK(expectedMiddleHeightInTiles == actualMiddleHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualMiddleWidthInTiles * actualMiddleHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualMiddleWidthInTiles;
        std::size_t tileCol = tileIndex % actualMiddleWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].red == actualMiddlePng[pixelRow][pixelCol].red);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].green == actualMiddlePng[pixelRow][pixelCol].green);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].blue == actualMiddlePng[pixelRow][pixelCol].blue);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].alpha == actualMiddlePng[pixelRow][pixelCol].alpha);
        }
    }

    // Check top.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_general/expected_decompiled/top.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"top.png"}));
    png::image<png::rgba_pixel> expectedTopPng{
        "resources/doctests/compiled_emerald_general/expected_decompiled/top.png"};
    png::image<png::rgba_pixel> actualTopPng{parentDir / std::filesystem::path{"top.png"}};

    std::size_t expectedTopWidthInTiles = expectedTopPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedTopHeightInTiles = expectedTopPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualTopWidthInTiles = actualTopPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualTopHeightInTiles = actualTopPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedTopWidthInTiles == actualTopWidthInTiles);
    CHECK(expectedTopHeightInTiles == actualTopHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualTopWidthInTiles * actualTopHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualTopWidthInTiles;
        std::size_t tileCol = tileIndex % actualTopWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedTopPng[pixelRow][pixelCol].red == actualTopPng[pixelRow][pixelCol].red);
            CHECK(expectedTopPng[pixelRow][pixelCol].green == actualTopPng[pixelRow][pixelCol].green);
            CHECK(expectedTopPng[pixelRow][pixelCol].blue == actualTopPng[pixelRow][pixelCol].blue);
            CHECK(expectedTopPng[pixelRow][pixelCol].alpha == actualTopPng[pixelRow][pixelCol].alpha);
        }
    }

    // Check attributes.csv
    porytiles_legacy::doctestAssertFileLinesIdentical(
        std::filesystem::path{"resources/doctests/compiled_emerald_general/expected_decompiled/attributes.csv"},
        parentDir / std::filesystem::path{"attributes.csv"});

    // TODO tests : (drive should emit all expected files) test animations once we implement anim decomp

    std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for compiled_emerald_lilycove") {
    porytiles_legacy::PorytilesContext ctx{};
    std::filesystem::path parentDir = porytiles_legacy::createTmpdir();
    ctx.output.path = parentDir;
    ctx.subcommand = porytiles_legacy::Subcommand::DECOMPILE_SECONDARY;
    ctx.printDieMsg = false;
    ctx.decompilerConfig.normalizeTransparency = true;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/compiled_emerald_general"}));
    ctx.decompilerSrcPaths.primarySourcePath = "resources/doctests/compiled_emerald_general";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/compiled_emerald_lilycove"}));
    ctx.decompilerSrcPaths.secondarySourcePath = "resources/doctests/compiled_emerald_lilycove";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"resources/doctests/metatile_behaviors.h"}));
    ctx.decompilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";

    porytiles_legacy::drive(ctx);

    // Check bottom.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_lilycove/expected_decompiled/bottom.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"bottom.png"}));
    png::image<png::rgba_pixel> expectedBottomPng{
        "resources/doctests/compiled_emerald_lilycove/expected_decompiled/bottom.png"};
    png::image<png::rgba_pixel> actualBottomPng{parentDir / std::filesystem::path{"bottom.png"}};

    std::size_t expectedBottomWidthInTiles = expectedBottomPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedBottomHeightInTiles = expectedBottomPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualBottomWidthInTiles = actualBottomPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualBottomHeightInTiles = actualBottomPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedBottomWidthInTiles == actualBottomWidthInTiles);
    CHECK(expectedBottomHeightInTiles == actualBottomHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualBottomWidthInTiles * actualBottomHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualBottomWidthInTiles;
        std::size_t tileCol = tileIndex % actualBottomWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedBottomPng[pixelRow][pixelCol].red == actualBottomPng[pixelRow][pixelCol].red);
            CHECK(expectedBottomPng[pixelRow][pixelCol].green == actualBottomPng[pixelRow][pixelCol].green);
            CHECK(expectedBottomPng[pixelRow][pixelCol].blue == actualBottomPng[pixelRow][pixelCol].blue);
            CHECK(expectedBottomPng[pixelRow][pixelCol].alpha == actualBottomPng[pixelRow][pixelCol].alpha);
        }
    }

    // Check middle.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_lilycove/expected_decompiled/middle.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"middle.png"}));
    png::image<png::rgba_pixel> expectedMiddlePng{
        "resources/doctests/compiled_emerald_lilycove/expected_decompiled/middle.png"};
    png::image<png::rgba_pixel> actualMiddlePng{parentDir / std::filesystem::path{"middle.png"}};

    std::size_t expectedMiddleWidthInTiles = expectedMiddlePng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedMiddleHeightInTiles = expectedMiddlePng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualMiddleWidthInTiles = actualMiddlePng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualMiddleHeightInTiles = actualMiddlePng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedMiddleWidthInTiles == actualMiddleWidthInTiles);
    CHECK(expectedMiddleHeightInTiles == actualMiddleHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualMiddleWidthInTiles * actualMiddleHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualMiddleWidthInTiles;
        std::size_t tileCol = tileIndex % actualMiddleWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].red == actualMiddlePng[pixelRow][pixelCol].red);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].green == actualMiddlePng[pixelRow][pixelCol].green);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].blue == actualMiddlePng[pixelRow][pixelCol].blue);
            CHECK(expectedMiddlePng[pixelRow][pixelCol].alpha == actualMiddlePng[pixelRow][pixelCol].alpha);
        }
    }

    // Check top.png
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"resources/doctests/compiled_emerald_lilycove/expected_decompiled/top.png"}));
    REQUIRE(std::filesystem::exists(parentDir / std::filesystem::path{"top.png"}));
    png::image<png::rgba_pixel> expectedTopPng{
        "resources/doctests/compiled_emerald_lilycove/expected_decompiled/top.png"};
    png::image<png::rgba_pixel> actualTopPng{parentDir / std::filesystem::path{"top.png"}};

    std::size_t expectedTopWidthInTiles = expectedTopPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t expectedTopHeightInTiles = expectedTopPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualTopWidthInTiles = actualTopPng.get_width() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;
    std::size_t actualTopHeightInTiles = actualTopPng.get_height() / porytiles_legacy::TILE_SIDE_LENGTH_PIX;

    CHECK(expectedTopWidthInTiles == actualTopWidthInTiles);
    CHECK(expectedTopHeightInTiles == actualTopHeightInTiles);

    for (std::size_t tileIndex = 0; tileIndex < actualTopWidthInTiles * actualTopHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / actualTopWidthInTiles;
        std::size_t tileCol = tileIndex % actualTopWidthInTiles;
        for (std::size_t pixelIndex = 0; pixelIndex < porytiles_legacy::TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow =
                (tileRow * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex / porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol =
                (tileCol * porytiles_legacy::TILE_SIDE_LENGTH_PIX) + (pixelIndex % porytiles_legacy::TILE_SIDE_LENGTH_PIX);
            CHECK(expectedTopPng[pixelRow][pixelCol].red == actualTopPng[pixelRow][pixelCol].red);
            CHECK(expectedTopPng[pixelRow][pixelCol].green == actualTopPng[pixelRow][pixelCol].green);
            CHECK(expectedTopPng[pixelRow][pixelCol].blue == actualTopPng[pixelRow][pixelCol].blue);
            CHECK(expectedTopPng[pixelRow][pixelCol].alpha == actualTopPng[pixelRow][pixelCol].alpha);
        }
    }

    // Check attributes.csv
    porytiles_legacy::doctestAssertFileLinesIdentical(
        std::filesystem::path{"resources/doctests/compiled_emerald_lilycove/expected_decompiled/attributes.csv"},
        parentDir / std::filesystem::path{"attributes.csv"});

    std::filesystem::remove_all(parentDir);
}

TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly") {
    SUBCASE("it should work for regular tiles") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 3;
        ctx.fieldmapConfig.numPalettesTotal = 6;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/error_tooManyUniqueColorsInTile_regular";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 6);
    }

    SUBCASE("it should work for anim tiles") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 3;
        ctx.fieldmapConfig.numPalettesTotal = 6;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/error_tooManyUniqueColorsInTile_anim";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 4);
    }
}

TEST_CASE("error_invalidAlphaValue should trigger correctly for regular tiles") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/error_invalidAlphaValue";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
}

TEST_CASE("error_animFrameWasNotAPng should trigger correctly when an anim frame is missing") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/error_animFrameWasNotAPng";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "found anim frame that was not a png", porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
}

TEST_CASE("error_allThreeLayersHadNonTransparentContent should trigger correctly when a dual-layer inference fails") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.compilerConfig.tripleLayer = false;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/error_allThreeLayersHadNonTransparentContent";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during layered tile import",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
}

TEST_CASE("error_invalidCsvRowFormat should trigger correctly when a row format is invalid") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Emerald row format, missing field") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/incorrect_row_format_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }
    SUBCASE("Firered row format, missing field") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/incorrect_row_format_2.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_unknownMetatileBehavior should trigger correctly when a row has an unrecognized behavior") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Emerald row format, missing metatile behavior") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/unknown_behavior_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_duplicateAttribute should trigger correctly when two rows specify the same metatile id") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Duplicate metatile definition test 1") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/duplicate_definition_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_invalidTerrainType should trigger correctly when a row specifies an invalid TerrainType") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid TerrainType test 1") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/invalid_terrain_type_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }
}

TEST_CASE("error_invalidEncounterType should trigger correctly when a row specifies an invalid EncounterType") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid EncounterType test 1") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/invalid_encounter_type_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular primary tiles") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "too many unique colors total", porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular secondary tiles") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/simple_metatiles_1";
    ctx.compilerSrcPaths.secondarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "too many unique colors total", porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_missingRequiredAnimFrameFile should trigger correctly in both cases:") {
    SUBCASE("when an anim frame is missing") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 1;
        ctx.fieldmapConfig.numPalettesTotal = 2;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_skipCase";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.printDieMsg = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim1 missing required anim frame file 01.png",
                             porytiles_legacy::PorytilesException);
    }

    SUBCASE("when there are no regular frames supplied") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 1;
        ctx.fieldmapConfig.numPalettesTotal = 2;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_keyOnlyCase";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.printDieMsg = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim1 missing required anim frame file 00.png",
                             porytiles_legacy::PorytilesException);
    }
}

TEST_CASE("fatalerror_missingKeyFrameFile should trigger correctly when there is no key frame supplied") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/fatalerror_missingKeyFrameFile";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim1 missing key frame file",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame width "
          "is mismatched") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_widthCase";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "anim anim1 frame 01.png dimension width mismatch",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame height "
          "is mismatched") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_heightCase";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "anim anim1 frame 02.png dimension height mismatch",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_transparentKeyFrameTile should trigger when an anim has a transparent tile") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_transparentKeyFrameTile";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim1 had a transparent key frame tile",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE(
    "fatalerror_duplicateKeyFrameTile should trigger when two different animations have a duplicate key frame tile") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/fatalerror_duplicateKeyFrameTile";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim2 had a duplicate key frame tile",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_keyFramePresentInPairedPrimary should trigger when an animation key frame tile is present in the "
          "paired primary tileset") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/primary";
    ctx.compilerSrcPaths.secondarySourcePath =
        "resources/doctests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/secondary";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.printDieMsg = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "animation anim1 key frame tile present in paired primary",
                         porytiles_legacy::PorytilesException);
}

TEST_CASE("fatalerror_invalidAttributesCsvHeader should trigger when an attributes file is missing a header") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Completely missing header") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/missing_header_1.csv"),
                             "resources/doctests/csv/missing_header_1.csv: incorrect header row format",
                             porytiles_legacy::PorytilesException);
    }

    SUBCASE("Header missing id field") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/missing_header_2.csv"),
                             "resources/doctests/csv/missing_header_2.csv: incorrect header row format",
                             porytiles_legacy::PorytilesException);
    }

    SUBCASE("Header missing behavior field") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/missing_header_3.csv"),
                             "resources/doctests/csv/missing_header_3.csv: incorrect header row format",
                             porytiles_legacy::PorytilesException);
    }

    SUBCASE("Header has terrainType but missing encounterType") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/missing_header_4.csv"),
                             "resources/doctests/csv/missing_header_4.csv: incorrect header row format",
                             porytiles_legacy::PorytilesException);
    }
}

TEST_CASE(
    "fatalerror_invalidIdInCsv should trigger when the id column in attribute csv contains a non-integral value") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid integer format 1") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/invalid_id_column_1.csv"),
                             "resources/doctests/csv/invalid_id_column_1.csv: invalid id foo",
                             porytiles_legacy::PorytilesException);
    }

    SUBCASE("Invalid integer format 2") {
        CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "resources/doctests/csv/invalid_id_column_2.csv"),
                             "resources/doctests/csv/invalid_id_column_2.csv: invalid id 6bar",
                             porytiles_legacy::PorytilesException);
    }
}

TEST_CASE("fatalerror_invalidBehaviorValue should trigger when the metatile behavior header has a non-integral "
          "behavior value") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;

    SUBCASE("Invalid integer format 1") {
        std::ifstream behaviorFile{"resources/doctests/metatile_behaviors_invalid_1.h"};
        CHECK_THROWS_WITH_AS(
            porytiles_legacy::importMetatileBehaviorHeader(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorFile),
            "invalid behavior value foo", porytiles_legacy::PorytilesException);
        behaviorFile.close();
    }

    SUBCASE("Invalid integer format 2") {
        std::ifstream behaviorFile{"resources/doctests/metatile_behaviors_invalid_2.h"};
        CHECK_THROWS_WITH_AS(
            porytiles_legacy::importMetatileBehaviorHeader(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorFile),
            "invalid behavior value 6bar", porytiles_legacy::PorytilesException);
        behaviorFile.close();
    }
}

TEST_CASE("warn_colorPrecisionLoss should trigger correctly when a color collapses") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnColorPrecisionLoss, porytiles_legacy::DiagLevel::Error);
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/warn_colorPrecisionLoss";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnColorPrecisionLoss) == 3);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 3);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Note) == 3);
}

TEST_CASE("warn_keyFrameNoMatchingTile should trigger correctly when a key frame tile is not used") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnKeyFrameNoMatchingTile, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during primary tile assignment",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnKeyFrameNoMatchingTile) == 2);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnKeyFrameNoMatchingTile, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath =
            "resources/doctests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during secondary tile assignment",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnKeyFrameNoMatchingTile) == 2);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
    }
}

TEST_CASE("warn_tooManyAttributesForTargetGame should correctly warn") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnAttributeFormatMismatch, porytiles_legacy::DiagLevel::Error);
    ctx.targetBaseGame = porytiles_legacy::TargetBaseGame::EMERALD;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
    CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                             "resources/doctests/csv/correct_2.csv"),
                         "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnAttributeFormatMismatch) == 1);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
}

TEST_CASE("warn_tooFewAttributesForTargetGame should correctly warn") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnAttributeFormatMismatch, porytiles_legacy::DiagLevel::Error);
    ctx.targetBaseGame = porytiles_legacy::TargetBaseGame::FIRERED;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
    CHECK_THROWS_WITH_AS(porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                             "resources/doctests/csv/correct_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnAttributeFormatMismatch) == 1);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
}

TEST_CASE("warn_attributesFileNotFound should correctly warn") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnMissingAttributesCsv, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_attributesFileNotFound/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during primary attributes import",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnMissingAttributesCsv) == 1);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnMissingAttributesCsv, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_attributesFileNotFound/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath =
            "resources/doctests/errors_and_warnings/warn_attributesFileNotFound/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.printDieMsg = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during secondary attributes import",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnMissingAttributesCsv) == 1);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }
}

TEST_CASE("warn_unusedAttribute should correctly warn") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnUnusedAttribute, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/warn_unusedAttribute/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during layered tile import",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnUnusedAttribute) == 1);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnUnusedAttribute, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_unusedAttribute/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath =
            "resources/doctests/errors_and_warnings/warn_unusedAttribute/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during layered tile import",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnUnusedAttribute) == 1);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a dual layer primary set") {
        porytiles_legacy::PorytilesContext ctx{};
        ctx.printDieMsg = false;
        auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->EnableAtLevel(porytiles_legacy::WarnUnusedAttribute, porytiles_legacy::DiagLevel::Error);
        ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerConfig.tripleLayer = false;
        ctx.compilerSrcPaths.primarySourcePath =
            "resources/doctests/errors_and_warnings/warn_unusedAttribute/dual/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

        CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during layered tile import",
                             porytiles_legacy::PorytilesException);
        CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnUnusedAttribute) == 1);
        CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 1);
    }
}

TEST_CASE("warn_nonTransparentRgbaCollapsedToTransparentBgr should trigger correctly when a color collapses") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnTransparencyCollapse, porytiles_legacy::DiagLevel::Error);
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/warn_nonTransparentRgbaCollapsedToTransparentBgr";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnTransparencyCollapse) == 2);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 2);
}

TEST_CASE("warn_unusedManualPalColor should trigger correctly") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnUnusedManualPalColor, porytiles_legacy::DiagLevel::Error);
    ctx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "resources/doctests/errors_and_warnings/warn_unusedManualPalColor";
    ctx.compilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors generated during tile normalization",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnUnusedManualPalColor) == 4);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 4);
}

TEST_CASE("warn_indexOutOfRangeWarnings should trigger correctly") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    auto engine = std::make_unique<porytiles_legacy::DiagEngine>(std::make_unique<porytiles_legacy::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnTileIndexOutOfRange, porytiles_legacy::DiagLevel::Error);
    ctx.diag->EnableAtLevel(porytiles_legacy::WarnPaletteIndexOutOfRange, porytiles_legacy::DiagLevel::Error);
    ctx.subcommand = porytiles_legacy::Subcommand::DECOMPILE_SECONDARY;
    ctx.decompilerSrcPaths.primarySourcePath =
        "resources/doctests/errors_and_warnings/warn_indexOutOfRangeWarnings/general";
    ctx.decompilerSrcPaths.secondarySourcePath =
        "resources/doctests/errors_and_warnings/warn_indexOutOfRangeWarnings/petalburg";
    ctx.decompilerSrcPaths.metatileBehaviors = "resources/doctests/metatile_behaviors.h";

    CHECK_THROWS_WITH_AS(porytiles_legacy::drive(ctx), "errors encountered while decompiling tileset",
                         porytiles_legacy::PorytilesException);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnTileIndexOutOfRange) == 8);
    CHECK(ctx.diag->InFlightCountFor(porytiles_legacy::WarnPaletteIndexOutOfRange) == 8);
    CHECK(ctx.diag->InFlightCountForLevel(porytiles_legacy::DiagLevel::Error) == 16);
}
#endif // DOCTEST_CONFIG_DISABLE
