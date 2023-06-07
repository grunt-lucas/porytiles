#ifndef TSCREATE_PALETTE_H
#define TSCREATE_PALETTE_H

#include "pixel_comparators.h"

#include <png.hpp>
#include <unordered_set>
#include <vector>

namespace tscreate {
extern const png::uint_32 PAL_SIZE_4BPP;

class Palette {
    std::unordered_set<png::rgb_pixel, tscreate::rgb_pixel_hasher, tscreate::rgb_pixel_eq> colors;
    std::vector<png::rgb_pixel> index;

public:
    Palette(const png::rgb_pixel& transparencyColor) {
        colors.insert(transparencyColor);
        index.push_back(transparencyColor);
    }

    bool addColor(const png::rgb_pixel& color);
    png::uint_32 remainingColors() const;
};
}

#endif // TSCREATE_PALLETE_H