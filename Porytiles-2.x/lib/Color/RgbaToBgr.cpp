#include "porytiles/Color/RgbaToBgr.h"

#include <porytiles/Color/Bgr15.h>
#include <porytiles/Color/Rgba32.h>

using namespace porytiles::color;

Bgr15 RgbaToBgr::convert(const Rgba32 &rgba) const
{
    const auto &rgb = dynamic_cast<const Rgba32 &>(rgba);
    const Bgr15 bgr{rgb.computeRedComponent(), rgb.computeGreenComponent(),
                    rgb.computeBlueComponent()};
    return bgr;
}