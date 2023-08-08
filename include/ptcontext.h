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
  FieldmapConfig fieldmapConfig;
  InputPaths inputPaths;
  Output output;
  CompilerConfig compilerConfig;
  CompilerContext compilerContext;
  ErrorsAndWarnings err;

  // Command params
  Subcommand subcommand;
  bool verbose;

  PtContext()
      : fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()}, inputPaths{}, output{}, compilerConfig{},
        compilerContext{}, err{}, subcommand{}, verbose{false}
  {
  }

  void validate() const
  {
    // TODO : handle this in a better way
    if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
      throw PtException{"fieldmap parameter `numTilesInPrimary' (" + std::to_string(fieldmapConfig.numTilesInPrimary) +
                        ") exceeded `numTilesTotal' (" + std::to_string(fieldmapConfig.numTilesTotal) + ")"};
    }
    if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
      throw PtException{"fieldmap parameter `numMetatilesInPrimary' (" +
                        std::to_string(fieldmapConfig.numMetatilesInPrimary) + ") exceeded `numMetatilesTotal' (" +
                        std::to_string(fieldmapConfig.numMetatilesTotal) + ")"};
    }
    if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
      throw PtException{"fieldmap parameter `numPalettesInPrimary' (" +
                        std::to_string(fieldmapConfig.numPalettesInPrimary) + ") exceeded `numPalettesTotal' (" +
                        std::to_string(fieldmapConfig.numPalettesTotal) + ")"};
    }
  }
};

} // namespace porytiles

#endif // PORYTILES_PT_CONTEXT_H