#ifndef PORYTILES_ERRORS_H
#define PORYTILES_ERRORS_H

#include <cstddef>

namespace porytiles {

void fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary, std::size_t primaryPalettesSize);

void error_specifiedPresetAndFieldmap();

}

#endif // PORYTILES_ERRORS_H