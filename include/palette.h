#ifndef TSCREATE_PALETTE_H
#define TSCREATE_PALETTE_H

#include "rgb_color.h"

#include <png.hpp>
#include <unordered_set>
#include <deque>

namespace tscreate {
/*
 * Why is this 15, not 16? Since we already know every palette will have the same transparency color at index 0, when
 * allocating colors we really only have 15 available slots. Therefore, all logic checking against this can check
 * against 15, since we will simply push_front the transparency color on each palette before actually allocating
 * tile indices in the final build step.
 */
constexpr int PAL_SIZE_4BPP = 15;

class Palette {
    std::unordered_set<RgbColor> index;
    std::deque<RgbColor> colors;

public:
    Palette() {
        index.reserve(PAL_SIZE_4BPP + 1);
    }

    bool addColorAtStart(const RgbColor& color);

    bool addColorAtEnd(const RgbColor& color);

    RgbColor colorAt(int i);

    [[nodiscard]] size_t size() const;

    [[nodiscard]] size_t remainingColors() const;

    [[nodiscard]] const std::unordered_set<RgbColor>& getIndex() const { return index; }
};
} // namespace tscreate

#endif // TSCREATE_PALETTE_H