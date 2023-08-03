#ifndef PORYTILES_ERRORS_H
#define PORYTILES_ERRORS_H

#include <cstddef>
#include <png.hpp>

namespace porytiles {
struct Errors {
  std::size_t errCount;
  bool dieCompilation;

  Errors() : errCount{0}, dieCompilation{false} {}
};

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize);
void internalerror_unknownCompilerMode();

void error_layerHeightNotDivisibleBy16(Errors &err, std::string layer, png::uint_32 height);
void error_layerWidthNeq128(Errors &err, std::string layer, png::uint_32 width);
void error_layerHeightsMustEq(Errors &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top);

void die_compilationTerminated();
void die_errorCount(const Errors &err);

} // namespace porytiles

#endif // PORYTILES_ERRORS_H