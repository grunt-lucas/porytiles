#ifndef TSCREATE_PNG_CHECKS_H
#define TSCREATE_PNG_CHECKS_H

#include <png.hpp>

namespace tscreate {
void validateMasterPngIsAPng(const std::string& masterPngPath);
void validateMasterPngDimensions(const png::image<png::rgb_pixel>& masterPng);
void validateMasterPngTilesEach16Colors(const png::image<png::rgb_pixel>& masterPng);
void validateMasterPngMaxUniqueColors(const png::image<png::rgb_pixel>& masterPng);
}

#endif // TSCREATE_PNG_CHECKS_H