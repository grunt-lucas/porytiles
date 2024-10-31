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
    return Bgr15{static_cast<std::uint16_t>(rgba.computeBlueComponent() >> 3 << 10 |
                                            rgba.computeGreenComponent() >> 3 << 5 |
                                            rgba.computeRedComponent() >> 3)};
}

Rgba32 bgrToRgba(const Bgr15 &bgr)
{
    return Rgba32{bgr.computeRedComponent(), bgr.computeGreenComponent(), bgr.computeBlueComponent()};
}
}