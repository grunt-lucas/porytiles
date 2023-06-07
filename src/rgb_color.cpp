#include "rgb_color.h"

namespace tscreate {
bool RgbColor::operator==(const RgbColor& other) const {
    return red == other.red && blue == other.blue && green == other.green;
}
} // namespace tscreate