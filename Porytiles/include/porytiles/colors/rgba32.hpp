#pragma once

#include <cstdint>
#include <string>

namespace porytiles {

class Rgba32 {
    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;
    std::uint8_t alpha_;

  public:
    Rgba32() : red_{0}, green_{0}, blue_{0}, alpha_{0} {}

    [[nodiscard]] std::uint8_t red() const {
        return red_;
    }

    [[nodiscard]] std::uint8_t green() const {
        return green_;
    }

    [[nodiscard]] std::uint8_t blue() const {
        return blue_;
    }

    [[nodiscard]] std::uint8_t alpha() const {
        return alpha_;
    }

    auto operator<=>(const Rgba32 &rgba) const = default;

    [[nodiscard]] std::string ToJascStr() const;

    [[nodiscard]] bool EqualsIgnoringAlpha(const Rgba32 &other) const;

    friend std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba);
};

/// Provide a simple way for fmtlib to format Rgba32:
/// https://fmt.dev/11.1/api/#formatting-user-defined-types
inline auto format_as(const Rgba32 &rgba) {
    return rgba.ToJascStr();
}

// std::size_t hash_value(const Rgba32 &obj) {
//     std::size_t seed = 0x7A22F97A;
//     seed ^= (seed << 6) + (seed >> 2) + 0x7687DDBC + static_cast<std::size_t>(obj.red);
//     seed ^= (seed << 6) + (seed >> 2) + 0x63724761 + static_cast<std::size_t>(obj.green);
//     seed ^= (seed << 6) + (seed >> 2) + 0x3E044131 + static_cast<std::size_t>(obj.blue);
//     seed ^= (seed << 6) + (seed >> 2) + 0x00E64742 + static_cast<std::size_t>(obj.alpha);
//     return seed;
// }

} // namespace porytiles

template <> struct std::hash<porytiles::Rgba32> {
    std::size_t operator()(const porytiles::Rgba32 &rgba) const noexcept {
        const std::size_t h1 = std::hash<std::uint8_t>{}(rgba.red());
        const std::size_t h2 = std::hash<std::uint8_t>{}(rgba.green());
        const std::size_t h3 = std::hash<std::uint8_t>{}(rgba.blue());
        const std::size_t h4 = std::hash<std::uint8_t>{}(rgba.alpha());
        return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
    }
};
