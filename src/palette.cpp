#include "palette.h"

#include "rgb_color.h"

namespace porytiles {
bool Palette::addColorAtStart(const RgbColor& color) {
    if (colors.size() >= PAL_SIZE_4BPP) {
        return false;
    }
    index.insert(color);
    colors.push_front(color);
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

[[nodiscard]] size_t Palette::size() const {
    return colors.size();
}

size_t Palette::remainingColors() const {
    return PAL_SIZE_4BPP - colors.size();
}
} // namespace porytiles
