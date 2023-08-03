#include "warnings.h"

#include "errors.h"
#include "logger.h"
#include "ptcontext.h"

namespace porytiles {

void warn_colorPrecisionLoss(Warnings &warnings, Errors &errors)
{
  if (warnings.colorPrecisionLossMode == WarningMode::ERR) {
    errors.errCount++;
  }
  else if (warnings.colorPrecisionLossMode == WarningMode::WARN) {
    warnings.colorPrecisionLossCount++;
  }
  // TODO : print logs
}

} // namespace porytiles
