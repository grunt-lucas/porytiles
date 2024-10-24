#include "porytiles/Color/Rgba32.h"

#include <string>

using namespace porytiles::color;

std::string Rgba32::toJascString() const
{
    return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
}

std::uint8_t Rgba32::getRedComponent() const
{
    return red;
}

std::uint8_t Rgba32::getGreenComponent() const
{
    return green;
}

std::uint8_t Rgba32::getBlueComponent() const
{
    return blue;
}

std::uint8_t Rgba32::getAlphaComponent() const
{
    return alpha;
}
