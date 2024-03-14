#ifndef PORYTILES_ERRORS_WARNINGS_H
#define PORYTILES_ERRORS_WARNINGS_H

#include <cstddef>
#include <png.hpp>
#include <string>

#include "types.h"

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
  std::size_t warnCount;
  bool printErrors;

  // Compilation warnings
  WarningMode colorPrecisionLoss;
  WarningMode keyFrameNoMatchingTile;
  WarningMode usedTrueColorMode;
  WarningMode attributeFormatMismatch;
  WarningMode missingAttributesCsv;
  WarningMode unusedAttribute;
  WarningMode transparencyCollapse;
  WarningMode assignCacheOverride;
  WarningMode invalidAssignCache;
  WarningMode missingAssignCache;

  // Decompilation warnings
  WarningMode tileIndexOutOfRange;
  WarningMode paletteIndexOutOfRange;

  ErrorsAndWarnings()
      : errCount{0}, warnCount{0}, printErrors{true}, colorPrecisionLoss{WarningMode::OFF},
        keyFrameNoMatchingTile{WarningMode::OFF}, usedTrueColorMode{WarningMode::OFF},
        attributeFormatMismatch{WarningMode::OFF}, missingAttributesCsv{WarningMode::OFF},
        unusedAttribute{WarningMode::OFF}, transparencyCollapse{WarningMode::OFF},
        assignCacheOverride{WarningMode::OFF}, invalidAssignCache{WarningMode::OFF},
        missingAssignCache{WarningMode::OFF}, tileIndexOutOfRange{WarningMode::OFF},
        paletteIndexOutOfRange{WarningMode::OFF}
  {
  }

  void setAllWarnings(WarningMode setting)
  {
    // Compilation warnings
    colorPrecisionLoss = setting;
    keyFrameNoMatchingTile = setting;
    usedTrueColorMode = setting;
    attributeFormatMismatch = setting;
    missingAttributesCsv = setting;
    unusedAttribute = setting;
    transparencyCollapse = setting;
    assignCacheOverride = setting;
    invalidAssignCache = setting;
    missingAssignCache = setting;

    // Decompilation warnings
    tileIndexOutOfRange = setting;
    paletteIndexOutOfRange = setting;
  }

  void setAllEnabledWarningsToErrors()
  {
    // Compilation warnings
    if (colorPrecisionLoss == WarningMode::WARN) {
      colorPrecisionLoss = WarningMode::ERR;
    }
    if (keyFrameNoMatchingTile == WarningMode::WARN) {
      keyFrameNoMatchingTile = WarningMode::ERR;
    }
    if (usedTrueColorMode == WarningMode::WARN) {
      usedTrueColorMode = WarningMode::ERR;
    }
    if (attributeFormatMismatch == WarningMode::WARN) {
      attributeFormatMismatch = WarningMode::ERR;
    }
    if (missingAttributesCsv == WarningMode::WARN) {
      missingAttributesCsv = WarningMode::ERR;
    }
    if (unusedAttribute == WarningMode::WARN) {
      unusedAttribute = WarningMode::ERR;
    }
    if (transparencyCollapse == WarningMode::WARN) {
      transparencyCollapse = WarningMode::ERR;
    }
    if (assignCacheOverride == WarningMode::WARN) {
      assignCacheOverride = WarningMode::ERR;
    }
    if (invalidAssignCache == WarningMode::WARN) {
      invalidAssignCache = WarningMode::ERR;
    }
    if (missingAssignCache == WarningMode::WARN) {
      missingAssignCache = WarningMode::ERR;
    }

    // Decompilation warnings
    if (tileIndexOutOfRange == WarningMode::WARN) {
      tileIndexOutOfRange = WarningMode::ERR;
    }
    if (paletteIndexOutOfRange == WarningMode::WARN) {
      paletteIndexOutOfRange = WarningMode::ERR;
    }
  }
};

// Compilation warnings
extern const char *const WARN_COLOR_PRECISION_LOSS;
extern const char *const WARN_KEY_FRAME_NO_MATCHING_TILE;
extern const char *const WARN_USED_TRUE_COLOR_MODE;
extern const char *const WARN_ATTRIBUTE_FORMAT_MISMATCH;
extern const char *const WARN_MISSING_ATTRIBUTES_CSV;
extern const char *const WARN_UNUSED_ATTRIBUTE;
extern const char *const WARN_TRANSPARENCY_COLLAPSE;
extern const char *const WARN_ASSIGN_CACHE_OVERRIDE;
extern const char *const WARN_INVALID_ASSIGN_CACHE;
extern const char *const WARN_MISSING_ASSIGN_CACHE;

// Decompilation warnings
extern const char *const WARN_TILE_INDEX_OUT_OF_RANGE;
extern const char *const WARN_PALETTE_INDEX_OUT_OF_RANGE;

/*
 * Internal compiler errors (due to bug in the compiler)
 */
void internalerror(std::string message);
void internalerror_unknownCompilerMode(std::string context);
void internalerror_unknownDecompilerMode(std::string context);
void internalerror_unknownSubcommand(std::string context);

/*
 * Regular compilation errors (due to bad user input), regular errors try to die as late as possible
 */
void error_freestandingDimensionNotDivisibleBy8(ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                std::string dimensionName, png::uint_32 dimension);

void error_animDimensionNotDivisibleBy8(ErrorsAndWarnings &err, std::string animName, std::string frameName,
                                        std::string dimensionName, png::uint_32 dimension);

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 height);

