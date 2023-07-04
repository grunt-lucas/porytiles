#include "bgr15.h"

#include <cstdint>
#include <iostream>
#include <compare>

#include "rgba32.h"

namespace porytiles {
std::ostream& operator<<(std::ostream& os, const BGR15& myBgr) {
    os << "BGR15{" << std::to_string(myBgr.bgr) << "}";
    return os;
}

BGR15 rgbaToBGR(const RGBA32& rgba) {
    return {std::uint16_t(((rgba.blue / 8) << 10) | ((rgba.green / 8) << 5) | (rgba.red / 8))};
}
}