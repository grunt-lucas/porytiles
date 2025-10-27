#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <cstdint>
#include <ostream>
#include <set>
#include <string>

namespace porytiles2 {

/**
 * @brief Represents a 32-bit RGBA color.
 *
 * @details
 * RGBA32 stores color values as four 8-bit components: red, green, blue, and alpha. Alpha of 0 indicates full
 * transparency, while alpha of 255 indicates full opacity.
 *
 * @invariant Default-constructed Rgba32 is transparent (satisfies SupportsTransparency design invariant). That is,
 * `Rgba32{}` produces a transparent color with all components set to 0, including alpha.
 */
class Rgba32 {
  public:
    static constexpr std::uint8_t alpha_transparent = 0;
    static constexpr std::uint8_t alpha_opaque = 255;

    constexpr Rgba32() : red_{0}, green_{0}, blue_{0}, alpha_{0} {}

    constexpr Rgba32(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = alpha_opaque)
        : red_{red}, green_{green}, blue_{blue}, alpha_{alpha}
    {
    }

    auto operator<=>(const Rgba32 &rgba) const = default;

    bool operator==(const Rgba32 &rgba) const = default;

    /**
     * @brief Checks if this color should be treated as transparent.
     *
     * @details
     * An RGBA32 color is considered transparent if either the color matches the extrinsic transparency color (ignoring
     * alpha values) or if this color's intrinsic alpha value indicates transparency (alpha == 0).
     *
     * @param extrinsic The extrinsic transparency color to check against
     * @return True if this color should be treated as transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent(const Rgba32 &extrinsic) const;

    [[nodiscard]] std::string to_jasc_str() const;

    [[nodiscard]] bool equals_ignoring_alpha(const Rgba32 &other) const;

    // friend std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba);

    [[nodiscard]] std::uint8_t red() const
    {
        return red_;
    }

    [[nodiscard]] std::uint8_t green() const
    {
        return green_;
    }

    [[nodiscard]] std::uint8_t blue() const
    {
        return blue_;
    }

    [[nodiscard]] std::uint8_t alpha() const
    {
        return alpha_;
    }

  private:
    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;
    std::uint8_t alpha_;
};

/**
 * @brief Stream insertion operator for Rgba32.
 *
 * @details
 * Allows Rgba32 objects to be written to output streams using the << operator. Uses the JASC string representation.
 *
 * @param os The output stream
 * @param rgba The Rgba32 color to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba)
{
    os << rgba.to_jasc_str();
    return os;
}

/**
 * @brief Provides a simple way for fmtlib to format an Rgba32.
 *
 * @details
 * https://fmt.dev/11.1/api/#formatting-user-defined-types
 */
inline auto format_as(const Rgba32 &rgba)
{
    return rgba.to_jasc_str();
}

inline std::string to_string(const Rgba32 &rgba)
{
    return rgba.to_jasc_str();
}

constexpr Rgba32 rgba_black{0, 0, 0, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_white{255, 255, 255, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_grey{128, 128, 128, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_red{255, 0, 0, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_green{0, 255, 0, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_blue{0, 0, 255, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_yellow{255, 255, 0, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_magenta{255, 0, 255, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_cyan{0, 255, 255, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_purple{128, 0, 255, Rgba32::alpha_opaque};
constexpr Rgba32 rgba_lime{128, 255, 128, Rgba32::alpha_opaque};

// std::size_t hash_value(const Rgba32 &obj) {
//     std::size_t seed = 0x7A22F97A;
//     seed ^= (seed << 6) + (seed >> 2) + 0x7687DDBC +
//     static_cast<std::size_t>(obj.red); seed ^= (seed << 6) + (seed >> 2) +
//     0x63724761 + static_cast<std::size_t>(obj.green); seed ^= (seed << 6) +
//     (seed >> 2) + 0x3E044131 + static_cast<std::size_t>(obj.blue); seed ^=
//     (seed << 6) + (seed >> 2) + 0x00E64742 +
//     static_cast<std::size_t>(obj.alpha); return seed;
// }

} // namespace porytiles2

template <>
struct std::hash<porytiles2::Rgba32> {
    std::size_t operator()(const porytiles2::Rgba32 &rgba) const noexcept
    {
        const std::size_t h1 = std::hash<std::uint8_t>{}(rgba.red());
        const std::size_t h2 = std::hash<std::uint8_t>{}(rgba.green());
        const std::size_t h3 = std::hash<std::uint8_t>{}(rgba.blue());
        const std::size_t h4 = std::hash<std::uint8_t>{}(rgba.alpha());
        return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
    }
};
