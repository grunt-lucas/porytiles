#ifndef PORYTILES_COLOR_RGBA32_H
#define PORYTILES_COLOR_RGBA32_H

#include <cstdint>
#include <string>

#include "porytiles/Color/Color.h"

namespace porytiles::color {

/**
 * @brief Value object representing a color in 32-bit RGBA format.
 *
 * @details
 * TODO 2.x : fill in explanation about RGBA format, 8 bits per color, alpha channel, etc
 */
class Rgba32 : public Color {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

  public:
    static constexpr std::uint8_t ALPHA_TRANSPARENT = 0;
    static constexpr std::uint8_t ALPHA_OPAQUE = 0xff;

    /**
     * @brief Default constructor for Rgba32. Initializes the color and alpha components to zero.
     */
    constexpr explicit Rgba32() : red{0}, green{0}, blue{0}, alpha{0} {}

    /**
     * @brief Constructs an Rgba32 object with specified red, green, blue, and alpha component
     * values.
     *
     * @param red The red component value.
     * @param green The green component value.
     * @param blue The blue component value.
     * @param alpha The alpha (transparency) component value.
     */
    constexpr explicit Rgba32(const std::uint8_t red, const std::uint8_t green,
                              const std::uint8_t blue, const std::uint8_t alpha)
        : red{red}, green{green}, blue{blue}, alpha{alpha}
    {
    }

    /**
     * @brief Constructs an Rgba32 object with specified RGB values and an implicit alpha value of
     * 255 (fully opaque).
     *
     * @param red The red component of the color.
     * @param green The green component of the color.
     * @param blue The blue component of the color.
     */
    constexpr explicit Rgba32(const std::uint8_t red, const std::uint8_t green,
                              const std::uint8_t blue)
        : red{red}, green{green}, blue{blue}, alpha{ALPHA_OPAQUE}
    {
    }

    /**
     * @brief Returns the internal red component.
     *
     * @return The internal 8-bit red component.
     */
    [[nodiscard]] std::uint8_t computeRedComponent() const override
    {
        return red;
    }

    /**
     * @brief Returns the internal green component.
     *
     * @return The internal 8-bit green component.
     */
    [[nodiscard]] std::uint8_t computeGreenComponent() const override
    {
        return green;
    }

    /**
     * @brief Returns the internal blue component.
     *
     * @return The internal 8-bit blue component.
     */
    [[nodiscard]] std::uint8_t computeBlueComponent() const override
    {
        return blue;
    }

    /**
     * @brief Returns the internal alpha component.
     *
     * @return The internal 8-bit alpha component.
     */
    [[nodiscard]] std::uint8_t computeAlphaComponent() const override
    {
        return alpha;
    }

    friend bool operator==(const Rgba32 &lhs, const Rgba32 &rhs)
    {
        return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue &&
               lhs.alpha == rhs.alpha;
    }

    friend bool operator!=(const Rgba32 &lhs, const Rgba32 &rhs)
    {
        return !(lhs == rhs);
    }
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA32_H
