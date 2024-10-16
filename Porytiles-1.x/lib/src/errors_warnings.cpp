#include "errors_warnings.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>
#include <tuple>

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include "compiler.h"
#include "driver.h"
#include "importer.h"
#include "logger.h"
#include "porytiles_exception.h"
#include "types.h"
#include "utilities.h"

namespace porytiles {

// Compilation warnings
const char *const WARN_COLOR_PRECISION_LOSS = "color-precision-loss";
const char *const WARN_KEY_FRAME_NO_MATCHING_TILE = "key-frame-no-matching-tile";
const char *const WARN_USED_TRUE_COLOR_MODE = "used-true-color-mode";
const char *const WARN_ATTRIBUTE_FORMAT_MISMATCH = "attribute-format-mismatch";
const char *const WARN_MISSING_ATTRIBUTES_CSV = "missing-attributes-csv";
const char *const WARN_UNUSED_ATTRIBUTE = "unused-attribute";
const char *const WARN_TRANSPARENCY_COLLAPSE = "transparency-collapse";
const char *const WARN_ASSIGN_CACHE_OVERRIDE = "assign-cache-override";
const char *const WARN_INVALID_ASSIGN_CACHE = "invalid-assign-cache";
const char *const WARN_MISSING_ASSIGN_CACHE = "missing-assign-cache";

// Decompilation warnings
const char *const WARN_TILE_INDEX_OUT_OF_RANGE = "tile-index-out-of-range";
const char *const WARN_PALETTE_INDEX_OUT_OF_RANGE = "palette-index-out-of-range";

static std::string getTilePrettyString(const RGBATile &tile)
{
  // TODO : display indexes according to offsets? (so they match up with Porymap?)
  std::string tileString = "";
  if (tile.type == TileType::LAYERED) {
    tileString = fmt::format("metatile 0x{:x} ({}), {}, {}", tile.metatileIndex, tile.metatileIndex,
                             layerString(tile.layer), subtileString(tile.subtile));
  }
  else if (tile.type == TileType::ANIM) {
    tileString = fmt::format("anim {}, {}, frame {}", tile.anim, tile.frame, tile.tileIndex);
  }
  else if (tile.type == TileType::FREESTANDING) {
    tileString = fmt::format("tile 0x{:x} ({})", tile.tileIndex, tile.tileIndex);
  }
  else if (tile.type == TileType::PRIMER) {
    tileString = fmt::format("primer {}", tile.primer);
  }
  else {
    throw std::runtime_error{"error_warnings::getTilePrettyString unknown TileType"};
  }
  return tileString;
}

void internalerror(std::string message) { throw std::runtime_error(message); }

void internalerror_unknownCompilerMode(std::string context) { internalerror(context + " unknown CompilerMode"); }

void internalerror_unknownDecompilerMode(std::string context) { internalerror(context + " unknown DecompilerMode"); }

void internalerror_unknownSubcommand(std::string context) { internalerror(context + " unknown Subcommand"); }

void error_freestandingDimensionNotDivisibleBy8(ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                std::string dimensionName, png::uint_32 dimension)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("source tiles PNG {} `{}' was not divisible by 8", dimensionName,
           fmt::styled(dimension, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_animDimensionNotDivisibleBy8(ErrorsAndWarnings &err, std::string animName, std::string frame,
                                        std::string dimensionName, png::uint_32 dimension)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("anim PNG {} `{}' was not divisible by 8", dimensionName, fmt::styled(dimension, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 height)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer source PNG height `{}' was not divisible by 16", layerString(layer),
           fmt::styled(height, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_layerWidthNeq128(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 width)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer source PNG width `{}' was not {}", layerString(layer), fmt::styled(width, fmt::emphasis::bold),
           METATILE_SHEET_WIDTH);
    pt_println(stderr, "");
  }
}

void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("bottom, middle, top layer source PNG heights `{}, {}, {}' were not equivalent",
           fmt::styled(bottom, fmt::emphasis::bold), fmt::styled(middle, fmt::emphasis::bold),
           fmt::styled(top, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string &animation, const std::string &file)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("animation `{}' frame file `{}' was not a valid PNG file", fmt::styled(animation, fmt::emphasis::bold),
           fmt::styled(file, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    std::string tileString = getTilePrettyString(tile);
    pt_err("too many unique colors, threw at `{}' subtile pixel col {}, row {}",
           fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(col, fmt::emphasis::bold),
           fmt::styled(row, fmt::emphasis::bold));
    pt_note("cannot have more than {} unique colors, including the transparency color",
            fmt::styled(PAL_SIZE, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha, std::size_t row,
                             std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    std::string tileString = getTilePrettyString(tile);
    pt_err("invalid alpha value `{}' at `{}' subtile pixel col {}, row {}", fmt::styled(alpha, fmt::emphasis::bold),
           fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(col, fmt::emphasis::bold),
           fmt::styled(row, fmt::emphasis::bold));
    pt_note("alpha value must be either {} for opaque or {} for transparent",
            fmt::styled(ALPHA_OPAQUE, fmt::emphasis::bold), fmt::styled(ALPHA_TRANSPARENT, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_allThreeLayersHadNonTransparentContent(ErrorsAndWarnings &err, std::size_t metatileIndex)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("dual-layer inference failed for metatile {}, all three layers had non-transparent content", metatileIndex);
    pt_println(stderr, "");
  }
}

void error_invalidCsvRowFormat(ErrorsAndWarnings &err, std::string filePath, std::size_t line)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: on line {}: provided columns did not match header", filePath, line);
    pt_println(stderr, "");
  }
}

void error_unknownMetatileBehavior(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string behavior)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: on line {}: unknown metatile behavior `{}'", filePath, line,
           fmt::styled(behavior, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_unknownMetatileBehaviorValue(ErrorsAndWarnings &err, std::string filePath, std::size_t entry,
                                        std::uint16_t behaviorValue)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: in metatile entry {}: unmapped metatile behavior value `{}'", filePath, entry,
           fmt::styled(behaviorValue, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_duplicateAttribute(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::size_t id,
                              std::size_t previousLine)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: on line {}: duplicate entry for metatile `{}', first definition on line {}", filePath, line,
           fmt::styled(id, fmt::emphasis::bold), previousLine);
    pt_println(stderr, "");
  }
}

void error_invalidTerrainType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: on line {}: invalid TerrainType `{}'", filePath, line, fmt::styled(type, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void error_invalidEncounterType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{}: on line {}: invalid EncounterType `{}'", filePath, line, fmt::styled(type, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void fatalerror(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode, std::string message)
{
  if (err.printErrors) {
    pt_fatal_err("{}", message);
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), message);
}

void fatalerror(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs, DecompilerMode mode,
                std::string message)
{
  if (err.printErrors) {
    pt_fatal_err("{}", message);
    pt_println(stderr, "");
  }
  die_decompilationTerminated(err, srcs.modeBasedSrcPath(mode), message);
}

void fatalerror(const ErrorsAndWarnings &err, std::string errorMessage)
{
  if (err.printErrors) {
    pt_fatal_err("{}", errorMessage);
  }
  throw PorytilesException{errorMessage};
}

void fatalerror_unrecognizedOption(const ErrorsAndWarnings &err, std::string option, Subcommand subcommand)
{
  if (err.printErrors) {
    pt_fatal_err("unrecognized option `{}' for subcommand `{}'", option, subcommandString(subcommand));
    pt_println(stderr, "Try `{} --help' for usage information.", subcommandString(subcommand));
  }
  throw PorytilesException{
      fmt::format("unrecognized option `{}' for subcommand `{}'", option, subcommandString(subcommand))};
}

void fatalerror_missingRequiredAnimFrameFile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                             CompilerMode mode, const std::string &animation, std::size_t index)
{
  std::string file = std::to_string(index) + ".png";
  if (index < 10) {
    file = "0" + file;
  }
  if (err.printErrors) {
    pt_fatal_err("animation `{}' was missing expected frame file `{}'", fmt::styled(animation, fmt::emphasis::bold),
                 fmt::styled(file, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("animation {} missing required anim frame file {}", animation, file));
}

void fatalerror_missingKeyFrameFile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                    const std::string &animation)
{
  if (err.printErrors) {
    pt_fatal_err("animation `{}' was missing key frame file", fmt::styled(animation, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("animation {} missing key frame file", animation));
}

void fatalerror_tooManyUniqueColorsTotal(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                         CompilerMode mode, std::size_t allowed, std::size_t found)
{
  if (err.printErrors) {
    pt_fatal_err("too many unique colors in {} tileset", compilerModeString(mode));
    pt_note("{} allowed based on fieldmap configuration, but found {}", fmt::styled(allowed, fmt::emphasis::bold),
            fmt::styled(found, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("too many unique colors total"));
}

void fatalerror_animFrameDimensionsDoNotMatchOtherFrames(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                         CompilerMode mode, std::string animName, std::string frame,
                                                         std::string dimensionName, png::uint_32 dimension)
{
  if (err.printErrors) {
    pt_fatal_err("animation `{}' frame `{}' {} `{}' did not match previous frame {}s",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(frame, fmt::emphasis::bold), dimensionName,
                 fmt::styled(dimension, fmt::emphasis::bold), dimensionName);
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("anim {} frame {} dimension {} mismatch", animName, frame, dimensionName));
}

void fatalerror_tooManyUniqueTiles(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                   std::size_t numTiles, std::size_t maxAllowedTiles)
{
  if (err.printErrors) {
    pt_fatal_err("unique tile count `{}' exceeded limit of `{}'", fmt::styled(numTiles, fmt::emphasis::bold),
                 fmt::styled(maxAllowedTiles, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("too many unique tiles in {} tileset", compilerModeString(mode)));
}

void fatalerror_assignExploreCutoffReached(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                           CompilerMode mode, AssignAlgorithm algo, std::size_t maxRecurses)
{
  if (err.printErrors) {
    pt_fatal_err("{} palette assignment exploration reached node cutoff", assignAlgorithmString(algo));
    pt_println(stderr, "");
  }
  die_compilationTerminatedFailHard(err, srcs.modeBasedSrcPath(mode), "too many assignment recurses");
}

void fatalerror_noPossiblePaletteAssignment(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                            CompilerMode mode)
{
  if (err.printErrors) {
    pt_fatal_err("no possible palette assignment exists, given the current assign search params");
    pt_println(stderr, "");
  }
  die_compilationTerminatedFailHard(err, srcs.modeBasedSrcPath(mode), "no possible palette assignment");
}

void fatalerror_tooManyMetatiles(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                 std::size_t numMetatiles, std::size_t metatileLimit)
{
  if (err.printErrors) {
    pt_fatal_err("source metatile count of `{}' exceeded the {} tileset limit of `{}'",
                 fmt::styled(numMetatiles, fmt::emphasis::bold), compilerModeString(mode),
                 fmt::styled(metatileLimit, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(
      err, srcs.modeBasedSrcPath(mode),
      fmt::format("too many {} metatiles: {} > {}", compilerModeString(mode), numMetatiles, metatileLimit));
}

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                          CompilerMode mode, std::string field, std::size_t primary, std::size_t total)
{
  if (err.printErrors) {
    pt_fatal_err("invalid configuration {}InPrimary `{}' exceeded {}Total `{}'", field,
                 fmt::styled(primary, fmt::emphasis::bold), field, fmt::styled(total, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("invalid config {}: {} > {}", field, primary, total));
}

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                          DecompilerMode mode, std::string field, std::size_t primary,
                                          std::size_t total)
{
  if (err.printErrors) {
    pt_fatal_err("invalid configuration {}InPrimary `{}' exceeded {}Total `{}'", field,
                 fmt::styled(primary, fmt::emphasis::bold), field, fmt::styled(total, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_decompilationTerminated(err, srcs.modeBasedSrcPath(mode),
                              fmt::format("invalid config {}: {} > {}", field, primary, total));
}

void fatalerror_transparentKeyFrameTile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                        CompilerMode mode, std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation `{}' key frame tile `{}' was transparent", fmt::styled(animName, fmt::emphasis::bold),
                 fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("this is not allowed, since there would be no way to tell if a transparent user-provided tile on the layer "
            "sheet");
    pt_println(stderr, "      referred to the true index 0 transparent tile, or if it was a reference into this "
                       "particular animation");
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("animation {} had a transparent key frame tile", animName));
}

void fatalerror_duplicateKeyFrameTile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                      std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation `{}' key frame tile `{}' duplicated another key frame tile in this tileset",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("key frame tiles must be unique within a tileset, and unique across any paired primary tileset");
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("animation {} had a duplicate key frame tile", animName));
}

void fatalerror_keyFramePresentInPairedPrimary(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                               CompilerMode mode, std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation `{}' key frame tile `{}' was present in the paired primary tileset",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("this is an error because it renders the animation inoperable, any reference to the key tile in the");
    pt_println(stderr,
               "      secondary layer sheet will be linked to primary tileset instead of the intended animation");
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("animation {} key frame tile present in paired primary", animName));
}

void fatalerror_invalidAttributesCsvHeader(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                           CompilerMode mode, std::string filePath)
{
  if (err.printErrors) {
    pt_fatal_err("{}: incorrect header row format", filePath);
    pt_note("valid headers are `{}' or `{}'", fmt::styled("id,behavior", fmt::emphasis::bold),
            fmt::styled("id,behavior,terrainType,encounterType", fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("{}: incorrect header row format", filePath));
}

void fatalerror_invalidIdInCsv(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                               std::string filePath, std::string id, std::size_t line)
{
  if (err.printErrors) {
    pt_fatal_err("{}: invalid value `{}' for column `{}' at line {}", filePath, fmt::styled(id, fmt::emphasis::bold),
                 fmt::styled("id", fmt::emphasis::bold), line);
    pt_note("column `{}' must contain an integral value (both decimal and hexidecimal notations are permitted)",
            fmt::styled("id", fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("{}: invalid id {}", filePath, id));
}

static void fatalerror_invalidBehaviorValueHelper(const ErrorsAndWarnings &err, const CompilerSourcePaths *compilerSrcs,
                                                  const DecompilerSourcePaths *decompilerSrcs,
                                                  const CompilerMode *compilerMode,
                                                  const DecompilerMode *decompilerMode, std::string behavior,
                                                  std::string value, std::size_t line)
{
  if (err.printErrors) {
    pt_fatal_err("invalid value `{}' for behavior `{}' defined at line {}", fmt::styled(value, fmt::emphasis::bold),
                 fmt::styled(behavior, fmt::emphasis::bold), line);
    pt_note("behavior must be an integral value (both decimal and hexidecimal notations are permitted)",
            fmt::styled("id", fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  if (compilerMode != nullptr && compilerSrcs != nullptr) {
    die_compilationTerminated(err, compilerSrcs->modeBasedSrcPath(*compilerMode),
                              fmt::format("invalid behavior value {}", value));
  }
  else if (decompilerMode != nullptr && decompilerSrcs != nullptr) {
    die_decompilationTerminated(err, decompilerSrcs->modeBasedSrcPath(*decompilerMode),
                                fmt::format("invalid behavior value {}", value));
  }
  else {
    internalerror("errors_warnings::fatalerror_invalidBehaviorValueHelper invalid call parameters");
  }
}

void fatalerror_invalidBehaviorValue(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                     std::string behavior, std::string value, std::size_t line)
{
  fatalerror_invalidBehaviorValueHelper(err, &srcs, nullptr, &mode, nullptr, behavior, value, line);
}

void fatalerror_invalidBehaviorValue(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                     DecompilerMode mode, std::string behavior, std::string value, std::size_t line)
{
  fatalerror_invalidBehaviorValueHelper(err, nullptr, &srcs, nullptr, &mode, behavior, value, line);
}

void fatalerror_assignCacheSyntaxError(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                       const CompilerMode &mode, std::string line, std::size_t lineNumber,
                                       std::string path)
{
  if (err.printErrors) {
    pt_fatal_err("{}: invalid syntax `{}' at line {}", path, fmt::styled(line, fmt::emphasis::bold), lineNumber);
    pt_note("`assign.cache' expected line syntax is: {}", fmt::styled("key=value", fmt::emphasis::bold));
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("invalid assign syntax {}", line));
}

void fatalerror_assignCacheInvalidKey(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                      const CompilerMode &mode, std::string key, std::size_t lineNumber,
                                      std::string path)
{
  if (err.printErrors) {
    pt_fatal_err("{}: invalid key `{}' at line {}", path, fmt::styled(key, fmt::emphasis::bold), lineNumber);
    pt_note("`assign.cache' expects keys to match the color assignment config options");
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("invalid assign key {}", key));
}

void fatalerror_assignCacheInvalidValue(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                        const CompilerMode &mode, std::string key, std::string value,
                                        std::size_t lineNumber, std::string path)
{
  if (err.printErrors) {
    pt_fatal_err("{}: invalid value `{}' for key `{}' at line {}", path, fmt::styled(value, fmt::emphasis::bold),
                 fmt::styled(key, fmt::emphasis::bold), lineNumber);
    pt_println(stderr, "");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                            fmt::format("invalid assign value {} for key {}", value, key));
}

void fatalerror_paletteAssignParamSearchMatrixFailed(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                     const CompilerMode &mode)
{
  if (err.printErrors) {
    pt_fatal_err("palette assignment parameter search matrix failed to find any suitable parameters");
    pt_note("please see the following wiki page for help with working through this error:");
    // TODO 1.0.0 : fill in wiki page link
    pt_println(stderr, "      https://wiki-page-link-goes-here.com");
  }
  die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("palette assign param search matrix failed"));
}

void fatalerror_noImpliedLayerType(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs, DecompilerMode mode)
{
  if (err.printErrors) {
    pt_fatal_err("no layer type was implied by the supplied metatiles and attributes");
    pt_note("either you forgot to supply the correct `-target-base-game' option, or a file is corrupted");
    pt_println(stderr, "");
  }
  die_decompilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("no implied layer type"));
}

static void printWarning(ErrorsAndWarnings &err, WarningMode warningMode, const std::string_view &warningName,
                         const std::string &message)
{
  if (warningMode == WarningMode::ERR) {
    err.errCount++;
    if (err.printErrors) {
      pt_err(
          "{} [{}]", message,
          fmt::styled(fmt::format("-Werror={}", warningName), fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)));
    }
  }
  else if (warningMode == WarningMode::WARN) {
    err.warnCount++;
    if (err.printErrors) {
      pt_warn(
          "{} [{}]", message,
          fmt::styled(fmt::format("-W{}", warningName), fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta)));
    }
  }
}

void warn_colorPrecisionLoss(ErrorsAndWarnings &err, CompilerMode mode, const RGBATile &tile, std::size_t row,
                             std::size_t col, const BGR15 &bgr, const RGBA32 &rgba,
                             const std::tuple<RGBA32, RGBATile, std::size_t, std::size_t> &previousRgba)
{
  std::string tileString = getTilePrettyString(tile);
  std::string message = fmt::format("color `{}' at {} `{}' subtile pixel col {}, row {} collapsed to duplicate BGR",
                                    fmt::styled(rgba.jasc(), fmt::emphasis::bold), compilerModeString(mode),
                                    fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(col, fmt::emphasis::bold),
                                    fmt::styled(row, fmt::emphasis::bold));
  printWarning(err, err.colorPrecisionLoss, WARN_COLOR_PRECISION_LOSS, message);
  if (err.printErrors && err.colorPrecisionLoss != WarningMode::OFF) {
    std::string previousTileString = getTilePrettyString(std::get<1>(previousRgba));
    pt_note("previously saw `{}' at `{}' subtile pixel col {}, row {}",
            fmt::styled(std::get<0>(previousRgba).jasc(), fmt::emphasis::bold),
            fmt::styled(previousTileString, fmt::emphasis::bold),
            fmt::styled(std::get<3>(previousRgba), fmt::emphasis::bold),
            fmt::styled(std::get<2>(previousRgba), fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void warn_keyFrameNoMatchingTile(ErrorsAndWarnings &err, std::string animName, std::size_t tileIndex)
{
  std::string message =
      fmt::format("animation `{}' key frame tile `{}' was not present in any metatile entries",
                  fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
  printWarning(err, err.keyFrameNoMatchingTile, WARN_KEY_FRAME_NO_MATCHING_TILE, message);
  if (err.printErrors && err.keyFrameNoMatchingTile != WarningMode::OFF) {
    pt_println(stderr, "");
  }
}

void warn_usedTrueColorMode(ErrorsAndWarnings &err)
{
  std::string message = "`true-color' mode requires Porymap minimum version 5.2.0";
  printWarning(err, err.usedTrueColorMode, WARN_USED_TRUE_COLOR_MODE, message);
  if (err.printErrors && err.usedTrueColorMode != WarningMode::OFF) {
    pt_note("if you are using an older version of Porymap, either update it or disable `true-color' mode");
    pt_println(stderr, "");
  }
}

void warn_tooManyAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame)
{
  printWarning(err, err.attributeFormatMismatch, WARN_ATTRIBUTE_FORMAT_MISMATCH,
               fmt::format("{}: too many attribute columns for base game `{}'", filePath,
                           fmt::styled(targetBaseGameString(baseGame), fmt::emphasis::bold)));
  if (err.printErrors && err.attributeFormatMismatch != WarningMode::OFF) {
    pt_println(stderr, "");
  }
}

void warn_tooFewAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame)
{
  printWarning(err, err.attributeFormatMismatch, WARN_ATTRIBUTE_FORMAT_MISMATCH,
               fmt::format("{}: too few attribute columns for base game `{}'", filePath,
                           fmt::styled(targetBaseGameString(baseGame), fmt::emphasis::bold)));
  if (err.printErrors && err.attributeFormatMismatch != WarningMode::OFF) {
    pt_note("unspecified columns will receive default values");
    pt_println(stderr, "");
  }
}

void warn_attributesFileNotFound(ErrorsAndWarnings &err, std::string filePath)
{
  printWarning(err, err.missingAttributesCsv, WARN_MISSING_ATTRIBUTES_CSV,
               fmt::format("{}: attributes file did not exist", filePath));
  if (err.printErrors && err.missingAttributesCsv != WarningMode::OFF) {
    pt_note("all attributes will receive default or inferred values");
    pt_println(stderr, "");
  }
}

void warn_unusedAttribute(ErrorsAndWarnings &err, std::size_t metatileId, std::size_t metatileCount,
                          std::string sourcePath)
{
  printWarning(
      err, err.unusedAttribute, WARN_UNUSED_ATTRIBUTE,
      fmt::format("found attribute for nonexistent metatile ID {}", fmt::styled(metatileId, fmt::emphasis::bold)));
  if (err.printErrors && err.unusedAttribute != WarningMode::OFF) {
    pt_note("{} metatiles found at source path {}", metatileCount, fmt::styled(sourcePath, fmt::emphasis::bold));
    pt_println(stderr, "");
  }
}

void warn_nonTransparentRgbaCollapsedToTransparentBgr(ErrorsAndWarnings &err, CompilerMode mode, const RGBATile &tile,
                                                      std::size_t row, std::size_t col, const RGBA32 &color,
                                                      const RGBA32 &transparency)
{
  std::string tileString = getTilePrettyString(tile);
  printWarning(
      err, err.transparencyCollapse, WARN_TRANSPARENCY_COLLAPSE,
      fmt::format("color `{}' at {} `{}' subtile pixel col {}, row {} collapsed to transparent under BGR conversion",
                  fmt::styled(color.jasc(), fmt::emphasis::bold), compilerModeString(mode),
                  fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(col, fmt::emphasis::bold),
                  fmt::styled(row, fmt::emphasis::bold)));
  if (err.printErrors && err.transparencyCollapse != WarningMode::OFF) {
    pt_note(
        "if you did not intend this to be a transparent pixel, please edit the color on the respective layer sheet");
    pt_println(stderr, "");
  }
}

void warn_assignCacheOverride(ErrorsAndWarnings &err, CompilerMode mode, const CompilerConfig &config, std::string path)
{
  printWarning(
      err, err.assignCacheOverride, WARN_ASSIGN_CACHE_OVERRIDE,
      fmt::format("{}: ignoring {} `assign.cache' due to command line override", path, compilerModeString(mode)));
  if (err.printErrors && err.assignCacheOverride != WarningMode::OFF) {
    if (mode == CompilerMode::PRIMARY) {
      pt_note("assign-algorithm={}", assignAlgorithmString(config.primaryAssignAlgorithm));
      pt_note("explored-node-cutoff={}", config.primaryExploredNodeCutoff);
      if (config.primaryBestBranches == SIZE_MAX) {
        pt_note("best-branches={}", "SIZE_MAX");
      }
      else {
        if (config.primarySmartPrune) {
          pt_note("best-branches={}", "smart-prune");
        }
        else {
          pt_note("best-branches={}", config.primaryBestBranches);
        }
      }
    }
    else if (mode == CompilerMode::SECONDARY) {
      pt_note("assign-algorithm={}", assignAlgorithmString(config.secondaryAssignAlgorithm));
      pt_note("explored-node-cutoff={}", config.secondaryExploredNodeCutoff);
      if (config.secondaryBestBranches == SIZE_MAX) {
        pt_note("best-branches={}", "SIZE_MAX");
      }
      else {
        if (config.secondarySmartPrune) {
          pt_note("best-branches={}", "smart-prune");
        }
        else {
          pt_note("best-branches={}", config.secondarySmartPrune);
        }
      }
    }
    pt_println(stderr, "");
  }
}

void warn_invalidAssignCache(ErrorsAndWarnings &err, const CompilerConfig &config, std::string path)
{
  printWarning(err, err.invalidAssignCache, WARN_INVALID_ASSIGN_CACHE,
               fmt::format("{}: cached compilation settings failed", path));
  if (err.printErrors && err.invalidAssignCache != WarningMode::OFF) {
    pt_note("running full parameter search matrix, this may take awhile...");
    pt_println(stderr, "");
  }
}

void warn_missingAssignCache(ErrorsAndWarnings &err, const CompilerConfig &config, std::string path)
{
  printWarning(err, err.missingAssignCache, WARN_MISSING_ASSIGN_CACHE,
               fmt::format("{}: cached compilation settings not found", path));
  if (err.printErrors && err.missingAssignCache != WarningMode::OFF) {
    pt_note("running full parameter search matrix, this may take awhile...");
    pt_println(stderr, "");
  }
}

void warn_tileIndexOutOfRange(ErrorsAndWarnings &err, DecompilerMode mode, std::size_t tileIndex,
                              std::size_t tilesheetSize, const RGBATile &tile)
{
  std::string tileString = getTilePrettyString(tile);
  printWarning(err, err.tileIndexOutOfRange, WARN_TILE_INDEX_OUT_OF_RANGE,
               fmt::format("{} `{}': tile index {} out of range (sheet size = {})", decompilerModeString(mode),
                           fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold),
                           tilesheetSize));
  if (err.printErrors && err.tileIndexOutOfRange != WarningMode::OFF) {
    pt_note("substituting primary tile 0 (transparent tile) so decompilation can continue");
    pt_println(stderr, "");
  }
}

void warn_paletteIndexOutOfRange(ErrorsAndWarnings &err, DecompilerMode mode, std::size_t paletteIndex,
                                 std::size_t numPalettesTotal, const RGBATile &tile)
{
  std::string tileString = getTilePrettyString(tile);
  printWarning(err, err.paletteIndexOutOfRange, WARN_PALETTE_INDEX_OUT_OF_RANGE,
               fmt::format("{} `{}': palette index {} out of range (numPalettesTotal = {})", decompilerModeString(mode),
                           fmt::styled(tileString, fmt::emphasis::bold), fmt::styled(paletteIndex, fmt::emphasis::bold),
                           numPalettesTotal));
  if (err.printErrors && err.paletteIndexOutOfRange != WarningMode::OFF) {
    pt_note("substituting palette 0 so decompilation can continue");
    pt_println(stderr, "");
  }
}

void die(const ErrorsAndWarnings &err, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "{}", errorMessage);
  }
  throw PorytilesException{errorMessage};
}

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
  }
  throw PorytilesException{errorMessage};
}

void die_compilationTerminatedFailHard(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
  }
  std::exit(1);
}

void die_decompilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "terminating decompilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
  }
  throw PorytilesException{errorMessage};
}

void die_errorCount(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage)
{
  if (err.printErrors) {
    std::string errorStr;
    if (err.errCount == 1) {
      errorStr = "error";
    }
    else {
      errorStr = "errors";
    }
    std::string warnStr;
    if (err.warnCount == 1) {
      warnStr = "warning";
    }
    else {
      warnStr = "warnings";
    }
    if (err.warnCount > 0) {
      pt_println(stderr, "{} {} and {} {} generated.", std::to_string(err.warnCount), warnStr,
                 std::to_string(err.errCount), errorStr);
    }
    else {
      pt_println(stderr, "{} {} generated.", std::to_string(err.errCount), errorStr);
    }
    pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
  }
  throw PorytilesException{errorMessage};
}

} // namespace porytiles

/*
 * Test cases that deliberately check for end-to-end error/warning correctness go here
 */
TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly")
{
  SUBCASE("it should work for regular tiles")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_tooManyUniqueColorsInTile_regular";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 6);
  }

  SUBCASE("it should work for anim tiles")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_tooManyUniqueColorsInTile_anim";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 4);
  }
}

TEST_CASE("error_invalidAlphaValue should trigger correctly for regular tiles")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_invalidAlphaValue";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                       porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("error_animFrameWasNotAPng should trigger correctly when an anim frame is missing")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_animFrameWasNotAPng";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "found anim frame that was not a png", porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 1);
}

TEST_CASE("error_allThreeLayersHadNonTransparentContent should trigger correctly when a dual-layer inference fails")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.compilerConfig.tripleLayer = false;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_allThreeLayersHadNonTransparentContent";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                       porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("error_invalidCsvRowFormat should trigger correctly when a row format is invalid")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Emerald row format, missing field")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/incorrect_row_format_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }
  SUBCASE("Firered row format, missing field")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/incorrect_row_format_2.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 2);
  }
}

