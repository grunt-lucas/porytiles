#include "warnings.h"

#include "logger.h"
#include "ptcontext.h"

namespace porytiles {

void warn_colorPrecisionLoss(Warnings &warnings, Errors &errors)
{
  if (warnings.colorPrecisionLossMode == ERR) {
    errors.errCount++;
  }
  else if (warnings.colorPrecisionLossMode == WARN) {
    warnings.colorPrecisionLossCount++;
  }
  // TODO : print logs
}

} // namespace porytiles
