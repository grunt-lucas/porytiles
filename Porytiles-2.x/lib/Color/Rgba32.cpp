#include "porytiles/Color/Rgba32.h"

#include <string>

using namespace porytiles::color;

std::string Rgba32::toJascString() const
{
    return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
}
