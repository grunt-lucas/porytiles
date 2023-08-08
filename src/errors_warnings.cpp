#include "errors_warnings.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize)
{
  throw std::runtime_error{"config.numPalettesInPrimary did not match primary palette set size (" +
                           std::to_string(configNumPalettesPrimary) + " != " + std::to_string(primaryPalettesSize) +
                           ")"};
}

void internalerror_unknownCompilerMode() { throw std::runtime_error{"unknown CompilerMode"}; }

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

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile)
{
  err.errCount++;
  // TODO : show which pixel gives the error
  if (err.printErrors) {
    pt_err_rgbatile(tile, "too many unique colors in tile");
    pt_note_rgbatile(tile, "cannot have greater than {}, including transparency color", PAL_SIZE);
  }
}

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha)
{
  err.errCount++;
  // TODO : show which pixel gives the error
  if (err.printErrors) {
    pt_err_rgbatile(tile, "invalid alpha value: {}", alpha);
    pt_note_rgbatile(tile, "alpha value must be either {} for opaque or {} for transparent", ALPHA_OPAQUE,
                     ALPHA_TRANSPARENT);
  }
}

void fatalerror_missingRequiredAnimFrameFile(ErrorsAndWarnings &err, const std::string &animation, std::size_t index)
{
  std::string file = std::to_string(index) + ".png";
  if (index < 10) {
    file = "0" + file;
  }
  if (err.printErrors) {
    pt_fatal_err("animation '{}' was missing expected frame file '{}'", fmt::styled(animation, fmt::emphasis::bold),
                 fmt::styled(file, fmt::emphasis::bold));
  }
  die_compilationTerminated(fmt::format("animation {} missing required anim frame file {}", animation, file));
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

void warn_transparentRepresentativeAnimTile(ErrorsAndWarnings &err)
{
  // TODO : better message
  if (err.transparentRepresentativeAnimTileMode == WarningMode::ERR) {
    err.errCount++;
    pt_err("transparent representative tile");
  }
  else if (err.transparentRepresentativeAnimTileMode == WarningMode::WARN) {
    err.warnCount++;
    pt_warn("transparent representative tile");
  }
}

void die_compilationTerminated(std::string errorMessage)
{
  pt_println(stderr, "compilation terminated.");
  throw PtException{errorMessage};
}

void die_errorCount(const ErrorsAndWarnings &err, std::string errorMessage)
{
  pt_println(stderr, "{} errors generated.", std::to_string(err.errCount));
  throw PtException{errorMessage};
}

} // namespace porytiles