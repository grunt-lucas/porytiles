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
  CompilerSourcePaths compilerSrcPaths;
  DecompilerSourcePaths decompilerSrcPaths;
  Output output;
  CompilerConfig compilerConfig;
  CompilerContext compilerContext;
  DecompilerConfig decompilerConfig;
  ErrorsAndWarnings err;

  // Command params
  Subcommand subcommand;
  bool verbose;

  PtContext()
      : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()},
        compilerSrcPaths{}, decompilerSrcPaths{}, output{}, compilerConfig{}, compilerContext{}, decompilerConfig{},
        err{}, subcommand{}, verbose{false}
  {
  }

  void validateFieldmapParameters() const
  {
    if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, this->compilerConfig.mode, "numTiles",
                                           fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal);
    }
    if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, this->compilerConfig.mode, "numMetatiles",
                                           fieldmapConfig.numMetatilesInPrimary, fieldmapConfig.numMetatilesTotal);
    }
    if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
      fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, this->compilerConfig.mode, "numPalettes",
                                           fieldmapConfig.numPalettesInPrimary, fieldmapConfig.numPalettesTotal);
    }
  }
};

} // namespace porytiles

#endif // PORYTILES_PT_CONTEXT_H