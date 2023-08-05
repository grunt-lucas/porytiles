#ifndef PORYTILES_ERRORS_WARNINGS_H
#define PORYTILES_ERRORS_WARNINGS_H

#include <cstddef>
#include <png.hpp>

namespace porytiles {
enum class WarningMode { OFF, WARN, ERR };

struct ErrorsAndWarnings {
  std::size_t errCount;
  bool dieCompilation;

  std::size_t colorPrecisionLossCount;
  WarningMode colorPrecisionLossMode;

  std::size_t paletteAllocEfficCount;
  WarningMode paletteAllocEfficMode;

  std::size_t transparentRepresentativeAnimTileCount;
  WarningMode transparentRepresentativeAnimTileMode;

  [[nodiscard]] std::size_t warnCount() const
  {
    return colorPrecisionLossCount + paletteAllocEfficCount + transparentRepresentativeAnimTileCount;
  }

  ErrorsAndWarnings()
      : errCount{0}, dieCompilation{false}, colorPrecisionLossCount{0}, colorPrecisionLossMode{WarningMode::OFF},
        paletteAllocEfficCount{0}, paletteAllocEfficMode{WarningMode::OFF}, transparentRepresentativeAnimTileCount{0},
        transparentRepresentativeAnimTileMode{WarningMode::OFF}
  {
  }
};

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize);
void internalerror_unknownCompilerMode();

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, std::string layer, png::uint_32 height);
void error_layerWidthNeq128(ErrorsAndWarnings &err, std::string layer, png::uint_32 width);
void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);
void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string& animation, const std::string &file);

void fatalerror_missingRequiredAnimFrameFile(const std::string& animation, std::size_t index);

void warn_colorPrecisionLoss(ErrorsAndWarnings &err);
void warn_transparentRepresentativeAnimTile(ErrorsAndWarnings &err);

void die_compilationTerminated();
void die_errorCount(const ErrorsAndWarnings &err);

} // namespace porytiles

#endif