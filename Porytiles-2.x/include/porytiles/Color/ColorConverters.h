#ifndef PORYTILES_COLOR_COLOR_CONVERTERS_H
#define PORYTILES_COLOR_COLOR_CONVERTERS_H

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/Rgba32.h"

namespace porytiles::color {

Bgr15 rgbaToBgr(const Rgba32 &rgba);

Rgba32 bgrToRgba(const Bgr15 &bgr);

} // namespace porytiles::color

#endif // PORYTILES_COLOR_COLOR_CONVERTERS_H
