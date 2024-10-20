#ifndef PORYTILES_COLOR_RGBA_H
#define PORYTILES_COLOR_RGBA_H

#include <compare>
#include <cstdint>
#include <string>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 32-bit RGBA format.
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
     * @brief Default constructor for Rgba32.
     *
     * Initializes an Rgba32 object with default values.
     * The default constructor is implicitly defined and initializes the color components to zero.
     *
     * @return A default-initialized Rgba32 object.
     */
    Rgba32() = default;

    /**
     * @brief Constructs an Rgba32 object with specified red, green, blue, and alpha component values.
     *
     * This constructor initializes an Rgba32 object with the provided values for each color component.
     *
     * @param red The red component value.
     * @param green The green component value.
     * @param blue The blue component value.
     * @param alpha The alpha (transparency) component value.
     * @return An Rgba32 object with the specified component values.
     */
    explicit constexpr Rgba32(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue,
                              const std::uint8_t alpha)
        : red{red}, green{green}, blue{blue}, alpha{alpha}
    {
    }

    /**
     * @brief Constructs an Rgba32 object with specified RGB values and an implicit alpha value of 255.
     *
     * This constructor initializes an Rgba32 object with the given red, green, and blue component values,
     * and automatically sets the alpha component to 255 (fully opaque).
     *
     * @param red The red component of the color.
     * @param green The green component of the color.
     * @param blue The blue component of the color.
     * @return An Rgba32 object with the specified RGB values and an alpha value of 255.
     */
    explicit constexpr Rgba32(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue)
        : red{red}, green{green}, blue{blue}, alpha{ALPHA_OPAQUE}
    {
    }

    /**
     * @brief Converts the RGBA color to a JASC-PAL formatted string.
     *
     * This method converts the red, green, and blue components of the Rgba32 object
     * into a space-separated string format commonly used in JASC-PAL color palette files.
     *
     * @return A string representation of the color in JASC-PAL format.
     */
    [[nodiscard]] std::string toJascString() const;

    /**
     * @brief Provides a three-way comparison for Rgba32 objects.
     *
     * The <=> operator allows Rgba32 objects to be compared using the built-in
     * strong ordering mechanism, facilitating easy and efficient comparisons.
     *
     * @param other The Rgba32 object to compare with.
     * @return A std::strong_ordering result indicating the relative order of the objects.
     */
    std::strong_ordering operator<=>(const Rgba32 &other) const = default;
};

} // end namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA_H
