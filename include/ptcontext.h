#ifndef PORYTILES_PT_CONTEXT_H
#define PORYTILES_PT_CONTEXT_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "errors_warnings.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {

struct PtContext {
  TargetBaseGame targetBaseGame;
  FieldmapConfig fieldmapConfig;
  SourcePaths srcPaths;
  Output output;
  CompilerConfig compilerConfig;
  CompilerContext compilerContext;
  ErrorsAndWarnings err;

  // Command params
  Subcommand subcommand;
  bool verbose;

  PtContext()
      : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()}, srcPaths{},
        output{}, compilerConfig{}, compilerContext{}, err{}, subcommand{}, verbose{false}
  {
  }

  void validateFieldmapParameters() const
  {
    if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->srcPaths, this->compilerConfig.mode, "numTiles",
                                           fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal);
    }
    if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->srcPaths, this->compilerConfig.mode, "numMetatiles",
                                           fieldmapConfig.numMetatilesInPrimary, fieldmapConfig.numMetatilesTotal);
    }
    if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->srcPaths, this->compilerConfig.mode, "numPalettes",
                                           fieldmapConfig.numPalettesInPrimary, fieldmapConfig.numPalettesTotal);
    }
  }
};

} // namespace porytiles

#endif // PORYTILES_PT_CONTEXT_H