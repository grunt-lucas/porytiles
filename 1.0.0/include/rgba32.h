#ifndef PORYTILES_RGBA32_H
#define PORYTILES_RGBA32_H

#include <cstdint>
#include <compare>
#include <iostream>

#include "doctest.h"

namespace porytiles {
struct RGBA32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

    auto operator<=>(const RGBA32&) const = default;
};

std::ostream& operator<<(std::ostream& os, const RGBA32& myRgba) {
    os << "RGBA32{" << std::to_string(myRgba.red) << "," << std::to_string(myRgba.green) << "," << std::to_string(myRgba.blue) << "," << std::to_string(myRgba.alpha) << "}";
    return os;
}

TEST_CASE("RGBA32 should be ordered component-wise") {
    RGBA32 rgb1 = {0, 1, 2, 3};
    RGBA32 rgb2 = {1, 2, 3, 4};
    RGBA32 rgb3 = {2, 3, 4, 5};
    RGBA32 zeros = {0, 0, 0, 0};

    CHECK(zeros == zeros);
    CHECK(zeros < rgb1);
    CHECK(rgb1 < rgb2);
    CHECK(rgb2 < rgb3);
}
}

#endif