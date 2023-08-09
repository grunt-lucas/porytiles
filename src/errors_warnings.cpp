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

void fatalerror(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode, std::string message)
{
  if (err.printErrors) {
    pt_fatal_err("{}", message);
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), message);
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

void warn_colorPrecisionLoss(ErrorsAndWarnings &err)
{
  // TODO : better message
  if (err.colorPrecisionLossMode == WarningMode::ERR) {
    err.errCount++;
    pt_err("color precision loss");
  }
  else if (err.colorPrecisionLossMode == WarningMode::WARN) {
    err.warnCount++;
    pt_warn("color precision loss");
  }
}

void warn_paletteAllocEffic(ErrorsAndWarnings &err)
{
  // TODO : better message
  if (err.paletteAllocEfficMode == WarningMode::ERR) {
    err.errCount++;
    pt_err("palette alloc effic");
  }
  else if (err.paletteAllocEfficMode == WarningMode::WARN) {
    err.warnCount++;
    pt_warn("palette alloc effic");
  }
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
    pt_println(stderr, "{} error(s) generated.", std::to_string(err.errCount));
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

TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/error_tooManyUniqueColorsInTile";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization", porytiles::PtException);
  CHECK(ctx.err.errCount == 6);
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

TEST_CASE("fatalerror_missingRequiredAnimFrameFile should trigger correctly when an anim frame is missing")
{
  porytiles::PtContext ctx{};
  ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "res/tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile";
  ctx.err.printErrors = false;

  CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 01.png",
                       porytiles::PtException);
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
