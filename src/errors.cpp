#include "errors.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <png.hpp>

#include "doctest.h"
#include "ptexception.h"
#include "logger.h"
#include "compiler_context.h"

namespace porytiles {

void fatal_numPalettesInPrimaryNeqPrimaryPalettesSize(std::size_t configNumPalettesPrimary, std::size_t primaryPalettesSize) {
    throw std::runtime_error{
        "config.numPalettesInPrimary did not match primary palette set size (" +
        std::to_string(configNumPalettesPrimary) +
        " != " +
        std::to_string(primaryPalettesSize) + ")"
    };
}

void fatal_unknownCompilerMode(CompilerMode mode) {
    throw std::runtime_error{"unknown CompilerMode: " + std::to_string(mode)};
}

void error_specifiedPresetAndFieldmap() {
    pt_err("found a game preset option and explicit fieldmap option");
    pt_msg(stderr, "Please specify either a per-game preset, or use the fieldmap options to customize");
}

void error_layerHeightNotDivisibleBy16(std::string layer, png::uint_32 height) {
    pt_err("{} layer input PNG height `{}' was not divisible by 16", layer, height);
}

void error_layerWidthNeq128(std::string layer, png::uint_32 width) {
    pt_err("{} layer input PNG width `{}' was not 128", layer, width);
}

void error_layerHeightsMustEq(png::uint_32 bottom, png::uint_32 middle, png::uint_32 top) {
    pt_err("bottom, middle, top layer input PNG heights `{}, {}, {}' were not equivalent", bottom, middle, top);
}

void die_compilationTerminated() {
    throw PtException{"compilation terminated."};
}

}
