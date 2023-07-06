#ifndef PORYTILES_CONFIG_H
#define PORYTILES_CONFIG_H

#include <limits>

#include "types.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {

/**
 * TODO : fill in doc comment
 */
struct Config {
    std::size_t numTilesInPrimary;
    std::size_t numTilesTotal;
    std::size_t numMetatilesInPrimary;
    std::size_t numMetatilesTotal;
    std::size_t numPalettesInPrimary;
    std::size_t numPalettesTotal;

    RGBA32 transparencyColor;
};

}
#endif
