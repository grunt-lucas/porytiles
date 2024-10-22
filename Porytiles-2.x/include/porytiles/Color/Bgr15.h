#ifndef PORYTILES_COLOR_BGR15_H
#define PORYTILES_COLOR_BGR15_H

#include <cstdint>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 15-bit BGR format.
 */
class Bgr15 {
    std::uint16_t bgr;

  public:
    /**
     * @brief Default constructor for Bgr15.
     *
     * Initializes a Bgr15 object with default values, equivalent to zero-initialization.
     * This constructor is implicitly defined.
     */
    Bgr15() = default;

    /**
     * @brief Construct a Bgr15 object from a 16-bit BGR value.
     *
     * This constructor initializes a Bgr15 object using a 16-bit integer
     * that represents a color in 15-bit BGR format.
     *
     * @param bgr The 16-bit BGR value used to initialize the object.
     */
    explicit constexpr Bgr15(const std::uint16_t bgr) : bgr{bgr} {}

    /**
     * @brief Constructs a Bgr15 object with the given 8-bit RGB color components.
     *
     * This constructor takes individual red, green, and blue color components,
     * scales them down to 5 bits each, and composes them into a 15-bit BGR value.
     *
     * @param red The 8-bit red component of the color.
     * @param green The 8-bit green component of the color.
     * @param blue The 8-bit blue component of the color.
     */
    explicit constexpr Bgr15(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    {
        bgr = static_cast<std::uint16_t>(((blue / 8) << 10) | ((green / 8) << 5) | (red / 8));
    }

    /**
     * @brief Returns the raw 16-bit BGR value.
     *
     * This method provides direct access to the internal 15-bit BGR representation
     * of the color.
     *
     * @return The 16-bit BGR value.
     */
    [[nodiscard]] std::uint16_t getRawValue() const;
};

} // end namespace porytiles::color

#endif // PORYTILES_COLOR_BGR15_H
