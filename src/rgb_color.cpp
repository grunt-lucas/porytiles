#include "rgb_color.h"

#include <cstddef>
#include <functional>

namespace tscreate {
bool RgbColor::operator==(const RgbColor &other) const {
    return red == other.red && blue == other.blue && green == other.green;
}
}