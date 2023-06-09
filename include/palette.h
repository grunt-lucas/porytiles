#ifndef TSCREATE_PALETTE_H
#define TSCREATE_PALETTE_H

#include "rgb_color.h"

#include <png.hpp>
#include <unordered_set>
#include <deque>

namespace tscreate {
extern const int PAL_SIZE_4BPP;

class Palette {
    std::unordered_set<RgbColor> index;
    std::deque<RgbColor> colors;

public:
    explicit Palette(const RgbColor& transparencyColor) {
        index.insert(transparencyColor);
        colors.push_back(transparencyColor);
    }

    bool addColorAtStart(const RgbColor& color);

    bool addColorAtEnd(const RgbColor& color);

    RgbColor colorAt(int i);

    [[nodiscard]] auto remainingColors() const;

    [[nodiscard]] const std::unordered_set<RgbColor>& getIndex() const { return index; }
};
} // namespace tscreate

#endif // TSCREATE_PALLETE_H