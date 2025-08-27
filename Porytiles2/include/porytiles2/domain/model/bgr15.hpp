#pragma once

#include <cstdint>
#include <string>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class Bgr15 {
  public:
    constexpr Bgr15() : red_{0}, green_{0}, blue_{0}, alpha_{Rgba32::alpha_opaque} {}

    constexpr Bgr15(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue)
    {
        // Class invariant: color channels always have the 3 LSBs unset
        red_ = (red >> 3) << 3;
        green_ = (green >> 3) << 3;
        blue_ = (blue >> 3) << 3;
        alpha_ = Rgba32::alpha_opaque;
    }

    auto operator<=>(const Bgr15 &) const = default;

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

    /**
     * @brief Checks if this color should be treated as transparent.
     *
     * @details
     * A BGR15 color is considered transparent if either the color matches the extrinsic
     * transparency color (ignoring alpha values) or if this color's intrinsic alpha value
     * indicates transparency.
     *
     * @param extrinsic The extrinsic transparency color to check against
     * @return True if this color should be treated as transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent(const Bgr15 &extrinsic) const;

    [[nodiscard]] std::uint16_t pack() const;

    [[nodiscard]] std::string to_jasc_str() const;

    [[nodiscard]] static Bgr15 unpack(std::uint16_t packed_bgr);

    [[nodiscard]] bool equals_ignoring_alpha(const Bgr15 &other) const;

    // friend std::ostream &operator<<(std::ostream &os, const Bgr15 &bgr);

  private:
    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;

    /*
     * The alpha channel is not a "real" channel in the BGR format. However, we store an alpha channel here so that we
     * can preserve intrinsic transparency context when BGR colors are initialized from an RGBA. The alpha value will
     * never actually be written to any palettes or binary files, nor is it included in the packed/string formats.
     */
    std::uint8_t alpha_;
};

/// Provide a simple way for fmtlib to format Bgr15:
/// https://fmt.dev/11.1/api/#formatting-user-defined-types
inline auto format_as(const Bgr15 &bgr)
{
    return bgr.to_jasc_str();
}

} // namespace porytiles2

template <>
struct std::hash<porytiles2::Bgr15> {
    std::size_t operator()(const porytiles2::Bgr15 &bgr) const noexcept
    {
        const std::size_t h1 = std::hash<std::uint8_t>{}(bgr.red());
        const std::size_t h2 = std::hash<std::uint8_t>{}(bgr.green());
        const std::size_t h3 = std::hash<std::uint8_t>{}(bgr.blue());
        const std::size_t h4 = std::hash<std::uint8_t>{}(bgr.alpha());
        return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
    }
};
