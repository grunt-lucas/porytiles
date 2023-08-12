#include "errors_warnings.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>

#include "driver.h"
#include "logger.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {

void internalerror(std::string message) { throw std::runtime_error(message); }

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::string context, std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize)
{
  internalerror("config.numPalettesInPrimary did not match primary palette set size (" +
                std::to_string(configNumPalettesPrimary) + " != " + std::to_string(primaryPalettesSize) + ")");
}

void internalerror_unknownCompilerMode(std::string context) { internalerror(context + " unknown CompilerMode"); }

void error_freestandingDimensionNotDivisibleBy8(ErrorsAndWarnings &err, const InputPaths &inputs,
                                                std::string dimensionName, png::uint_32 dimension)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("input tiles PNG {} '{}' was not divisible by 8", dimensionName,
           fmt::styled(dimension, fmt::emphasis::bold));
  }
}

void error_animDimensionNotDivisibleBy8(ErrorsAndWarnings &err, std::string animName, std::string frame,
                                        std::string dimensionName, png::uint_32 dimension)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("anim PNG {} '{}' was not divisible by 8", dimensionName, fmt::styled(dimension, fmt::emphasis::bold));
  }
}

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 height)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer input PNG height '{}' was not divisible by 16", layerString(layer),
           fmt::styled(height, fmt::emphasis::bold));
  }
}

void error_layerWidthNeq128(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 width)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer input PNG width '{}' was not 128", layerString(layer), fmt::styled(width, fmt::emphasis::bold));
  }
}

void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("bottom, middle, top layer input PNG heights '{}, {}, {}' were not equivalent",
           fmt::styled(bottom, fmt::emphasis::bold), fmt::styled(middle, fmt::emphasis::bold),
           fmt::styled(top, fmt::emphasis::bold));
  }
}

void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string &animation, const std::string &file)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("animation '{}' frame file '{}' was not a valid PNG file", fmt::styled(animation, fmt::emphasis::bold),
           fmt::styled(file, fmt::emphasis::bold));
  }
}

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err_rgbatile(tile, "too many unique colors, threw at pixel col {}, row {}", col, row);
    pt_note_rgbatile(tile, "cannot have more than {} unique colors, including the transparency color",
                     fmt::styled(PAL_SIZE, fmt::emphasis::bold));
  }
}

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha, std::size_t row,
                             std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err_rgbatile(tile, "invalid alpha value '{}' at pixel col {}, row {}", fmt::styled(alpha, fmt::emphasis::bold),
                    col, row);
    pt_note_rgbatile(tile, "alpha value must be either {} for opaque or {} for transparent",
                     fmt::styled(ALPHA_OPAQUE, fmt::emphasis::bold),
                     fmt::styled(ALPHA_TRANSPARENT, fmt::emphasis::bold));
  }
}

void error_nonTransparentRgbaCollapsedToTransparentBgr(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row,
                                                       std::size_t col, const RGBA32 &color, const RGBA32 &transparency)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err_rgbatile(tile, "color '{}' at pixel col {}, row {} collapsed to transparent under BGR conversion",
                    fmt::styled(color.jasc(), fmt::emphasis::bold), col, row);
  }
}

void error_allThreeLayersHadNonTransparentContent(ErrorsAndWarnings &err, std::size_t metatileIndex)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("dual-layer inference failed for metatile {}, all three layers had non-transparent content", metatileIndex);
  }
}

void fatalerror(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode, std::string message)
{
  if (err.printErrors) {
    pt_fatal_err("{}", message);
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), message);
}

void fatalerror_basicprefix(const ErrorsAndWarnings &err, std::string errorMessage)
{
  if (err.printErrors) {
    pt_fatal_err_prefix("{}", errorMessage);
  }
  throw PtException{errorMessage};
}

void fatalerror_invalidInputPath(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                 std::string path)
{
  if (err.printErrors) {
    pt_fatal_err_prefix("{}: input path does not exist or is not a directory", path);
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), fmt::format("invalid input path {}", path));
}

