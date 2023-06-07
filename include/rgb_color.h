#ifndef TSCREATE_RGB_COLOR_H
#define TSCREATE_RGB_COLOR_H

#include <png.hpp>
#include <cstddef>

namespace tscreate {
class RgbColor {
    png::byte red;
    png::byte green;
    png::byte blue;

public:
    RgbColor() : red{0}, green{0}, blue{0} {}

    RgbColor(const png::byte red, const png::byte green, const png::byte blue)
        : red{red}, green{green}, blue{blue} {}

    bool operator==(const RgbColor& other) const;

    struct Hash {
        std::size_t operator()(const RgbColor& rgbColor) const;
    };
};
}

#endif // TSCREATE_RGB_COLOR_H