#ifndef PORYTILES_COLOR_RGBA32_H
#define PORYTILES_COLOR_RGBA32_H

#include <cstdint>
#include <string>

#include <porytiles/Color/RGBLike.h>

namespace porytiles::color {

/**
 * @brief Value object representing a color in 32-bit RGBA format.
 *
 * @details
 * The 32-bit RGBA format represents an RGBLike with four 8-bit channels: one channel for red,
 * blue, and green respectively. The last 8-bit channel, the alpha channel, represents color
 * transparency. For the purposes of Porytiles, the only relevant alpha channel values are 0
 * (completely transparent) and 255 (completely opaque). You can read more about the RGBA color
 * format here: <a href="https://en.wikipedia.org/wiki/RGBA_color_model">RGBA Color Model</a>
 */
class Rgba32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

  public:
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
     * 255 (completely opaque).
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
    [[nodiscard]] std::uint8_t computeRedComponent() const
    {
        return red;
    }

    /**
     * @brief Returns the internal green component.
     *
     * @return The internal 8-bit green component.
     */
    [[nodiscard]] std::uint8_t computeGreenComponent() const
    {
        return green;
    }

    /**
     * @brief Returns the internal blue component.
     *
     * @return The internal 8-bit blue component.
     */
    [[nodiscard]] std::uint8_t computeBlueComponent() const
    {
        return blue;
    }

    /**
     * @brief Returns the internal alpha component.
     *
     * @return The internal 8-bit alpha component.
     */
    [[nodiscard]] std::uint8_t computeAlphaComponent() const
    {
        return alpha;
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
     * @brief Equality operator for Rgba32 objects.
     *
     * @details
     * Rgba32 equality is computed component-wise. If each component is equal (including alpha
     * channel), then the two Rgba32 colors are equal.
     *
     * @param lhs The left-hand side Rgba32 object.
     * @param rhs The right-hand side Rgba32 object.
     * @return True if the RGBA components of the two Rgba32 objects are equal, false otherwise.
     */
    friend bool operator==(const Rgba32 &lhs, const Rgba32 &rhs)
    {
        return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue &&
               lhs.alpha == rhs.alpha;
    }

    /**
     * @brief Inequality operator for Rgba32 objects.
     *
     * @details
     * Rgba32 inequality is computed component-wise. If any component is not equal (including alpha
     * channel), then the two Rgba32 colors are not equal.
     *
     * @param lhs The left-hand side Rgba32 object.
     * @param rhs The right-hand side Rgba32 object.
     * @return True if the RGBA components of the two Rgba32 objects are not equal, false otherwise.
     */
    friend bool operator!=(const Rgba32 &lhs, const Rgba32 &rhs)
    {
        return !(lhs == rhs);
    }
};

/**
 * @brief Completely transparent preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_TRANSPARENT{0, 0, 0, ALPHA_TRANSPARENT};

/**
 * @brief Black preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_BLACK{0, 0, 0};

/**
 * @brief Red preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_RED{255, 0, 0};

/**
 * @brief Green preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_GREEN{0, 255, 0};

/**
 * @brief Blue preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_BLUE{0, 0, 255};

/**
 * @brief Yellow preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_YELLOW{255, 255, 0};

/**
 * @brief Cyan preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_CYAN{0, 255, 255};

/**
 * @brief Magenta preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_MAGENTA{255, 0, 255};

/**
 * @brief White preinitialized Rgba32.
 */
static constexpr Rgba32 RGBA_WHITE{255, 255, 255};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA32_H