void fatalerror_missingRequiredAnimFrameFile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                             const std::string &animation, std::size_t index)
{
  std::string file = std::to_string(index) + ".png";
  if (index < 10) {
    file = "0" + file;
  }
  if (err.printErrors) {
    pt_fatal_err("animation '{}' was missing expected frame file '{}'", fmt::styled(animation, fmt::emphasis::bold),
                 fmt::styled(file, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} missing required anim frame file {}", animation, file));
}

void fatalerror_missingKeyFrameFile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                    const std::string &animation)
{
  if (err.printErrors) {
    pt_fatal_err("animation '{}' was missing key frame file", fmt::styled(animation, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} missing key frame file", animation));
}

void fatalerror_tooManyUniqueColorsTotal(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                         std::size_t allowed, std::size_t found)
{
  if (err.printErrors) {
    pt_fatal_err("too many unique colors in {} tileset", compilerModeString(mode));
    pt_note("{} allowed based on fieldmap configuration, but found {}", fmt::styled(allowed, fmt::emphasis::bold),
            fmt::styled(found, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), fmt::format("too many unique colors total"));
}

void fatalerror_animFrameDimensionsDoNotMatchOtherFrames(const ErrorsAndWarnings &err, const InputPaths &inputs,
                                                         CompilerMode mode, std::string animName, std::string frame,
                                                         std::string dimensionName, png::uint_32 dimension)
{
  if (err.printErrors) {
    pt_fatal_err("animation '{}' frame '{}' {} '{}' did not match previous frame {}s",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(frame, fmt::emphasis::bold), dimensionName,
                 fmt::styled(dimension, fmt::emphasis::bold), dimensionName);
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("anim {} frame {} dimension {} mismatch", animName, frame, dimensionName));
}

void fatalerror_tooManyUniqueTiles(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                   std::size_t numTiles, std::size_t maxAllowedTiles)
{
  if (err.printErrors) {
    pt_fatal_err("unique tile count '{}' exceeded limit of '{}'", fmt::styled(numTiles, fmt::emphasis::bold),
                 fmt::styled(maxAllowedTiles, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("too many unique tiles in {} tileset", compilerModeString(mode)));
}

void fatalerror_tooManyAssignmentRecurses(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                          std::size_t maxRecurses)
{
  if (err.printErrors) {
    pt_fatal_err("palette assignment exceeded maximum depth '{}'", fmt::styled(maxRecurses, fmt::emphasis::bold));
    // TODO : impl this CLI option
    pt_note("you can increase this depth with the '-fmax-assign-depth' option");
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), "too many assignment recurses");
}

void fatalerror_noPossiblePaletteAssignment(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode)
{
  if (err.printErrors) {
    pt_fatal_err("no possible palette assignment exists for the given inputs");
    pt_note("increase the number of allowed palettes or restructure your tiles to use colors more efficiently");
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), "no possible palette assignment");
}

void fatalerror_tooManyMetatiles(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                 std::size_t numMetatiles, std::size_t metatileLimit)
{
  if (err.printErrors) {
    pt_fatal_err("input metatile count of '{}' exceeded the {} tileset limit of '{}'",
                 fmt::styled(numMetatiles, fmt::emphasis::bold), compilerModeString(mode),
                 fmt::styled(metatileLimit, fmt::emphasis::bold));
  }
  die_compilationTerminated(
      err, inputs.modeBasedInputPath(mode),
      fmt::format("too many {} metatiles: {} > {}", compilerModeString(mode), numMetatiles, metatileLimit));
}

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                          std::string field, std::size_t primary, std::size_t total)
{
  if (err.printErrors) {
    pt_fatal_err("invalid configuration {}InPrimary '{}' exceeded {}Total '{}'", field,
                 fmt::styled(primary, fmt::emphasis::bold), field, fmt::styled(total, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("invalid config {}: {} > {}", field, primary, total));
}

void fatalerror_transparentKeyFrameTile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                        std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation '{}' key frame tile '{}' was transparent", fmt::styled(animName, fmt::emphasis::bold),
                 fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("this is not allowed, since there would be no way to tell if a transparent user-provided tile on the layer "
            "sheet");
    pt_println(stderr, "      referred to the true index 0 transparent tile, or if it was a reference into this "
                       "particular animation");
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} had a transparent key frame tile", animName));
}

void fatalerror_duplicateKeyFrameTile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                      std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation '{}' key frame tile '{}' duplicated another key frame tile in this tileset",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("key frame tiles must be unique within a tileset, and unique across any paired primary tileset");
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} had a duplicate key frame tile", animName));
}

void fatalerror_keyFramePresentInPairedPrimary(const ErrorsAndWarnings &err, const InputPaths &inputs,
                                               CompilerMode mode, std::string animName, std::size_t tileIndex)
{
  if (err.printErrors) {
    pt_fatal_err("animation '{}' key frame tile '{}' was present in the paired primary tileset",
                 fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
    pt_note("this is an error because it renders the animation inoperable, any reference to the key tile in the");
    pt_println(stderr,
               "      secondary layer sheet will be linked to primary tileset instead of the intended animation");
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} key frame tile present in paired primary", animName));
}

static void printWarning(ErrorsAndWarnings &err, WarningMode warningMode, const std::string &warningName,
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

static void printTileWarning(ErrorsAndWarnings &err, WarningMode warningMode, const std::string &warningName,
                             const RGBATile &tile, const std::string &message)
{
  if (err.colorPrecisionLoss == WarningMode::ERR) {
    err.errCount++;
    if (err.printErrors) {
      pt_err_rgbatile(
          tile, "{} [{}]", message,
          fmt::styled(fmt::format("-Werror={}", warningName), fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)));
    }
  }
  else if (err.colorPrecisionLoss == WarningMode::WARN) {
    err.warnCount++;
    if (err.printErrors) {
      pt_warn_rgbatile(
          tile, "{} [{}]", message,
          fmt::styled(fmt::format("-W{}", warningName), fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta)));
    }
  }
}

void warn_colorPrecisionLoss(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col,
                             const BGR15 &bgr, const RGBA32 &rgba, const RGBA32 &previousRgba)
{
  // TODO : can we improve this message? it's a bit vague
  std::string message = fmt::format(
      "color '{}' at pixel col {}, row {} collapsed to duplicate BGR (previously saw '{}')",
      fmt::styled(rgba.jasc(), fmt::emphasis::bold), col, row, fmt::styled(previousRgba.jasc(), fmt::emphasis::bold));
  printTileWarning(err, err.colorPrecisionLoss, "color-precision-loss", tile, message);
}

void warn_keyFrameTileDidNotAppearInAssignment(ErrorsAndWarnings &err, std::string animName, std::size_t tileIndex)
{
  std::string message =
      fmt::format("animation '{}' key frame tile '{}' was not present in any assignments",
                  fmt::styled(animName, fmt::emphasis::bold), fmt::styled(tileIndex, fmt::emphasis::bold));
  printWarning(err, err.keyFrameTileDidNotAppearInAssignment, "key-frame-missing-assignment", message);
}

void warn_usedTrueColorMode(ErrorsAndWarnings &err)
{
  std::string message = "`true-color' mode not yet supported by Porymap";
  printWarning(err, err.usedTrueColorMode, "used-true-color-mode", message);
  pt_note("Porymap PR #536 (https://github.com/huderlem/porymap/pull/536) will add support for `true-color' mode");
}

void die(const ErrorsAndWarnings &err, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "{}", errorMessage);
  }
  throw PtException{errorMessage};
}

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "terminating compilation of {}", fmt::styled(inputPath, fmt::emphasis::bold));
  }
  throw PtException{errorMessage};
}

