#include "rgba32.h"

#include <cstdint>
#include <compare>
#include <iostream>

namespace porytiles {
std::ostream& operator<<(std::ostream& os, const RGBA32& myRgba) {
    os << "RGBA32{" << std::to_string(myRgba.red) << "," << std::to_string(myRgba.green) << ","
       << std::to_string(myRgba.blue) << "," << std::to_string(myRgba.alpha) << "}";
    return os;
}
}
