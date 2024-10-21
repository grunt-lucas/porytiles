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
     * @brief Default constructor that initializes the Bgr15 object to zero.
     *
     * This constructor sets the 15-bit BGR value to zero, resulting in a color
     * representation with no intensity in blue, green, or red channels, i.e. black.
     *
     * @return A Bgr15 object with all color components set to zero.
     */
    Bgr15() : bgr{0} {}

    /**
     * Constructs a Bgr15 object with the given 16-bit BGR value.
     *
     * @param bgr The 16-bit BGR value used to initialize the Bgr15 object.
     */
    explicit Bgr15(const std::uint16_t bgr) : bgr{bgr} {}

    /**
     * Constructs a Bgr15 object with the given red, green, and blue color values.
     *
     * @param red The red value used to initialize the Bgr15 object.
     * @param green The green value used to initialize the Bgr15 object.
     * @param blue The blue value used to initialize the Bgr15 object.
     */
    explicit Bgr15(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

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
