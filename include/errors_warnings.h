#ifndef PORYTILES_ERRORS_WARNINGS_H
#define PORYTILES_ERRORS_WARNINGS_H

#include <cstddef>
#include <png.hpp>

namespace porytiles {
struct Errors {
  std::size_t errCount;
  bool dieCompilation;

  Errors() : errCount{0}, dieCompilation{false} {}
};

enum class WarningMode { OFF, WARN, ERR };

struct Warnings {
  std::size_t colorPrecisionLossCount;
  WarningMode colorPrecisionLossMode;

  std::size_t paletteAllocEfficCount;
  WarningMode paletteAllocEfficMode;

  [[nodiscard]] std::size_t warnCount() const { return colorPrecisionLossCount + paletteAllocEfficCount; }

  Warnings()
      : colorPrecisionLossCount{0}, colorPrecisionLossMode{WarningMode::OFF}, paletteAllocEfficCount{0},
        paletteAllocEfficMode{WarningMode::OFF}
  {
  }
};

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize);
void internalerror_unknownCompilerMode();

void error_layerHeightNotDivisibleBy16(Errors &err, std::string layer, png::uint_32 height);
void error_layerWidthNeq128(Errors &err, std::string layer, png::uint_32 width);
void error_layerHeightsMustEq(Errors &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);

void die_compilationTerminated();
void die_errorCount(const Errors &err);

void warn_colorPrecisionLoss(Warnings &warnings, Errors &errors);

} // namespace porytiles

#endif