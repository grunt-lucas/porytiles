#include "rgb_color.h"

#include <string>

namespace porytiles {
bool RgbColor::operator==(const RgbColor& other) const {
    return red == other.red && green == other.green && blue == other.blue;
}

bool RgbColor::operator!=(const RgbColor& other) const {
    return !(*this == other);
}

std::string RgbColor::prettyString() const {
    return std::to_string(red) + "," + std::to_string(green) + "," + std::to_string(blue);
}

std::string RgbColor::jascString() const {
    return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
}

doctest::String toString(const RgbColor& color) {
    std::string colorString = "RgbColor{" + std::to_string(color.getRed()) + "," +
                              std::to_string(color.getGreen()) + "," + std::to_string(color.getBlue()) + "}";
    return colorString.c_str();
}
} // namespace porytiles