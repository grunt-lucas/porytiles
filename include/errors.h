#ifndef PORYTILES_ERRORS_H
#define PORYTILES_ERRORS_H

#include <cstddef>
#include <png.hpp>

#include "compiler_context.h"

namespace porytiles {

void fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary, std::size_t primaryPalettesSize);
void fatal_unknownCompilerMode(CompilerMode mode);

void error_layerHeightNotDivisibleBy16(std::string layer, png::uint_32 height);
void error_layerWidthNeq128(std::string layer, png::uint_32 width);
void error_layerHeightsMustEq(png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);

void die_compilationTerminated();

}

#endif // PORYTILES_ERRORS_H