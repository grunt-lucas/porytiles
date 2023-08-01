#include "errors.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "ptcontext.h"
#include "ptexception.h"

namespace porytiles {

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize)
{
  throw std::runtime_error{"config.numPalettesInPrimary did not match primary palette set size (" +
                           std::to_string(configNumPalettesPrimary) + " != " + std::to_string(primaryPalettesSize) +
                           ")"};
}

void internalerror_unknownCompilerMode(CompilerMode mode) { throw std::runtime_error{"unknown CompilerMode"}; }

void error_layerHeightNotDivisibleBy16(Errors &err, std::string layer, png::uint_32 height)
{
  err.errCount++;
  pt_err("{} layer input PNG height '{}' was not divisible by 16", layer, fmt::styled(height, fmt::emphasis::bold));
}

void error_layerWidthNeq128(Errors &err, std::string layer, png::uint_32 width)
{
  err.errCount++;
  pt_err("{} layer input PNG width '{}' was not 128", layer, fmt::styled(width, fmt::emphasis::bold));
}

void error_layerHeightsMustEq(Errors &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top)
{
  err.errCount++;
  pt_err("bottom, middle, top layer input PNG heights '{}, {}, {}' were not equivalent",
         fmt::styled(bottom, fmt::emphasis::bold), fmt::styled(middle, fmt::emphasis::bold),
         fmt::styled(top, fmt::emphasis::bold));
}

void die_compilationTerminated() { throw PtException{"compilation terminated."}; }

void die_errorCount(const Errors &err) { throw PtException{std::to_string(err.errCount) + " errors generated."}; }

} // namespace porytiles
