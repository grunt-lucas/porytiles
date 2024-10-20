#include "porytiles/Color/Bgr15.h"

using namespace porytiles::color;

std::uint16_t Bgr15::getRawValue() const { return bgr; }

Bgr15::Bgr15(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue)
{
    bgr = static_cast<std::uint16_t>(((blue / 8) << 10) | ((green / 8) << 5) | (red / 8));
}
