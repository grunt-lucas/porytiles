#ifndef PORYTILES_CONFIG_H
#define PORYTILES_CONFIG_H

#include <limits>

#include "types.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {


enum TilesPngPalette {
    _4BPP,
    _8BPP,
    GREYSCALE
};

enum Subcommand {
    COMPILE_RAW
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
    bool secondary;

    // Input PNG params
    RGBA32 transparencyColor;

    // Output PNG params
    TilesPngPalette tilesPngPalette;

    Subcommand subcommand;

    std::size_t maxPalettes() const {
        return secondary ? numPalettesTotal - numPalettesInPrimary : numPalettesInPrimary;
    }

    std::size_t maxTiles() const {
        return secondary ? numTilesTotal - numTilesInPrimary : numTilesInPrimary;
    }

    std::size_t maxMetatiles() const {
        return secondary ? numMetatilesTotal - numMetatilesInPrimary : numMetatilesInPrimary;
    }
};

// TODO : add method to get a good default config for unit tests
Config defaultConfig();

}
#endif
