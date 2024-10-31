#ifndef PORYTILES_COLOR_COLOR_H
#define PORYTILES_COLOR_COLOR_H

#include <cstdint>
#include <iostream>

namespace porytiles::color {

/**
 * @brief An abstract representation of a color from the perspective of the Porytiles library.
 *
 * @details
 * The Porytiles library is color implementation agnostic. That is, it only needs a color to conform
 * to this simple interface. The underlying color implementation is irrelevant, as most color
 * representations can be converted into red, green, and blue channels via a simple formula.
 * Porytiles provides a few Color implementations that may be useful, but users of the Porytiles
 * library may also implement their own if they have a specific use-case.
 */
class Color {
  public:
    virtual ~Color() = default;

    /**
     * @brief Computes the red component of the color.
     *
     * @return The red component as an 8-bit unsigned integer.
     */
    [[nodiscard]] virtual std::uint8_t computeRedComponent() const = 0;

    /**
     * @brief Computes the green component of the color.
     *
     * @return The green component as an 8-bit unsigned integer.
     */
    [[nodiscard]] virtual std::uint8_t computeGreenComponent() const = 0;

    /**
     * @brief Computes the blue component of the color.
     *
     * @return The blue component as an 8-bit unsigned integer.
     */
    [[nodiscard]] virtual std::uint8_t computeBlueComponent() const = 0;

    /**
     * @brief Computes the alpha component of the color.
     *
     * @return The alpha component as an 8-bit unsigned integer.
     */
    [[nodiscard]] virtual std::uint8_t computeAlphaComponent() const = 0;

    /**
     * @brief Converts the Color to a JASC-PAL formatted string.
     *
     * @details
     * This method converts the red, green, and blue components of the Color object into a
     * space-separated string format commonly used in JASC-PAL color palette files.
     *
     * @return A string representation of the color in JASC-PAL format.
     */
    [[nodiscard]] virtual std::string toJascString() const
    {
        return std::to_string(computeRedComponent()) + " " +
               std::to_string(computeGreenComponent()) + " " +
               std::to_string(computeBlueComponent());
    }

    /**
     * @brief Compares two Color objects for equality.
     *
     * @param lhs The left-hand side Color object for comparison.
     * @param rhs The right-hand side Color object for comparison.
     * @return True if the Color objects are equal in terms of red, green, blue, and alpha
     * components; otherwise, false.
     */
    friend bool operator==(const Color &lhs, const Color &rhs)
    {
        const auto leftRed = lhs.computeRedComponent();
        const auto leftGreen = lhs.computeGreenComponent();
        const auto leftBlue = lhs.computeBlueComponent();
        const auto leftAlpha = lhs.computeAlphaComponent();

        const auto rightRed = rhs.computeRedComponent();
        const auto rightGreen = rhs.computeGreenComponent();
        const auto rightBlue = rhs.computeBlueComponent();
        const auto rightAlpha = rhs.computeAlphaComponent();

        return leftRed == rightRed && leftGreen == rightGreen && leftBlue == rightBlue &&
               leftAlpha == rightAlpha;
    }

    /**
     * @brief Compares two Color objects for inequality.
     *
     * @param lhs The left-hand side Color object for comparison.
     * @param rhs The right-hand side Color object for comparison.
     * @return True if the Color objects are not equal in terms of red, green, blue, and alpha
     * components; otherwise, false.
     */
    friend bool operator!=(const Color &lhs, const Color &rhs)
    {
        return !(lhs == rhs);
    }
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_COLOR_H
