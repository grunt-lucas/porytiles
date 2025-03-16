#ifndef PORYTILES_PORYTILES_CONTEXT_H
#define PORYTILES_PORYTILES_CONTEXT_H

#include <memory>
#include <string>

#include "./diagnostics/diagnostic_engine.hpp"
#include "./diagnostics/diagnostics.hpp"
#include "./errors_warnings.h"
#include "./types.h"

namespace porytiles {

struct PorytilesContext {
    TargetBaseGame targetBaseGame;
    FieldmapConfig fieldmapConfig;
    CompilerSourcePaths compilerSrcPaths;
    DecompilerSourcePaths decompilerSrcPaths;
    Output output;
    CompilerConfig compilerConfig;
    DecompilerConfig decompilerConfig;
    CompilerContext compilerContext;
    DecompilerContext decompilerContext;
    ErrorsAndWarnings err;
    std::unique_ptr<diag_engine> diag_;

    // Command params
    Subcommand subcommand;
    bool verbose;

    PorytilesContext()
        : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()},
          compilerSrcPaths{}, decompilerSrcPaths{}, output{}, compilerConfig{}, decompilerConfig{}, compilerContext{},
          decompilerContext{}, err{}, diag_{std::make_unique<diag_engine>()}, subcommand{}, verbose{false} {}

    void validateFieldmapParameters(CompilerMode compilerMode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, compilerMode, "numTiles",
                                                 fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal);
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, compilerMode, "numMetatiles",
                                                 fieldmapConfig.numMetatilesInPrimary,
                                                 fieldmapConfig.numMetatilesTotal);
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->compilerSrcPaths, compilerMode, "numPalettes",
                                                 fieldmapConfig.numPalettesInPrimary, fieldmapConfig.numPalettesTotal);
        }
    }

    void validateFieldmapParameters(DecompilerMode decompilerMode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->decompilerSrcPaths, decompilerMode, "numTiles",
                                                 fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal);
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->decompilerSrcPaths, decompilerMode, "numMetatiles",
                                                 fieldmapConfig.numMetatilesInPrimary,
                                                 fieldmapConfig.numMetatilesTotal);
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            fatalerror_misconfiguredPrimaryTotal(this->err, this->decompilerSrcPaths, decompilerMode, "numPalettes",
                                                 fieldmapConfig.numPalettesInPrimary, fieldmapConfig.numPalettesTotal);
        }
    }

    void set_diag_engine(std::unique_ptr<diag_engine> diag) {
        diag_ = std::move(diag);
    }
};

} // namespace porytiles

#endif // PORYTILES_PORYTILES_CONTEXT_H