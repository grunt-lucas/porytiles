#include "rgb_color.h"

#include <cstddef>
#include <functional>

namespace tscreate {
bool RgbColor::operator==(const RgbColor &other) const {
    return red == other.red && blue == other.blue && green == other.green;
}

std::size_t RgbColor::Hash::operator()(const RgbColor &rgbColor) const {
    using std::hash;
    using std::size_t;

    return ((hash<int>()(rgbColor.red) ^
            (hash<int>()(rgbColor.green) << 1)) >> 1) ^
            (hash<int>()(rgbColor.blue) << 1);
}
}