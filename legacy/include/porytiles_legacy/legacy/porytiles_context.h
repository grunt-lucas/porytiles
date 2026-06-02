#ifndef PORYTILES_PORYTILES_CONTEXT_H
#define PORYTILES_PORYTILES_CONTEXT_H

#include <memory>
#include <string>

#include "../diagnostics/diagnostic_engine.hpp"
#include "../diagnostics/diagnostics.hpp"
#include "./types.h"

namespace porytiles_legacy {

struct PorytilesContext;

/*
 * Die functions.
 */
[[noreturn]] void die(const PorytilesContext &ctx, const std::string &errorMessage);

[[noreturn]] void die_compilationTerminated(const PorytilesContext &ctx, const std::string &srcPath,
                                            const std::string &errorMessage);

[[noreturn]] void die_compilationTerminatedFailHard(const PorytilesContext &ctx, const std::string &srcPath);

[[noreturn]] void die_decompilationTerminated(const PorytilesContext &ctx, const std::string &srcPath,
                                              const std::string &errorMessage);

[[noreturn]] void die_errorCount(const PorytilesContext &ctx, const std::string &srcPath,
                                 const std::string &errorMessage);

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
    bool printDieMsg;
    std::unique_ptr<DiagEngine> diag;

    // Command params
    Subcommand subcommand;
    bool verbose;

    PorytilesContext()
        : targetBaseGame{TargetBaseGame::EMERALD}, fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()},
          compilerSrcPaths{}, decompilerSrcPaths{}, output{}, compilerConfig{}, decompilerConfig{}, compilerContext{},
          decompilerContext{}, printDieMsg{true}, diag{std::make_unique<DiagEngine>()}, subcommand{}, verbose{false} {}

    void validateFieldmapParameters(CompilerMode mode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            const auto msg = fmt::format("invalid configuration numTilesInPrimary '{}' exceeded numTilesTotal '{}'",
                                         this->diag->Bold(fieldmapConfig.numTilesInPrimary),
                                         this->diag->Bold(fieldmapConfig.numTilesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_compilationTerminated(*this, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numTiles: {} > {}", fieldmapConfig.numTilesInPrimary,
                                                  fieldmapConfig.numTilesTotal));
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            const auto msg =
                fmt::format("invalid configuration numMetatilesInPrimary '{}' exceeded numMetatilesTotal '{}'",
                            this->diag->Bold(fieldmapConfig.numMetatilesInPrimary),
                            this->diag->Bold(fieldmapConfig.numMetatilesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_compilationTerminated(*this, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numMetatiles: {} > {}",
                                                  fieldmapConfig.numMetatilesInPrimary,
                                                  fieldmapConfig.numMetatilesTotal));
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            const auto msg =
                fmt::format("invalid configuration numPalettesInPrimary '{}' exceeded numPalettesTotal '{}'",
                            this->diag->Bold(fieldmapConfig.numPalettesInPrimary),
                            this->diag->Bold(fieldmapConfig.numPalettesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_compilationTerminated(*this, this->compilerSrcPaths.modeBasedSrcPath(mode),
                                      fmt::format("invalid config numPalettes: {} > {}",
                                                  fieldmapConfig.numPalettesInPrimary,
                                                  fieldmapConfig.numPalettesTotal));
        }
    }

    void validateFieldmapParameters(DecompilerMode mode) const {
        if (fieldmapConfig.numTilesInPrimary > fieldmapConfig.numTilesTotal) {
            const auto msg = fmt::format("invalid configuration numTilesInPrimary '{}' exceeded numTilesTotal '{}'",
                                         this->diag->Bold(fieldmapConfig.numTilesInPrimary),
                                         this->diag->Bold(fieldmapConfig.numTilesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(*this, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numTiles: {} > {}",
                                                    fieldmapConfig.numTilesInPrimary, fieldmapConfig.numTilesTotal));
        }
        if (fieldmapConfig.numMetatilesInPrimary > fieldmapConfig.numMetatilesTotal) {
            const auto msg =
                fmt::format("invalid configuration numMetatilesInPrimary '{}' exceeded numMetatilesTotal '{}'",
                            this->diag->Bold(fieldmapConfig.numMetatilesInPrimary),
                            this->diag->Bold(fieldmapConfig.numMetatilesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(*this, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numMetatiles: {} > {}",
                                                    fieldmapConfig.numMetatilesInPrimary,
                                                    fieldmapConfig.numMetatilesTotal));
        }
        if (fieldmapConfig.numPalettesInPrimary > fieldmapConfig.numPalettesTotal) {
            const auto msg =
                fmt::format("invalid configuration numPalettesInPrimary '{}' exceeded numPalettesTotal '{}'",
                            this->diag->Bold(fieldmapConfig.numPalettesInPrimary),
                            this->diag->Bold(fieldmapConfig.numPalettesTotal));
            this->diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(*this, this->decompilerSrcPaths.modeBasedSrcPath(mode),
                                        fmt::format("invalid config numPalettes: {} > {}",
                                                    fieldmapConfig.numPalettesInPrimary,
                                                    fieldmapConfig.numPalettesTotal));
        }
    }

    void set_diag_engine(std::unique_ptr<DiagEngine> new_diag) {
        diag = std::move(new_diag);
    }
};

} // namespace porytiles_legacy

#endif // PORYTILES_PORYTILES_CONTEXT_H