void die_errorCount(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage)
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
    pt_println(stderr, "terminating compilation of {}", fmt::styled(inputPath, fmt::emphasis::bold));
  }
  throw PtException{errorMessage};
}

} // namespace porytiles

/*
 * Test cases that deliberately check for end-to-end error/warning correctness go here
 */
#include "compiler.h"
#include "importer.h"

TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly")
{
  SUBCASE("it should work for regular tiles")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_tooManyUniqueColorsInTile_regular";
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
    CHECK(ctx.err.errCount == 6);
  }

  SUBCASE("it should work for anim tiles")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_tooManyUniqueColorsInTile_anim";
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
    CHECK(ctx.err.errCount == 4);
  }
}

TEST_CASE("error_invalidAlphaValue should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_invalidAlphaValue";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PtException);
}

TEST_CASE("error_animFrameWasNotAPng should trigger correctly when an anim frame is missing")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_animFrameWasNotAPng";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "found anim frame that was not a png", porytiles::PtException);
  CHECK(ctx.err.errCount == 1);
}

TEST_CASE("error_nonTransparentRgbaCollapsedToTransparentBgr should trigger correctly when a color collapses")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_nonTransparentRgbaCollapsedToTransparentBgr";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("error_allThreeLayersHadNonTransparentContent should trigger correctly when a dual-layer inference fails")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.compilerConfig.tripleLayer = false;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_allThreeLayersHadNonTransparentContent";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import", porytiles::PtException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular secondary tiles")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/simple_metatiles_1";
  ctx.inputPaths.secondaryInputPath = "res/tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PtException);
}

