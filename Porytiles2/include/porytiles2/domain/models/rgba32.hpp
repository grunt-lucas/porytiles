#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <cstdint>
#include <format>
#include <ostream>
#include <set>
#include <string>
#include <vector>

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
     * @brief Checks if this color is intrinsically transparent based on its alpha channel.
     *
     * @details
     * A color is intrinsically transparent if its alpha value is 0.
     *
     * @return True if alpha == alpha_transparent, false otherwise
     */
    [[nodiscard]] bool is_intrinsically_transparent() const;

    /**
     * @brief Checks if this color matches the extrinsic transparency color.
     *
     * @details
     * A color is extrinsically transparent if its RGB components match the extrinsic transparency color, regardless of
     * alpha values.
     *
     * @param extrinsic The extrinsic transparency color to check against
     * @return True if this color's RGB components match the extrinsic color, false otherwise
     */
    [[nodiscard]] bool is_extrinsically_transparent(const Rgba32 &extrinsic) const;

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

    [[nodiscard]] std::string to_csv_str() const;

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

/**
 * @brief Returns the standard 16-color greyscale palette used for indexed tile output.
 *
 * @details
 * This palette maps index 0 to pure white and index 15 to pure black, which matches vanilla game tilesets. The
 * intermediate values are evenly spaced greyscale tones.
 *
 * @return A vector of 16 Rgba32 colors representing the greyscale palette
 */
inline std::vector<Rgba32> standard_greyscale_pal()
{
    return {
        Rgba32{255, 255, 255, 255},
        Rgba32{238, 238, 238, 255},
        Rgba32{222, 222, 222, 255},
        Rgba32{205, 205, 205, 255},
        Rgba32{189, 189, 189, 255},
        Rgba32{172, 172, 172, 255},
        Rgba32{156, 156, 156, 255},
        Rgba32{139, 139, 139, 255},
        Rgba32{115, 115, 115, 255},
        Rgba32{98, 98, 98, 255},
        Rgba32{82, 82, 82, 255},
        Rgba32{65, 65, 65, 255},
        Rgba32{49, 49, 49, 255},
        Rgba32{32, 32, 32, 255},
        Rgba32{16, 16, 16, 255},
        Rgba32{0, 0, 0, 255},
    };
}

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

template <>
struct std::formatter<porytiles2::Rgba32> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::Rgba32 &rgba, std::format_context &ctx) const
    {
        return std::format_to(ctx.out(), "{}", rgba.to_jasc_str());
    }
};
