#ifndef PORYTILES_PNG_CHECKS_H
#define PORYTILES_PNG_CHECKS_H

#include "rgb_tiled_png.h"

#include <png.hpp>

namespace porytiles {
void validateMasterPngIsAPng(const std::string& masterPngPath);

void validateMasterPngDimensions(const png::image<png::rgb_pixel>& masterPng);

void validateMasterPngTilesEach16Colors(const RgbTiledPng& png);

void validateMasterPngMaxUniqueColors(const RgbTiledPng& png, size_t maxPalettes);
} // namespace porytiles

#endif // PORYTILES_PNG_CHECKS_H