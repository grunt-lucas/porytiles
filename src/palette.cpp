#include "palette.h"

#include <png.hpp>

namespace tscreate {
const int PAL_SIZE_4BPP = 16;

bool Palette::addColor(const tscreate::RgbColor& color) {
    if (index.size() >= PAL_SIZE_4BPP) {
        return false;
    }
    colors.insert(color);
    index.push_back(color);
    return true;
}

auto Palette::remainingColors() const {
    return PAL_SIZE_4BPP - index.size();
}
}
