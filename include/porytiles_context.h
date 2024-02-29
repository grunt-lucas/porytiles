#ifndef PORYTILES_PORYTILES_CONTEXT_H
#define PORYTILES_PORYTILES_CONTEXT_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "errors_warnings.h"
#include "porytiles_exception.h"
#include "types.h"

namespace porytiles {

struct PorytilesContext {
  TargetBaseGame targetBaseGame;
  FieldmapConfig fieldmapConfig;
  CompilerSourcePaths compilerSrcPaths;
  DecompilerSourcePaths decompilerSrcPaths;
  Output output;
  CompilerConfig compilerConfig;
  CompilerContext compilerContext;
  DecompilerConfig decompilerConfig;
  DecompilerContext decompilerContext;
  ErrorsAndWarnings err;

  // Command params
  Subcommand subcommand;
  bool verbose;

  PorytilesContext()
      : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()},
        compilerSrcPaths{}, decompilerSrcPaths{}, output{}, compilerConfig{}, compilerContext{}, decompilerConfig{},
        decompilerContext{}, err{}, subcommand{}, verbose{false}
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

#endif // PORYTILES_PORYTILES_CONTEXT_H