#include "errors.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>

#include "compiler_context.h"
#include "logger.h"
#include "ptexception.h"

namespace porytiles {

void fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                      std::size_t primaryPalettesSize)
{
  throw std::runtime_error{"config.numPalettesInPrimary did not match primary palette set size (" +
                           std::to_string(configNumPalettesPrimary) + " != " + std::to_string(primaryPalettesSize) +
                           ")"};
}

void fatal_unknownCompilerMode(CompilerMode mode)
{
  throw std::runtime_error{"unknown CompilerMode: " + std::to_string(mode)};
}

void error_layerHeightNotDivisibleBy16(std::string layer, png::uint_32 height)
{
  pt_err("{} layer input PNG height `{}' was not divisible by 16", layer, height);
}

void error_layerWidthNeq128(std::string layer, png::uint_32 width)
{
  pt_err("{} layer input PNG width `{}' was not 128", layer, width);
}

void error_layerHeightsMustEq(png::uint_32 bottom, png::uint_32 middle, png::uint_32 top)
{
  pt_err("bottom, middle, top layer input PNG heights `{}, {}, {}' were not equivalent", bottom, middle, top);
}

void die_compilationTerminated() { throw PtException{"compilation terminated."}; }

} // namespace porytiles
