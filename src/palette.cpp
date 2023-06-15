#include "palette.h"

#include "cli_parser.h"
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

RgbColor Palette::colorAt(size_t i) const {
    return colors.at(i);
}

size_t Palette::indexOf(const RgbColor& color) const {
    size_t idx = 0;
    for (const auto& iterColor: colors) {
        if (iterColor == color)
            return idx;
        idx++;
    }
    throw std::runtime_error{"internal: color " + color.prettyString() + " not found in palette"};
}

void Palette::pushTransparencyColor() {
    index.insert(gOptTransparentColor);
    colors.push_front(gOptTransparentColor);
}

void Palette::pushZeroColor() {
    index.insert(RgbColor{0, 0, 0});
    colors.emplace_back(0, 0, 0);
}

size_t Palette::size() const {
    return colors.size();
}

size_t Palette::remainingColors() const {
    return PAL_SIZE_4BPP - colors.size();
}

int Palette::paletteWithFewestColors(const std::vector<Palette>& palettes) {
    int indexOfMin = 0;
    // Palettes can only have 15 colors, so 1000 will work fine as a starting value
    size_t minColors = 1000;
    for (size_t i = 0; i < palettes.size(); i++) {
        auto size = palettes.at(i).size();
        if (size < minColors) {
            indexOfMin = static_cast<int>(i);
            minColors = size;
        }
    }
    return indexOfMin;
}
} // namespace porytiles
