#ifndef PORYTILES_ERRORS_WARNINGS_H
#define PORYTILES_ERRORS_WARNINGS_H

#include <cstddef>
#include <string>
#include <unordered_set>

#include "./types.h"

namespace porytiles {
enum class WarningMode { OFF, WARN, ERR };

struct ErrorsAndWarnings {
    /*
     * TODO : consider having a error-specific counts. This would allows us to intelligently bail in certain places
     * depending on which errors have actually been generated. So compilation could potentially carry further and
     * generate additional errors for the user. E.g. when attributes.csv is missing and user specified this warning to
     * be an error, we could continue compilation further before terminating if we are able to check for specific error
     * counts instead of just a generalized count.
     */
    std::size_t errCount;
    bool printErrors;

    ErrorsAndWarnings() : errCount{0}, printErrors{true} {}

    [[nodiscard]] std::size_t errTotal() const {
        return errCount;
    }
};

/*
 * Internal compiler errors (due to bug in the compiler)
 */
void internalerror(const std::string &message);
void internalerror_unknownCompilerMode(const std::string &context);
void internalerror_unknownDecompilerMode(const std::string &context);
void internalerror_unknownSubcommand(const std::string &context);

/*
 * Fatal compilation errors (due to bad user input), fatal errors die immediately
 */
void fatalerror_assignCacheSyntaxError(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                       const CompilerMode &mode, std::string line, std::size_t lineNumber,
                                       std::string path);

void fatalerror_assignCacheInvalidKey(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                      const CompilerMode &mode, std::string key, std::size_t lineNumber,
                                      std::string path);

void fatalerror_assignCacheInvalidValue(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                        const CompilerMode &mode, std::string key, std::string value,
                                        std::size_t lineNumber, std::string path);

void fatalerror_paletteAssignParamSearchMatrixFailed(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                     const CompilerMode &mode);

void fatalerror_noImpliedLayerType(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                   DecompilerMode mode);

/*
 * Die functions
 */
void die(const ErrorsAndWarnings &err, std::string errorMessage);

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_compilationTerminatedFailHard(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_decompilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_errorCount(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

} // namespace porytiles

#endif