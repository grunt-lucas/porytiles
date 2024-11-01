#ifndef PORYTILES_COLOR_BGR15_H
#define PORYTILES_COLOR_BGR15_H

#include <cstdint>
#include <iostream>
#include <string>

#include <porytiles/Color/Rgba32.h>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 15-bit BGR format.
 *
 * @details
 * The 15-bit BGR format represents a color with three 5-bit channels: one channel for blue, green,
 * and red respectively. Internally, a Bgr15 is representing using a 16-bit integer, with the top
 * bit left unused. Bgr15 is the preferred color format for the Game Boy Advance's palette RAM. The
 * BGR format has no concept of transparency, so Bgr15's @link Bgr15::computeAlphaComponent
 * computeAlphaComponent @endlink implementation simply returns @link ALPHA_OPAQUE @endlink.
 */
class Bgr15 {
    std::uint16_t bgr;

  public:
    /**
     * @brief Default constructor for Bgr15. Zero initialized, i.e. the color will be black.
     */
    constexpr explicit Bgr15() : bgr{0} {}

    /**
     * @brief Construct a Bgr15 object from a 16-bit BGR value.
     *
     * @param bgr The 16-bit BGR value used to initialize the object.
     */
    constexpr explicit Bgr15(const std::uint16_t bgr) : bgr{bgr} {}

    /**
     * @brief Constructs a Bgr15 object with the given 8-bit RGB color components.
     *
     * @details
     * This constructor takes individual red, green, and blue color components, scales them down to
     * 5 bits each, and composes them into a 15-bit BGR value.
     *
     * @param red The 8-bit red component of the color.
     * @param green The 8-bit green component of the color.
     * @param blue The 8-bit blue component of the color.
     */
    explicit Bgr15(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue)
    {
        bgr = static_cast<std::uint16_t>((blue >> 3 << 10) | (green >> 3 << 5) | red >> 3);
    }

    /**
     * @brief Returns the raw 16-bit BGR value.
     *
     * @return The 16-bit BGR value.
     */
    [[nodiscard]] std::uint16_t getRawValue() const;

    /**
     * @brief Computes the red component from the 15-bit BGR color value.
     *
     * @details
     * Extracts the red color component from the internal 15-bit BGR representation, scales it back
     * up to an 8-bit value, and returns it.
     *
     * @return The 8-bit red component derived from the 15-bit BGR value.
     */
    [[nodiscard]] std::uint8_t computeRedComponent() const;

    /**
     * @brief Computes the green component from the 15-bit BGR color value.
     *
     * @details
     * Extracts the green color component from the internal 15-bit BGR representation, scales it
     * back up to an 8-bit value, and returns it.
     *
     * @return The 8-bit green component derived from the 15-bit BGR value.
     */
    [[nodiscard]] std::uint8_t computeGreenComponent() const;

    /**
     * @brief Computes the blue component from the 15-bit BGR color value.
     *
     * @details
     * Extracts the blue color component from the internal 15-bit BGR representation, scales it back
     * up to an 8-bit value, and returns it.
     *
     * @return The 8-bit blue component derived from the 15-bit BGR value.
     */
    [[nodiscard]] std::uint8_t computeBlueComponent() const;

    /**
     * @brief Computes the alpha component, which will always be opaque.
     *
     * @details
     * The BGR15 format does not support an alpha channel. Thus, this method will always return an
     * opaque alpha value for Colors of type Bgr15.
     *
     * @return The opaque alpha component.
     */
    [[nodiscard]] std::uint8_t computeAlphaComponent() const
    {
        return ALPHA_OPAQUE;
    }

    /**
     * @brief Converts the Color to a JASC-PAL formatted string.
     *
     * @details
     * This method converts the red, green, and blue components of the Color object into a
     * space-separated string format commonly used in JASC-PAL color palette files. JASC-PAL files
     * typically do not include an alpha channel value, so that value is omitted from the string
     * returned here.
     *
     * @return A string representation of the color in JASC-PAL format.
     */
    [[nodiscard]] std::string toJascString() const
    {
        return std::to_string(computeRedComponent()) + " " +
               std::to_string(computeGreenComponent()) + " " +
               std::to_string(computeBlueComponent());
    }

    /**
     * @brief Equality operator for Bgr15 objects.
     *
     * @details
     * Compares two Bgr15 objects to determine whether their 15-bit BGR values are equal. The
     * comparison uses the raw value but shifts off the top bit, since this bit is unused in BGR15
     * format. Garbage bit values here should not affect logical equality.
     *
     * @param lhs The left-hand side Bgr15 object to compare.
     * @param rhs The right-hand side Bgr15 object to compare.
     * @return True if the 15-bit BGR values of both Bgr15 objects are equal, false otherwise.
     */
    friend bool operator==(const Bgr15 &lhs, const Bgr15 &rhs)
    {
        return lhs.getRawValue() << 1 == rhs.getRawValue() << 1;
    }

    /**
     * @brief Inequality operator for Bgr15 objects.
     *
     * @details
     * Compares two Bgr15 objects to determine whether their 15-bit BGR values are not equal.
     * Currently, it compares the raw 16-bit values, but it should be updated to ignore the top
     * unused bit.
     *
     * @param lhs The left-hand side Bgr15 object to compare.
     * @param rhs The right-hand side Bgr15 object to compare.
     * @return True if the 15-bit BGR values of both Bgr15 objects are not equal, false otherwise.
     */
    friend bool operator!=(const Bgr15 &lhs, const Bgr15 &rhs)
    {
        return !(lhs == rhs);
    }
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_BGR15_H
