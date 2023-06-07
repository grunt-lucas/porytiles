#include "palette.h"

#include <png.hpp>

namespace tscreate {
const png::uint_32 PAL_SIZE_4BPP = 16;

bool Palette::addColor(const png::rgb_pixel& color) {
    if (index.size() >= PAL_SIZE_4BPP) {
        return false;
    }
    colors.insert(color);
    index.push_back(color);
    return true;
}

png::uint_32 Palette::remainingColors() const {
    return PAL_SIZE_4BPP - index.size();
}
}
