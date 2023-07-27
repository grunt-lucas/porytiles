#ifndef PORYTILES_CONFIG_H
#define PORYTILES_CONFIG_H

#include <limits>

#include "types.h"
#include "ptexception.h"

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
    // Fieldmap params
    std::size_t numTilesInPrimary;
    std::size_t numTilesTotal;
    std::size_t numMetatilesInPrimary;
    std::size_t numMetatilesTotal;
    std::size_t numPalettesInPrimary;
    std::size_t numPalettesTotal;
    std::size_t numTilesPerMetatile;

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
    bool secondary;

    // Output params
    TilesPngPaletteMode tilesPngPaletteMode;
    std::string outputPath;

    // Command params
    Subcommand subcommand;
    bool verbose;

    [[nodiscard]] std::size_t numPalettesInSecondary() const {
        return numPalettesTotal - numPalettesInPrimary;
    }

    [[nodiscard]] std::size_t numTilesInSecondary() const {
        return numTilesTotal - numTilesInPrimary;
    }

    [[nodiscard]] std::size_t numMetatilesInSecondary() const {
        return numMetatilesTotal - numMetatilesInPrimary;
    }

    void validate() const {
        if (numTilesInPrimary > numTilesTotal) {
            throw PtException{
                    "fieldmap parameter `numTilesInPrimary' (" + std::to_string(numTilesInPrimary) +
                    ") exceeded `numTilesTotal' (" + std::to_string(numTilesTotal) + ")"};
        }
        if (numMetatilesInPrimary > numMetatilesTotal) {
            throw PtException{
                    "fieldmap parameter `numMetatilesInPrimary' (" + std::to_string(numMetatilesInPrimary) +
                    ") exceeded `numMetatilesTotal' (" + std::to_string(numMetatilesTotal) + ")"};
        }
        if (numPalettesInPrimary > numPalettesTotal) {
            throw PtException{
                    "fieldmap parameter `numPalettesInPrimary' (" + std::to_string(numPalettesInPrimary) +
                    ") exceeded `numPalettesTotal' (" + std::to_string(numPalettesTotal) + ")"};
        }
    }
};

Config defaultConfig();
void setPokeemeraldDefaultTilesetParams(Config& config);
void setPokefireredDefaultTilesetParams(Config& config);
void setPokerubyDefaultTilesetParams(Config& config);

}
#endif // PORYTILES_CONFIG_H
