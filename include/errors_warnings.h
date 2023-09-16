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

  WarningMode colorPrecisionLoss;
  WarningMode keyFrameTileDidNotAppearInAssignment;
  WarningMode usedTrueColorMode;
  WarningMode attributeFormatMismatch;
  WarningMode missingAttributesCsv;
  WarningMode missingBehaviorsHeader;
  WarningMode unusedAttribute;
  WarningMode transparencyCollapse;

  ErrorsAndWarnings()
      : errCount{0}, warnCount{0}, printErrors{true}, colorPrecisionLoss{WarningMode::OFF},
        keyFrameTileDidNotAppearInAssignment{WarningMode::OFF}, usedTrueColorMode{WarningMode::OFF},
        attributeFormatMismatch{WarningMode::OFF}, missingAttributesCsv{WarningMode::OFF},
        missingBehaviorsHeader{WarningMode::OFF}, unusedAttribute{WarningMode::OFF},
        transparencyCollapse{WarningMode::OFF}
  {
  }

  void setAllWarnings(WarningMode setting)
  {
    colorPrecisionLoss = setting;
    keyFrameTileDidNotAppearInAssignment = setting;
    usedTrueColorMode = setting;
    attributeFormatMismatch = setting;
    missingAttributesCsv = setting;
    missingBehaviorsHeader = setting;
    unusedAttribute = setting;
    transparencyCollapse = setting;
  }

  void setAllEnabledWarningsToErrors()
  {
    if (colorPrecisionLoss == WarningMode::WARN) {
      colorPrecisionLoss = WarningMode::ERR;
    }
    if (keyFrameTileDidNotAppearInAssignment == WarningMode::WARN) {
      keyFrameTileDidNotAppearInAssignment = WarningMode::ERR;
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
    if (missingBehaviorsHeader == WarningMode::WARN) {
      missingBehaviorsHeader = WarningMode::ERR;
    }
    if (unusedAttribute == WarningMode::WARN) {
      unusedAttribute = WarningMode::ERR;
    }
    if (transparencyCollapse == WarningMode::WARN) {
      transparencyCollapse = WarningMode::ERR;
    }
  }
};

extern const char *const WARN_COLOR_PRECISION_LOSS;
extern const char *const WARN_KEY_FRAME_DID_NOT_APPEAR;
extern const char *const WARN_USED_TRUE_COLOR_MODE;
extern const char *const WARN_ATTRIBUTE_FORMAT_MISMATCH;
extern const char *const WARN_MISSING_ATTRIBUTES_CSV;
extern const char *const WARN_MISSING_BEHAVIORS_HEADER;
extern const char *const WARN_UNUSED_ATTRIBUTE;
extern const char *const WARN_TRANSPARENCY_COLLAPSE;

// Internal compiler errors (due to bug in the compiler)
void internalerror(std::string message);
void internalerror_unknownCompilerMode(std::string context);
void internalerror_unknownDecompilerMode(std::string context);

// Regular compilation errors (due to bad user input), regular errors try to die as late as possible
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

void error_duplicateAttribute(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::size_t id,
                              std::size_t previousLine);

void error_invalidTerrainType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type);

void error_invalidEncounterType(ErrorsAndWarnings &err, std::string filePath, std::size_t line, std::string type);

// Fatal compilation errors (due to bad user input), fatal errors die immediately
void fatalerror(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode, std::string message);
void fatalerror(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs, DecompilerMode mode,
                std::string message);

void fatalerror_porytilesprefix(const ErrorsAndWarnings &err, std::string errorMessage);

void fatalerror_invalidSourcePath(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                  std::string path);
void fatalerror_invalidSourcePath(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs, DecompilerMode mode,
                                  std::string path);

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

void fatalerror_invalidBehaviorValue(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs, CompilerMode mode,
                                     std::string behavior, std::string value, std::size_t line);

// Compilation warnings (due to possible mistakes in user input), compilation can continue
void warn_colorPrecisionLoss(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col,
                             const BGR15 &bgr, const RGBA32 &rgba,
                             const std::tuple<RGBA32, RGBATile, std::size_t, std::size_t> &previousRgba);

void warn_keyFrameTileDidNotAppearInAssignment(ErrorsAndWarnings &err, std::string animName, std::size_t tileIndex);

void warn_usedTrueColorMode(ErrorsAndWarnings &err);

void warn_tooManyAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame);

void warn_tooFewAttributesForTargetGame(ErrorsAndWarnings &err, std::string filePath, TargetBaseGame baseGame);

void warn_attributesFileNotFound(ErrorsAndWarnings &err, std::string filePath);

void warn_behaviorsHeaderNotSpecified(ErrorsAndWarnings &err, std::string filePath);

void warn_unusedAttribute(ErrorsAndWarnings &err, std::size_t metatileId, std::size_t metatileCount,
                          std::string sourcePath);

void warn_nonTransparentRgbaCollapsedToTransparentBgr(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row,
                                                      std::size_t col, const RGBA32 &color, const RGBA32 &transparency);

// Die functions
void die(const ErrorsAndWarnings &err, std::string errorMessage);

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_compilationTerminatedFailHard(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_decompilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

void die_errorCount(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage);

} // namespace porytiles

#endif