#ifndef PORYTILES_PT_CONTEXT_H
#define PORYTILES_PT_CONTEXT_H

#include <cstddef>
#include <string>

#include "types.h"

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

  static FieldmapConfig pokeemeraldDefaults() {
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

  static FieldmapConfig pokefireredDefaults() {
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

  static FieldmapConfig pokerubyDefaults() {
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

struct PtContext {
  FieldmapConfig fieldmapConfig;

  RGBA32 transparencyColor;
  std::string rawSecondaryTilesheetPath;
  std::string rawPrimaryTilesheetPath;
  std::string bottomPrimaryTilesheetPath;
  std::string middlePrimaryTilesheetPath;
  std::string topPrimaryTilesheetPath;
  std::string bottomSecondaryTilesheetPath;
  std::string middleSecondaryTilesheetPath;
  std::string topSecondaryTilesheetPath;
  std::size_t maxRecurseCount;
  bool secondary;

  // Output params
  TilesPngPaletteMode tilesPngPaletteMode;
  std::string outputPath;

  // Command params
  Subcommand subcommand;
  bool verbose;

  // Compiler params
  CompilerMode compilerMode;
  const CompiledTileset *primaryTileset;
};

} // namespace porytiles

#endif // PORYTILES_PT_CONTEXT_H