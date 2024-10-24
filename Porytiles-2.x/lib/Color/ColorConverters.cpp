#include "porytiles/Color/ColorConverters.h"

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/Rgba32.h"

namespace porytiles::color {
Bgr15 rgbaToBgr(const Rgba32 &rgba)
{
    /*
     * Convert each color channel from 8-bit to 5-bit via a right shift of 3, then shift back into
     * the correct position.
     */
    return Bgr15{static_cast<std::uint16_t>(rgba.getBlueComponent() >> 3 << 10 |
                                            rgba.getGreenComponent() >> 3 << 5 |
                                            rgba.getRedComponent() >> 3)};
}

Rgba32 bgrToRgba(const Bgr15 &bgr)
{
    const auto rawBgr = bgr.getRawValue();
    constexpr auto mask = 0x1f;
    const std::uint8_t redComponent = (rawBgr & mask) << 3;
    const std::uint8_t greenComponent = (rawBgr >> 5 & mask) << 3;
    const std::uint8_t blueComponent = (rawBgr >> 10 & mask) << 3;
    return Rgba32{redComponent, greenComponent, blueComponent};
}
}