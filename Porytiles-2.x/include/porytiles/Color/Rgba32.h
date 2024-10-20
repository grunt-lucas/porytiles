#ifndef PORYTILES_COLOR_RGBA_H
#define PORYTILES_COLOR_RGBA_H

#include <cstdint>
#include <string>

namespace porytiles::color {

/**
 * @brief Value object class representing a color in 32-bit RGBA format.
 */
class Rgba32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

public:
    Rgba32() : red{0}, green{0}, blue {0}, alpha{0} {}

  /**
   * @brief Converts the RGBA color to a JASC-PAL formatted string.
   *
   * This method converts the red, green, and blue components of the Rgba32 object
   * into a space-separated string format commonly used in JASC-PAL color palette files.
   *
   * @return A string representation of the color in JASC-PAL format.
   */
  [[nodiscard]] std::string toJascString() const;
};

} // end namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA_H
