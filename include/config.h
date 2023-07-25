#ifndef PORYTILES_CONFIG_H
#define PORYTILES_CONFIG_H

#include <limits>

#include "types.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {


enum TilesPngPaletteMode {
    PAL0,
    TRUE_COLOR,
    GREYSCALE
};

enum Subcommand {
    COMPILE_RAW,
    COMPILE
};

/**
 * TODO : fill in doc comment
 */
struct Config {
    // Tileset params
    std::size_t numTilesInPrimary;
    std::size_t numTilesTotal;
    std::size_t numMetatilesInPrimary;
    std::size_t numMetatilesTotal;
    std::size_t numPalettesInPrimary;
    std::size_t numPalettesTotal;
    std::size_t numTilesPerMetatile;
    // TODO : remove this field
    bool secondary;

    // Input params
    RGBA32 transparencyColor;
    std::string rawTilesheetPath;
    std::string bottomTilesheetPath;
    std::string middleTilesheetPath;
    std::string topTilesheetPath;
    std::string bottomPrimaryTilesheetPath;
    std::string middlePrimaryTilesheetPath;
    std::string topPrimaryTilesheetPath;
    std::size_t maxRecurseCount;

    // Output params
    TilesPngPaletteMode tilesPngPaletteMode;
    std::string outputPath;

    Subcommand subcommand;

    std::size_t numPalettesInSecondary() const {
        return numPalettesTotal - numPalettesInPrimary;
    }

    std::size_t numTilesInSecondary() const {
        return numTilesTotal - numTilesInPrimary;
    }
};

// TODO : add method to get a good default config for unit tests
Config defaultConfig();

}
#endif // PORYTILES_CONFIG_H
