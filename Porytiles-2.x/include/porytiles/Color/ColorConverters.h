#ifndef PORYTILES_COLOR_COLOR_CONVERTERS_H
#define PORYTILES_COLOR_COLOR_CONVERTERS_H

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/Rgba32.h"

namespace porytiles::color {

/**
 * @brief Converts an Rgba32 color to a 15-bit Bgr15 color.
 *
 * @param rgba The Rgba32 color object to convert.
 * @return The converted Bgr15 color.
 */
Bgr15 rgbaToBgr(const Rgba32 &rgba);

/**
 * @brief Converts a Bgr15 color to an Rgba32 color.
 *
 * @param bgr The Bgr15 color object to convert.
 * @return The converted Rgba32 color.
 */
Rgba32 bgrToRgba(const Bgr15 &bgr);

} // namespace porytiles::color

#endif // PORYTILES_COLOR_COLOR_CONVERTERS_H
