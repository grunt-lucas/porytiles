#ifndef TSCREATE_RGB_COLOR_H
#define TSCREATE_RGB_COLOR_H

#include <png.hpp>
#include <cstddef>
#include <functional>

namespace tscreate {
class RgbColor {
    png::byte red;
    png::byte green;
    png::byte blue;

public:
    RgbColor() : red{0}, green{0}, blue{0} {}

    RgbColor(const png::byte red, const png::byte green, const png::byte blue)
            : red{red}, green{green}, blue{blue} {}

    RgbColor(const RgbColor& other) {
        red = other.red;
        green = other.green;
        blue = other.blue;
    }

    [[nodiscard]] png::byte getRed() const { return red; }

    [[nodiscard]] png::byte getGreen() const { return green; }

    [[nodiscard]] png::byte getBlue() const { return blue; }

    bool operator==(const RgbColor& other) const;

    [[nodiscard]] std::string prettyString() const;
};
} // namespace tscreate

namespace std {
template<>
struct std::hash<tscreate::RgbColor> {
    size_t operator()(const tscreate::RgbColor& rgbColor) const {
        return ((hash<int>()(rgbColor.getRed()) ^
                 (hash<int>()(rgbColor.getGreen()) << 1)) >> 1) ^
               (hash<int>()(rgbColor.getBlue()) << 1);
    }
};
} // namespace tscreate

#endif // TSCREATE_RGB_COLOR_H