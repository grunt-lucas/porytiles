#include "errors.h"

#include <cstddef>
#include <stdexcept>
#include <string>

#include "ptexception.h"
#include "logger.h"

namespace porytiles {

void fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary, std::size_t primaryPalettesSize) {
    throw std::runtime_error{
        "config.numPalettesInPrimary did not match primary palette set size (" +
        std::to_string(configNumPalettesPrimary) +
        " != " +
        std::to_string(primaryPalettesSize) + ")"
    };
}

void error_specifiedPresetAndFieldmap() {
    pt_err("found a game preset option and explicit fieldmap option");
    pt_msg(stderr, "Please specify either a per-game preset, or use the fieldmap options to customize");
    throw PtException("compilation terminated.");
}

}