TEST_CASE("error_unknownMetatileBehavior should trigger correctly when a row has an unrecognized behavior")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Emerald row format, missing metatile behavior")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/unknown_behavior_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 2);
  }
}

TEST_CASE("error_duplicateAttribute should trigger correctly when two rows specify the same metatile id")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Duplicate metatile definition test 1")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/duplicate_definition_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 2);
  }
}

TEST_CASE("error_invalidTerrainType should trigger correctly when a row specifies an invalid TerrainType")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Invalid TerrainType test 1")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/invalid_terrain_type_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }
}

TEST_CASE("error_invalidEncounterType should trigger correctly when a row specifies an invalid EncounterType")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Invalid EncounterType test 1")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/invalid_encounter_type_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular primary tiles")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PorytilesException);
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular secondary tiles")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/simple_metatiles_1";
  ctx.compilerSrcPaths.secondarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PorytilesException);
}

TEST_CASE("fatalerror_missingRequiredAnimFrameFile should trigger correctly in both cases:")
{
  SUBCASE("when an anim frame is missing")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_skipCase";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 01.png",
                         porytiles::PorytilesException);
  }

  SUBCASE("when there are no regular frames supplied")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_keyOnlyCase";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 00.png",
                         porytiles::PorytilesException);
  }
}

