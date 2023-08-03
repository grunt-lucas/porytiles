#ifndef PORYTILES_WARNINGS_H
#define PORYTILES_WARNINGS_H

#include "errors.h"

namespace porytiles {
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

void warn_colorPrecisionLoss(Warnings &warnings, Errors &errors);

} // namespace porytiles

#endif // PORYTILES_WARNINGS_H