#include "init_checks.h"

#include "tscreate.h"

namespace tscreate {
void validateMasterPngExistsAndDimensions(const std::string& masterPngPath)
{
    png::image<png::rgb_pixel> masterPng(masterPngPath);
    // TODO : don't use magic number 8 here, should have a constant for tile dimension
    if (masterPng.get_width() % 8 != 0) {
        throw TsException("master PNG width must be divisible by 8, was: " + std::to_string(masterPng.get_width()));
    }
    if (masterPng.get_height() % 8 != 0) {
        throw TsException("master PNG height must be divisible by 8, was: " + std::to_string(masterPng.get_height()));
    }
}
}