TEST_CASE("fatalerror_missingKeyFrameFile should trigger correctly when there is no key frame supplied")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_missingKeyFrameFile";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing key frame file", porytiles::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame width "
          "is mismatched")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath =
      "Resources/Tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_widthCase";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 01.png dimension width mismatch",
                       porytiles::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame height "
          "is mismatched")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath =
      "Resources/Tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_heightCase";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 02.png dimension height mismatch",
                       porytiles::PorytilesException);
}

TEST_CASE("fatalerror_transparentKeyFrameTile should trigger when an anim has a transparent tile")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_transparentKeyFrameTile";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 had a transparent key frame tile",
                       porytiles::PorytilesException);
}

TEST_CASE(
    "fatalerror_duplicateKeyFrameTile should trigger when two different animations have a duplicate key frame tile")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_duplicateKeyFrameTile";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim2 had a duplicate key frame tile",
                       porytiles::PorytilesException);
}

TEST_CASE("fatalerror_keyFramePresentInPairedPrimary should trigger when an animation key frame tile is present in the "
          "paired primary tileset")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 2;
  ctx.fieldmapConfig.numPalettesTotal = 4;
  ctx.compilerSrcPaths.primarySourcePath =
      "Resources/Tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/primary";
  ctx.compilerSrcPaths.secondarySourcePath =
      "Resources/Tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/secondary";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 key frame tile present in paired primary",
                       porytiles::PorytilesException);
}

