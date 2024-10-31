#include "porytiles/Color/Bgr15.h"

using namespace porytiles::color;

namespace {
    constexpr auto BGR_MASK = 0x1f;
}

std::uint16_t Bgr15::getRawValue() const
{
    return bgr;
}

std::uint8_t Bgr15::computeRedComponent() const
{
    return (getRawValue() & BGR_MASK) << 3;
}

std::uint8_t Bgr15::computeGreenComponent() const
{
    return (getRawValue() >> 5 & BGR_MASK) << 3;
}

std::uint8_t Bgr15::computeBlueComponent() const
{
    return (getRawValue() >> 10 & BGR_MASK) << 3;
}
