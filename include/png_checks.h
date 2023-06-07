#ifndef TSCREATE_PNG_CHECKS_H
#define TSCREATE_PNG_CHECKS_H

#include "rgb_tiled_png.h"

#include <png.hpp>

namespace tscreate {
void validateMasterPngIsAPng(const std::string& masterPngPath);

void validateMasterPngDimensions(const png::image<png::rgb_pixel>& masterPng);

void validateMasterPngTilesEach16Colors(const RgbTiledPng& png);

void validateMasterPngMaxUniqueColors(const RgbTiledPng& png);
} // namespace tscreate

#endif // TSCREATE_PNG_CHECKS_H