TEST_CASE("fatalerror_invalidAttributesCsvHeader should trigger when an attributes file is missing a header")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Completely missing header")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/missing_header_1.csv"),
                         "Resources/Tests/csv/missing_header_1.csv: incorrect header row format",
                         porytiles::PorytilesException);
  }

  SUBCASE("Header missing id field")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/missing_header_2.csv"),
                         "Resources/Tests/csv/missing_header_2.csv: incorrect header row format",
                         porytiles::PorytilesException);
  }

  SUBCASE("Header missing behavior field")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/missing_header_3.csv"),
                         "Resources/Tests/csv/missing_header_3.csv: incorrect header row format",
                         porytiles::PorytilesException);
  }

  SUBCASE("Header has terrainType but missing encounterType")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/missing_header_4.csv"),
                         "Resources/Tests/csv/missing_header_4.csv: incorrect header row format",
                         porytiles::PorytilesException);
  }
}

TEST_CASE("fatalerror_invalidIdInCsv should trigger when the id column in attribute csv contains a non-integral value")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("Invalid integer format 1")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/invalid_id_column_1.csv"),
                         "Resources/Tests/csv/invalid_id_column_1.csv: invalid id foo", porytiles::PorytilesException);
  }

  SUBCASE("Invalid integer format 2")
  {
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/invalid_id_column_2.csv"),
                         "Resources/Tests/csv/invalid_id_column_2.csv: invalid id 6bar", porytiles::PorytilesException);
  }
}

