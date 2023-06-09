#include "palette.h"

#include "rgb_color.h"

namespace tscreate {
const int PAL_SIZE_4BPP = 16;

bool Palette::addColorAtStart(const RgbColor& color) {
    if (colors.size() >= PAL_SIZE_4BPP) {
        return false;
    }
    index.insert(color);
    RgbColor transparent = colors.at(0);
    colors.at(0) = color;
    colors.push_front(transparent);
    return true;
}

bool Palette::addColorAtEnd(const RgbColor& color) {
    if (colors.size() >= PAL_SIZE_4BPP) {
        return false;
    }
    index.insert(color);
    colors.push_back(color);
    return true;
}

RgbColor Palette::colorAt(int i) {
    return colors.at(i);
}

auto Palette::remainingColors() const {
    return PAL_SIZE_4BPP - colors.size();
}
} // namespace tscreate
