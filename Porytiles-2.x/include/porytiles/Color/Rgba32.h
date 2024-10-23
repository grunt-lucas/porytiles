#ifndef PORYTILES_COLOR_RGBA32_H
#define PORYTILES_COLOR_RGBA32_H

#include <compare>
#include <cstdint>
#include <string>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 32-bit RGBA format.
 *
 *  @details
 * TODO 2.x : fill in explanation about RGBA format, 8 bits per color, alpha channel, etc
 */
class Rgba32 {
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
    Rgba32() = default;

    /**
     * @brief Constructs an Rgba32 object with specified red, green, blue, and alpha component
     * values.
     *
     * @param red The red component value.
     * @param green The green component value.
     * @param blue The blue component value.
     * @param alpha The alpha (transparency) component value.
     */
    explicit constexpr Rgba32(const std::uint8_t red, const std::uint8_t green,
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
    explicit constexpr Rgba32(const std::uint8_t red, const std::uint8_t green,
                              const std::uint8_t blue)
        : red{red}, green{green}, blue{blue}, alpha{ALPHA_OPAQUE}
    {
    }

    /**
     * @brief Retrieves the red component of the Rgba32 color.
     *
     * @return The red component as an 8-bit unsigned integer.
     */
    [[nodiscard]] std::uint8_t getRedComponent() const;

    /**
     * @brief Retrieves the green component of the Rgba32 color.
     *
     * @return The green component as an 8-bit unsigned integer.
     */
    [[nodiscard]] std::uint8_t getGreenComponent() const;

    /**
     * @brief Retrieves the blue component of the Rgba32 color.
     *
     * @return The blue component as an 8-bit unsigned integer.
     */
    [[nodiscard]] std::uint8_t getBlueComponent() const;

    /**
     * @brief Retrieves the alpha component of the Rgba32 color.
     *
     * @return The alpha component as an 8-bit unsigned integer.
     */
    [[nodiscard]] std::uint8_t getAlphaComponent() const;

    /**
     * @brief Converts the RGBA color to a JASC-PAL formatted string.
     *
     * @details
     * This method converts the red, green, and blue components of the Rgba32 object into a
     * space-separated string format commonly used in JASC-PAL color palette files.
     *
     * @return A string representation of the color in JASC-PAL format.
     */
    [[nodiscard]] std::string toJascString() const;

    /**
     * @brief Provides a three-way comparison for Rgba32 objects.
     *
     * @param other The Rgba32 object to compare with.
     * @return A std::strong_ordering result indicating the relative order of the objects.
     */
    std::strong_ordering operator<=>(const Rgba32 &other) const = default;
};

} // end namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA32_H