TEST_CASE("fatalerror_invalidBehaviorValue should trigger when the metatile behavior header has a non-integral "
          "behavior value")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;

  SUBCASE("Invalid integer format 1")
  {
    std::ifstream behaviorFile{"Resources/Tests/metatile_behaviors_invalid_1.h"};
    CHECK_THROWS_WITH_AS(porytiles::importMetatileBehaviorHeader(ctx, porytiles::CompilerMode::PRIMARY, behaviorFile),
                         "invalid behavior value foo", porytiles::PorytilesException);
    behaviorFile.close();
  }

  SUBCASE("Invalid integer format 2")
  {
    std::ifstream behaviorFile{"Resources/Tests/metatile_behaviors_invalid_2.h"};
    CHECK_THROWS_WITH_AS(porytiles::importMetatileBehaviorHeader(ctx, porytiles::CompilerMode::PRIMARY, behaviorFile),
                         "invalid behavior value 6bar", porytiles::PorytilesException);
    behaviorFile.close();
  }
}

TEST_CASE("warn_colorPrecisionLoss should trigger correctly when a color collapses")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_colorPrecisionLoss";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.colorPrecisionLoss = porytiles::WarningMode::ERR;
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                       porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 3);
}

TEST_CASE("warn_keyFrameTileDidNotAppearInAssignment should trigger correctly when a key frame tile is not used")
{
  SUBCASE("it should trigger correctly for a primary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.keyFrameNoMatchingTile = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during primary tile assignment",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 2);
  }

  SUBCASE("it should trigger correctly for a secondary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary_correct";
    ctx.compilerSrcPaths.secondarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/secondary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.keyFrameNoMatchingTile = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during secondary tile assignment",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 2);
  }
}