void error_layerWidthNeq128(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 width);

void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);

void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string &animation, const std::string &file);

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col);

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha, std::size_t row,
                             std::size_t col);

void error_allThreeLayersHadNonTransparentContent(ErrorsAndWarnings &err, std::size_t metatileIndex);

void error_invalidCsvRowFormat(ErrorsAndWarnings &err, std::string filePath, std::size_t line);

void error_unknownMetatileBehavior(ErrorsAndWarnings &err, std::string filePath, std::size_t line,
                                   std::string behavior);

void error_unknownMetatileBehaviorValue(ErrorsAndWarnings &err, std::string filePath, std::size_t entry,
                                        std::uint16_t behaviorValue);

void error_duplicateAttribute(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::size_t id,
                              std::size_t previousLine);

void error_invalidTerrainType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type);

void error_invalidEncounterType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type);

/*
 * Fatal compilation errors (due to bad user input), fatal errors die immediately
 */
void fatalerror(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode, std::string message);
void fatalerror(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs, DecompilerMode mode,
                std::string message);
void fatalerror(const ErrorsAndWarnings &err, std::string errorMessage);

void fatalerror_unrecognizedOption(const ErrorsAndWarnings &err, std::string option, Subcommand subcommand);

void fatalerror_missingRequiredAnimFrameFile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                             CompilerMode mode, const std::string &animation, std::size_t index);

void fatalerror_missingKeyFrameFile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                    const std::string &animation);

void fatalerror_tooManyUniqueColorsTotal(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                         CompilerMode mode, std::size_t allowed, std::size_t found);

void fatalerror_animFrameDimensionsDoNotMatchOtherFrames(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                         CompilerMode mode, std::string animName, std::string frameName,
                                                         std::string dimensionName, png::uint_32 dimension);

void fatalerror_tooManyUniqueTiles(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                   std::size_t numTiles, std::size_t maxAllowedTiles);

void fatalerror_assignExploreCutoffReached(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                           CompilerMode mode, AssignAlgorithm algo, std::size_t maxRecurses);

void fatalerror_noPossiblePaletteAssignment(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                            CompilerMode mode);

void fatalerror_tooManyMetatiles(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                 std::size_t numMetatiles, std::size_t metatileLimit);

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                          CompilerMode mode, std::string field, std::size_t primary, std::size_t total);

void fatalerror_misconfiguredPrimaryTotal(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                          DecompilerMode mode, std::string field, std::size_t primary,
                                          std::size_t total);

void fatalerror_transparentKeyFrameTile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                        CompilerMode mode, std::string animName, std::size_t tileIndex);

void fatalerror_duplicateKeyFrameTile(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                      std::string animName, std::size_t tileIndex);

void fatalerror_keyFramePresentInPairedPrimary(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                               CompilerMode mode, std::string animName, std::size_t tileIndex);

void fatalerror_invalidAttributesCsvHeader(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                           CompilerMode mode, std::string filePath);

void fatalerror_invalidIdInCsv(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                               std::string filePath, std::string id, std::size_t line);

void fatalerror_invalidBehaviorValueCompiler(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                             CompilerMode mode, std::string behavior, std::string value,
                                             std::size_t line);

// FIXME 1.0.0 : combine these two somehow

void fatalerror_invalidBehaviorValueDecompiler(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                               DecompilerMode mode, std::string behavior, std::string value,
                                               std::size_t line);

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

/*
 * Compilation warnings (due to possible mistakes in user input), compilation can continue
 */
void warn_colorPrecisionLoss(ErrorsAndWarnings &err, CompilerMode mode, const RGBATile &tile, std::size_t row,
                             std::size_t col, const BGR15 &bgr, const RGBA32 &rgba,
                             const std::tuple<RGBA32, RGBATile, std::size_t, std::size_t> &previousRgba);

void warn_keyFrameNoMatchingTile(ErrorsAndWarnings &err, std::string animName, std::size_t tileIndex);

void warn_usedTrueColorMode(ErrorsAndWarnings &err);

void warn_tooManyAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame);

void warn_tooFewAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame);

void warn_attributesFileNotFound(ErrorsAndWarnings &err, std::string filePath);

void warn_unusedAttribute(ErrorsAndWarnings &err, std::size_t metatileId, std::size_t metatileCount,
                          std::string sourcePath);

void warn_nonTransparentRgbaCollapsedToTransparentBgr(ErrorsAndWarnings &err, CompilerMode mode, const RGBATile &tile,
                                                      std::size_t row, std::size_t col, const RGBA32 &color,
                                                      const RGBA32 &transparency);

void warn_assignCacheOverride(ErrorsAndWarnings &err, CompilerMode mode, const CompilerConfig &config,
                              std::string path);

void warn_invalidAssignCache(ErrorsAndWarnings &err, const CompilerConfig &config, std::string path);

void warn_missingAssignCache(ErrorsAndWarnings &err, const CompilerConfig &config, std::string path);

/*
 * Decompilation warnings (due to possible mistakes in user input), decompilation can continue
 */
void warn_tileIndexOutOfRange(ErrorsAndWarnings &err, DecompilerMode mode, std::size_t tileIndex,
                              std::size_t tilesheetSize, const RGBATile &tile);

void warn_paletteIndexOutOfRange(ErrorsAndWarnings &err, DecompilerMode mode, std::size_t paletteIndex,
                                 std::size_t numPalettesTotal, const RGBATile &tile);

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