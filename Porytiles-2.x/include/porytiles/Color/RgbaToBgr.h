#ifndef PORYTILES_COLOR_RGBA_TO_BGR_H
#define PORYTILES_COLOR_RGBA_TO_BGR_H

#include <porytiles/Color/Rgba32.h>
#include <porytiles/Color/Bgr15.h>
#include <porytiles/Color/ColorConverter.h>

namespace porytiles::color {

/**
 * @brief A ColorConverter implementation that converts an Rgba32 into a Bgr15.
 */
class RgbaToBgr final : public ColorConverter<Rgba32, Bgr15> {
  public:
    /**
     * @brief Converts an Rgba32 color to a Bgr15 color format.
     *
     * @details
     * This method takes an Rgba32 color object, extracts its red, green, and
     * blue components, and converts it to a Bgr15 color object.
     *
     * @param rgba A constant reference to an Rgba32 color object.
     * @return An equivalent Bgr15 color object.
     */
    [[nodiscard]] Bgr15 convert(const Rgba32 &rgba) const override;
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_RGBA_TO_BGR_H