TEST_CASE("warn_tooManyAttributesForTargetGame should correctly warn")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;
  ctx.err.attributeFormatMismatch = porytiles::WarningMode::ERR;
  ctx.targetBaseGame = porytiles::TargetBaseGame::EMERALD;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
  CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                          "Resources/Tests/csv/correct_2.csv"),
                       "errors generated during attributes CSV parsing", porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 1);
}

TEST_CASE("warn_tooFewAttributesForTargetGame should correctly warn")
{
  porytiles::PorytilesContext ctx{};
  ctx.err.printErrors = false;
  ctx.err.attributeFormatMismatch = porytiles::WarningMode::ERR;
  ctx.targetBaseGame = porytiles::TargetBaseGame::FIRERED;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
  CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                          "Resources/Tests/csv/correct_1.csv"),
                       "errors generated during attributes CSV parsing", porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 1);
}

TEST_CASE("warn_attributesFileNotFound should correctly warn")
{
  SUBCASE("it should trigger correctly for a primary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/primary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.missingAttributesCsv = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during primary attributes import",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }

  SUBCASE("it should trigger correctly for a secondary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/primary_correct";
    ctx.compilerSrcPaths.secondarySourcePath = "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/secondary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.missingAttributesCsv = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during secondary attributes import",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }
}

TEST_CASE("warn_unusedAttribute should correctly warn")
{
  SUBCASE("it should trigger correctly for a primary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/primary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.unusedAttribute = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }

  SUBCASE("it should trigger correctly for a secondary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/primary_correct";
    ctx.compilerSrcPaths.secondarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/secondary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.unusedAttribute = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }

  SUBCASE("it should trigger correctly for a dual layer primary set")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerConfig.tripleLayer = false;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/dual/primary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.unusedAttribute = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                         porytiles::PorytilesException);
    CHECK(ctx.err.errCount == 1);
  }
}

TEST_CASE("warn_nonTransparentRgbaCollapsedToTransparentBgr should trigger correctly when a color collapses")
{
  porytiles::PorytilesContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.compilerSrcPaths.primarySourcePath =
      "Resources/Tests/errors_and_warnings/error_nonTransparentRgbaCollapsedToTransparentBgr";
  ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
  ctx.err.transparencyCollapse = porytiles::WarningMode::ERR;
  ctx.err.printErrors = false;
  ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
  ctx.compilerConfig.cacheAssign = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                       porytiles::PorytilesException);
  CHECK(ctx.err.errCount == 2);
}
