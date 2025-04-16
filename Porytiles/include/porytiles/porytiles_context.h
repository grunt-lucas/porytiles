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
    std::unique_ptr<DiagEngine> diag;

    // Command params
    Subcommand subcommand;
    bool verbose;

    PorytilesContext()
        : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()},
          compilerSrcPaths{}, decompilerSrcPaths{}, output{}, compilerConfig{}, decompilerConfig{}, compilerContext{},
          decompilerContext{}, err{}, diag{std::make_unique<DiagEngine>()}, subcommand{}, verbose{false} {}

    void validateFieldmapParameters(CompilerMode mode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            const auto msg = fmt::format("invalid configuration numTilesInPrimary '{}' exceeded numTilesTotal '{}'",
                                         this->diag->bold(fieldmapConfig.numTilesInPrimary),
                                         this->diag->bold(fieldmapConfig.numTilesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_compilationTerminated(err, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numTiles: {} > {}", fieldmapConfig.numTilesInPrimary,
                                                  fieldmapConfig.numTilesTotal));
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            const auto msg =
                fmt::format("invalid configuration numMetatilesInPrimary '{}' exceeded numMetatilesTotal '{}'",
                            this->diag->bold(fieldmapConfig.numMetatilesInPrimary),
                            this->diag->bold(fieldmapConfig.numMetatilesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_compilationTerminated(err, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numMetatiles: {} > {}",
                                                  fieldmapConfig.numMetatilesInPrimary,
                                                  fieldmapConfig.numMetatilesTotal));
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            const auto msg =
                fmt::format("invalid configuration numPalettesInPrimary '{}' exceeded numPalettesTotal '{}'",
                            this->diag->bold(fieldmapConfig.numPalettesInPrimary),
                            this->diag->bold(fieldmapConfig.numPalettesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_compilationTerminated(err, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numPalettes: {} > {}",
                                                  fieldmapConfig.numPalettesInPrimary,
                                                  fieldmapConfig.numPalettesTotal));
        }
    }

    void validateFieldmapParameters(DecompilerMode mode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            const auto msg = fmt::format("invalid configuration numTilesInPrimary '{}' exceeded numTilesTotal '{}'",
                                         this->diag->bold(fieldmapConfig.numTilesInPrimary),
                                         this->diag->bold(fieldmapConfig.numTilesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_decompilationTerminated(err, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numTiles: {} > {}",
                                                    fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal));
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            const auto msg =
                fmt::format("invalid configuration numMetatilesInPrimary '{}' exceeded numMetatilesTotal '{}'",
                            this->diag->bold(fieldmapConfig.numMetatilesInPrimary),
                            this->diag->bold(fieldmapConfig.numMetatilesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_decompilationTerminated(err, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numMetatiles: {} > {}",
                                                    fieldmapConfig.numMetatilesInPrimary,
                                                    fieldmapConfig.numMetatilesTotal));
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            const auto msg =
                fmt::format("invalid configuration numPalettesInPrimary '{}' exceeded numPalettesTotal '{}'",
                            this->diag->bold(fieldmapConfig.numPalettesInPrimary),
                            this->diag->bold(fieldmapConfig.numPalettesTotal));
            this->diag->report(E_FATAL_GENERIC, msg);
            die_decompilationTerminated(err, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numPalettes: {} > {}",
                                                    fieldmapConfig.numPalettesInPrimary,
                                                    fieldmapConfig.numPalettesTotal));
        }
    }

    void set_diag_engine(std::unique_ptr<DiagEngine> new_diag) {
        diag = std::move(new_diag);
    }
};

} // namespace porytiles

#endif // PORYTILES_PORYTILES_CONTEXT_H