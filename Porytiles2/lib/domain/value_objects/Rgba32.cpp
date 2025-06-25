#include "porytiles2/domain/value_objects/Rgba32.hpp"

#include <string>

namespace porytiles {

std::string Rgba32::ToJascStr() const {
    return std::to_string(red_) + " " + std::to_string(green_) + " " + std::to_string(blue_);
}

bool Rgba32::EqualsIgnoringAlpha(const Rgba32 &other) const {
    return red_ == other.red_ && green_ == other.green_ && blue_ == other.blue_;
}

// std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba) {
//     // For debugging purposes, print the solid colors with names rather than int values
//     if (rgba == kRgbaBlack || rgba == bgrToRgba(BGR_BLACK)) {
//         os << "black";
//     } else if (rgba == kRgbaRed || rgba == bgrToRgba(BGR_RED)) {
//         os << "red";
//     } else if (rgba == kRgbaGreen || rgba == bgrToRgba(BGR_GREEN)) {
//         os << "green";
//     } else if (rgba == kRgbaBlue || rgba == bgrToRgba(BGR_BLUE)) {
//         os << "blue";
//     } else if (rgba == kRgbaYellow || rgba == bgrToRgba(BGR_YELLOW)) {
//         os << "yellow";
//     } else if (rgba == kRgbaMagenta || rgba == bgrToRgba(BGR_MAGENTA)) {
//         os << "magenta";
//     } else if (rgba == kRgbaCyan || rgba == bgrToRgba(BGR_CYAN)) {
//         os << "cyan";
//     } else if (rgba == kRgbaWhite || rgba == bgrToRgba(BGR_WHITE)) {
//         os << "white";
//     } else if (rgba == kRgbaGrey || rgba == bgrToRgba(BGR_GREY)) {
//         os << "grey";
//     } else if (rgba == kRgbaPurple || rgba == bgrToRgba(BGR_PURPLE)) {
//         os << "purple";
//     } else if (rgba == kRgbaLime || rgba == bgrToRgba(BGR_LIME)) {
//         os << "lime";
//     } else {
//         os << std::to_string(rgba.red()) << "," << std::to_string(rgba.green()) << "," <<
//         std::to_string(rgba.blue()); if (rgba.alpha() != 255) {
//             // Only show alpha if not opaque
//             os << "," << std::to_string(rgba.alpha());
//         }
//     }
//     return os;
// }

} // namespace porytiles
