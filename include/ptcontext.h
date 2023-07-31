#ifndef PORYTILES_PT_CONTEXT_H
#define PORYTILES_PT_CONTEXT_H

#include <cstddef>
#include <memory>
#include <string>

#include "types.h"
#include "ptexception.h"

namespace porytiles {

enum TilesPngPaletteMode { PAL0, TRUE_COLOR, GREYSCALE };

enum Subcommand { DECOMPILE, COMPILE };

enum CompilerMode { PRIMARY, SECONDARY, FREESTANDING };

struct FieldmapConfig {
  // Fieldmap params
  std::size_t numTilesInPrimary;
  std::size_t numTilesTotal;
  std::size_t numMetatilesInPrimary;
  std::size_t numMetatilesTotal;
  std::size_t numPalettesInPrimary;
  std::size_t numPalettesTotal;
  std::size_t numTilesPerMetatile;

  [[nodiscard]] std::size_t numPalettesInSecondary() const { return numPalettesTotal - numPalettesInPrimary; }

  [[nodiscard]] std::size_t numTilesInSecondary() const { return numTilesTotal - numTilesInPrimary; }

  [[nodiscard]] std::size_t numMetatilesInSecondary() const { return numMetatilesTotal - numMetatilesInPrimary; }

  static FieldmapConfig pokeemeraldDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 13;
    config.numTilesPerMetatile = 12;
    return config;
  }

  static FieldmapConfig pokefireredDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 640;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 640;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 7;
    config.numPalettesTotal = 13;
    config.numTilesPerMetatile = 12;
    return config;
  }

  static FieldmapConfig pokerubyDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 12;
    config.numTilesPerMetatile = 12;
    return config;
  }
};

struct InputPaths {
  std::string rawFreestandingTilesheetPath;

  std::string bottomPrimaryTilesheetPath;
  std::string middlePrimaryTilesheetPath;
  std::string topPrimaryTilesheetPath;

  std::string bottomSecondaryTilesheetPath;
  std::string middleSecondaryTilesheetPath;
  std::string topSecondaryTilesheetPath;
};

struct Output {
  TilesPngPaletteMode paletteMode;
  std::string path;

  Output() : paletteMode{GREYSCALE} {}
};

struct CompilerConfig {
  CompilerMode mode;
  RGBA32 transparencyColor;
  std::size_t maxRecurseCount;

  CompilerConfig() : mode{}, transparencyColor{RGBA_MAGENTA}, maxRecurseCount{2'000'000} {}
};

struct CompilerContext {
  std::unique_ptr<CompiledTileset> pairedPrimaryTiles;

  CompilerContext() : pairedPrimaryTiles{nullptr} {}
};

struct PtContext {
  FieldmapConfig fieldmapConfig;
  InputPaths inputPaths;
  Output output;
  CompilerConfig compilerConfig;
  CompilerContext compilerContext;

  // Command params
  Subcommand subcommand;
  bool verbose;
  // TODO : remove this in favor of compiler mode
  bool secondary;

  PtContext()
      : fieldmapConfig{FieldmapConfig::pokeemeraldDefaults()}, inputPaths{}, output{}, compilerConfig{},
        compilerContext{}, subcommand{}, verbose{false}
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