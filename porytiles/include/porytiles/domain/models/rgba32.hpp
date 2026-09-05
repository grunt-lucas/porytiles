#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <cstdint>
#include <expected>
#include <format>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace porytiles {

/// @brief Converts an 8-bit RGB color channel to the GBA's 5-bit BGR15 channel.
///
/// @details
/// Copied from gbagfx's DOWNCONVERT_BIT_DEPTH macro (pokeemerald tools/gbagfx/gfx.c).
///
/// @param channel The 8-bit channel value
/// @return The 5-bit channel value in [0, 31]
[[nodiscard]] constexpr std::uint8_t gba_downconvert_channel(std::uint8_t channel)
{
    return static_cast<std::uint8_t>(channel / 8);
}

/// @brief Converts a GBA BGR15 5-bit color channel back to an 8 bit RGB channel.
///
/// @details
/// Copied from gbagfx's UPCONVERT_BIT_DEPTH macro (pokeemerald tools/gbagfx/gfx.c).
///
/// @param channel The 5-bit channel value
/// @pre @p channel is at most 31
/// @return The 8-bit channel value
[[nodiscard]] constexpr std::uint8_t gba_upconvert_channel(std::uint8_t channel)
{
    return static_cast<std::uint8_t>((channel * 255) / 31);
}

/// @brief Represents a 32-bit RGBA color.
///
/// @details
/// RGBA32 stores color values as four 8-bit components: red, green, blue, and alpha. Alpha of 0 indicates full
/// transparency, while alpha of 255 indicates full opacity.
///
/// @invariant Default-constructed Rgba32 is transparent (satisfies SupportsTransparency design invariant). That is,
/// `Rgba32{}` produces a transparent color with all components set to 0, including alpha.
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

    /// @brief Checks if this color is intrinsically transparent based on its alpha channel.
    ///
    /// @details
    /// A color is intrinsically transparent if its alpha value is 0.
    ///
    /// @return True if alpha == alpha_transparent, false otherwise
    [[nodiscard]] bool is_intrinsically_transparent() const;

    /// @brief Checks if this color matches the extrinsic transparency color.
    ///
    /// @details
    /// A color is extrinsically transparent if its RGB components match the extrinsic transparency color, regardless of
    /// alpha values.
    ///
    /// @param extrinsic The extrinsic transparency color to check against
    /// @return True if this color's RGB components match the extrinsic color, false otherwise
    [[nodiscard]] bool is_extrinsically_transparent(const Rgba32 &extrinsic) const;

    /// @brief Checks if this color should be treated as transparent.
    ///
    /// @details
    /// An RGBA32 color is considered transparent if either the color matches the extrinsic transparency color (ignoring
    /// alpha values) or if this color's intrinsic alpha value indicates transparency (alpha == 0).
    ///
    /// @param extrinsic The extrinsic transparency color to check against
    /// @return True if this color should be treated as transparent, false otherwise
    [[nodiscard]] bool is_transparent(const Rgba32 &extrinsic) const;

    [[nodiscard]] std::string to_jasc_str() const;

    [[nodiscard]] std::string to_csv_str() const;

    [[nodiscard]] bool equals_ignoring_alpha(const Rgba32 &other) const;

    /// @brief Round-trips this color through the GBA's downsample conversion.
    ///
    /// @details
    /// Each channel goes through gba_downconvert_channel and then gba_upconvert_channel, so the result is the 8-bit
    /// color the GBA displays for this input. This is also the value the color will have in a decompiled JASC file. Two
    /// colors with the same quantized value are the same color on hardware.
    ///
    /// @return The quantized color
    [[nodiscard]] Rgba32 quantize_to_gba() const;

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

inline std::string to_string(const Rgba32 &rgba)
{
    return "[" + std::to_string(rgba.red()) + ", " + std::to_string(rgba.green()) + ", " + std::to_string(rgba.blue()) +
           ", " + std::to_string(rgba.alpha()) + "]";
}

/// @brief Parses an Rgba32 from a comma-separated component string.
///
/// @details
/// Accepts "R,G,B" or "R,G,B,A" with each component an integer in [0, 255]. Whitespace around a component is
/// ignored. Alpha defaults to @c Rgba32::alpha_opaque when omitted.
///
/// @param text The component string to parse
/// @return The parsed color, or a lowercase error fragment (no leading capital, no trailing period) describing the
///         first problem found, for the caller to wrap with its own context
[[nodiscard]] std::expected<Rgba32, std::string> parse_rgba32_string(std::string_view text);

/// @brief Stream insertion operator for Rgba32.
///
/// @details
/// Allows Rgba32 objects to be written to output streams using the << operator. Uses the bracketed component format
/// (e.g., "[R, G, B, A]").
///
/// @param os The output stream
/// @param rgba The Rgba32 color to output
/// @return Reference to the output stream
inline std::ostream &operator<<(std::ostream &os, const Rgba32 &rgba)
{
    os << to_string(rgba);
    return os;
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

/// @brief Returns the standard 16-color greyscale palette used for indexed tile output.
///
/// @details
/// This palette maps index 0 to pure white and index 15 to pure black, which matches vanilla game tilesets. The
/// intermediate values are evenly spaced greyscale tones.
///
/// @return A vector of 16 Rgba32 colors representing the greyscale palette
inline std::vector<Rgba32> standard_greyscale_palette()
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

} // namespace porytiles

template <>
struct std::hash<porytiles::Rgba32> {
    std::size_t operator()(const porytiles::Rgba32 &rgba) const noexcept
    {
        const std::size_t h1 = std::hash<std::uint8_t>{}(rgba.red());
        const std::size_t h2 = std::hash<std::uint8_t>{}(rgba.green());
        const std::size_t h3 = std::hash<std::uint8_t>{}(rgba.blue());
        const std::size_t h4 = std::hash<std::uint8_t>{}(rgba.alpha());
        return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
    }
};

/// @brief std::formatter specialization for Rgba32.
///
/// @details
/// Enables Rgba32 to be used with std::format() and related formatting functions. This makes Rgba32 participate in
/// the FormatParam string conversion chain via the std::formattable concept.
///
/// Example usage:
/// ```c++
/// Rgba32 color{255, 128, 0, 255};
/// std::string s = std::format("Color: {}", color);  // "Color: [255, 128, 0, 255]"
///
/// // Works automatically with FormatParam:
/// FormattableError{"Invalid color: {}", FormatParam{color, Style::bold}};
/// ```
///
/// This is the recommended way to add formatting support for custom types in Porytiles. The formatter delegates to
/// the porytiles::to_string() overload for consistent string representation across the codebase.
template <>
struct std::formatter<porytiles::Rgba32> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::Rgba32 &rgba, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(rgba));
    }
};
