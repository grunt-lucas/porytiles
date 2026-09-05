#include "porytiles/domain/models/rgba32.hpp"

#include <algorithm>
#include <charconv>
#include <expected>
#include <format>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "porytiles/utilities/string_utils.hpp"

namespace porytiles {

[[nodiscard]] bool Rgba32::is_intrinsically_transparent() const
{
    return alpha_ == alpha_transparent;
}

[[nodiscard]] bool Rgba32::is_extrinsically_transparent(const Rgba32 &extrinsic) const
{
    return extrinsic.equals_ignoring_alpha(*this);
}

[[nodiscard]] bool Rgba32::is_transparent(const Rgba32 &extrinsic) const
{
    return is_intrinsically_transparent() || is_extrinsically_transparent(extrinsic);
}

std::string Rgba32::to_jasc_str() const
{
    return std::to_string(red_) + " " + std::to_string(green_) + " " + std::to_string(blue_);
}

std::string Rgba32::to_csv_str() const
{
    return std::to_string(red_) + ", " + std::to_string(green_) + ", " + std::to_string(blue_);
}

bool Rgba32::equals_ignoring_alpha(const Rgba32 &other) const
{
    return red_ == other.red_ && green_ == other.green_ && blue_ == other.blue_;
}

Rgba32 Rgba32::quantize_to_gba() const
{
    const auto round_trip = [](std::uint8_t channel) {
        return gba_upconvert_channel(gba_downconvert_channel(channel));
    };
    return Rgba32{round_trip(red_), round_trip(green_), round_trip(blue_), alpha_};
}

// std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba) {
//     // For debugging purposes, print the solid colors with names rather than
//     int values if (rgba == kRgbaBlack || rgba == bgrToRgba(BGR_BLACK)) {
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
//         os << std::to_string(rgba.red()) << "," <<
//         std::to_string(rgba.green()) << "," << std::to_string(rgba.blue());
//         if (rgba.alpha() != 255) {
//             // Only show alpha if not opaque
//             os << "," << std::to_string(rgba.alpha());
//         }
//     }
//     return os;
// }

std::expected<Rgba32, std::string> parse_rgba32_string(std::string_view text)
{
    std::vector<std::uint8_t> components{};
    for (std::string token : split(std::string{text}, ",")) {
        trim(token);
        int value = 0;
        const auto *begin = token.data();
        const auto *end = token.data() + token.size();
        const auto [ptr, ec] = std::from_chars(begin, end, value);
        if (ec != std::errc{} || ptr != end) {
            return std::unexpected{std::format("'{}' is not a valid integer", token)};
        }
        if (value < 0 || value > 255) {
            return std::unexpected{std::format("component {} is out of range (must be 0-255)", value)};
        }
        components.push_back(static_cast<std::uint8_t>(value));
    }

    if (components.size() == 3) {
        return Rgba32{components.at(0), components.at(1), components.at(2), Rgba32::alpha_opaque};
    }
    if (components.size() == 4) {
        return Rgba32{components.at(0), components.at(1), components.at(2), components.at(3)};
    }
    return std::unexpected{std::format("expected R,G,B or R,G,B,A format (got {} components)", components.size())};
}

} // namespace porytiles