TEST_CASE("fatalerror_missingRequiredAnimFrameFile should trigger correctly in both cases:")
{
  SUBCASE("when an anim frame is missing")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_skipCase";
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 01.png",
                         porytiles::PtException);
  }

  SUBCASE("when there are no regular frames supplied")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.inputPaths.primaryInputPath =
        "res/tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_keyOnlyCase";
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 00.png",
                         porytiles::PtException);
  }
}

TEST_CASE("fatalerror_missingKeyFrameFile should trigger correctly when there is no key frame supplied")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_missingKeyFrameFile";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing key frame file", porytiles::PtException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame width "
          "is mismatched")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath =
      "res/tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_widthCase";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 01.png dimension width mismatch",
                       porytiles::PtException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame height "
          "is mismatched")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath =
      "res/tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_heightCase";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 02.png dimension height mismatch",
                       porytiles::PtException);
}

TEST_CASE("fatalerror_transparentKeyFrameTile should trigger when an anim has a transparent tile")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_transparentKeyFrameTile";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 had a transparent key frame tile",
                       porytiles::PtException);
}

TEST_CASE(
    "fatalerror_duplicateKeyFrameTile should trigger when two different animations have a duplicate key frame tile")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_duplicateKeyFrameTile";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim2 had a duplicate key frame tile", porytiles::PtException);
}

TEST_CASE("fatalerror_keyFramePresentInPairedPrimary should trigger when an animation key frame tile is present in the "
          "paired primary tileset")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 2;
  ctx.fieldmapConfig.numPalettesTotal = 4;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/primary";
  ctx.inputPaths.secondaryInputPath =
      "res/tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/secondary";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 key frame tile present in paired primary",
                       porytiles::PtException);
}

TEST_CASE("warn_colorPrecisionLoss should trigger correctly when a color collapses")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/warn_colorPrecisionLoss";
  ctx.err.colorPrecisionLoss = porytiles::WarningMode::ERR;
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
  CHECK(ctx.err.errCount == 3);
}

TEST_CASE("warn_keyFrameTileDidNotAppearInAssignment should trigger correctly when a key frame tile is not used")
{
  SUBCASE("it should trigger correctly for a primary set")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary";
    ctx.err.keyFrameTileDidNotAppearInAssignment = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during primary tile assignment",
                         porytiles::PtException);
    CHECK(ctx.err.errCount == 2);
  }

  SUBCASE("it should trigger correctly for a secondary set")
  {
    porytiles::PtContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.inputPaths.primaryInputPath =
        "res/tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary_correct";
    ctx.inputPaths.secondaryInputPath =
        "res/tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/secondary";
    ctx.err.keyFrameTileDidNotAppearInAssignment = porytiles::WarningMode::ERR;
    ctx.err.printErrors = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during secondary tile assignment",
                         porytiles::PtException);
    CHECK(ctx.err.errCount == 2);
  }
}
