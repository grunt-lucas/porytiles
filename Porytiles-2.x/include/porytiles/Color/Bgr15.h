#ifndef PORYTILES_COLOR_BGR15_H
#define PORYTILES_COLOR_BGR15_H

#include <compare>
#include <cstdint>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 15-bit BGR format.
 *
 * @details
 * TODO 2.x : fill in explanation about BGR format, 5 bits per color, top bit unused, etc.
 */
class Bgr15 {
    std::uint16_t bgr;

  public:
    /**
     * @brief Default constructor for Bgr15. Initializes to 0.
     */
    Bgr15() = default;

    /**
     * @brief Construct a Bgr15 object from a 16-bit BGR value.
     *
     * @param bgr The 16-bit BGR value used to initialize the object.
     */
    explicit constexpr Bgr15(const std::uint16_t bgr) : bgr{bgr} {}

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
    explicit constexpr Bgr15(const std::uint8_t red, const std::uint8_t green,
                             const std::uint8_t blue)
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
     * @brief Provides a three-way comparison for Bgr15 objects.
     *
     * @param other The Bgr15 object to compare with.
     * @return A std::strong_ordering result indicating the relative order of the objects.
     */
    std::strong_ordering operator<=>(const Bgr15 &other) const = default;
};

} // end namespace porytiles::color

#endif // PORYTILES_COLOR_BGR15_H
