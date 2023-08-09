#ifndef PORYTILES_ERRORS_WARNINGS_H
#define PORYTILES_ERRORS_WARNINGS_H

#include <cstddef>
#include <png.hpp>

#include "types.h"

namespace porytiles {
enum class WarningMode { OFF, WARN, ERR };

struct ErrorsAndWarnings {
  std::size_t errCount;
  std::size_t warnCount;
  bool printErrors;

  WarningMode colorPrecisionLossMode;

  ErrorsAndWarnings() : errCount{0}, warnCount{0}, printErrors{true}, colorPrecisionLossMode{WarningMode::OFF} {}

  void enableAllWarnings() { colorPrecisionLossMode = WarningMode::WARN; }

  void setAllEnabledWarningsToErrors()
  {
    if (colorPrecisionLossMode == WarningMode::WARN) {
      colorPrecisionLossMode = WarningMode::ERR;
    }
  }
};

// Internal compiler errors (due to bug in the compiler)
void internalerror(std::string message);
void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::string context, std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize);
void internalerror_unknownCompilerMode(std::string context);

// Regular compilation errors (due to bad user input), regular errors try to die as late as possible
void error_freestandingDimensionNotDivisibleBy8(ErrorsAndWarnings &err, const InputPaths &inputs,
                                                std::string dimensionName, png::uint_32 dimension);

void error_animDimensionNotDivisibleBy8(ErrorsAndWarnings &err, std::string animName, std::string frame,
                                        std::string dimensionName, png::uint_32 dimension);

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 height);

void error_layerWidthNeq128(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 width);

void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);

void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string &animation, const std::string &file);

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col);

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha, std::size_t row,
                             std::size_t col);

void error_nonTransparentRgbaCollapsedToTransparentBgr(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row,
                                                       std::size_t col, const RGBA32 &color,
                                                       const RGBA32 &transparency);

// Fatal compilation errors (due to bad user input), fatal errors die immediately
void fatalerror(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode, std::string message);

void fatalerror_basicprefix(const ErrorsAndWarnings &err, std::string message);

void fatalerror_missingRequiredAnimFrameFile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                             const std::string &animation, std::size_t index);

void fatalerror_missingKeyFrameFile(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                             const std::string &animation);

void fatalerror_tooManyUniqueColorsTotal(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                         std::size_t allowed, std::size_t found);

void fatalerror_animFrameDimensionsDoNotMatchOtherFrames(const ErrorsAndWarnings &err, const InputPaths &inputs,
                                                         CompilerMode mode, std::string animName, std::string frame,
                                                         std::string dimensionName, png::uint_32 dimension);

void fatalerror_tooManyUniqueTiles(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                   std::size_t numTiles, std::size_t maxAllowedTiles);

void fatalerror_tooManyAssignmentRecurses(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                          std::size_t maxRecurses);

void fatalerror_noPossiblePaletteAssignment(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode);

void fatalerror_tooManyMetatiles(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                 std::size_t numMetatiles, std::size_t metatileLimit);

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                          std::string field, std::size_t primary, std::size_t total);

// Compilation warnings (due to possible mistakes in user input), compilation can continue
void warn_colorPrecisionLoss(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col,
                             const BGR15 &bgr, const RGBA32 &rgba, const RGBA32 &previousRgba);

// Die functions
void die_compilationTerminated(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage);

void die_errorCount(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage);

} // namespace porytiles

#endif