#ifndef PORYTILES_BGR15_H
#define PORYTILES_BGR15_H

#include <cstdint>
#include <iostream>
#include <compare>

#include "doctest.h"
#include "rgba32.h"

namespace porytiles {
struct BGR15 {
    std::uint16_t bgr;

    auto operator<=>(BGR15 const&) const = default;
};

std::ostream& operator<<(std::ostream& os, const BGR15& myBgr);

BGR15 rgbaToBGR(const RGBA32& rgba);

TEST_CASE("RGBA32 to BGR15 should lose precision") {
    RGBA32 rgb1 = {0, 1, 2, 3};
    RGBA32 rgb2 = {255, 255, 255, 255};

    BGR15 bgr1 = rgbaToBGR(rgb1);
    BGR15 bgr2 = rgbaToBGR(rgb2);

    CHECK(bgr1 == BGR15{0});
    CHECK(bgr2 == BGR15{32767}); // this value is uint16 max divided by two, i.e. 15 bits are set
}
}

#endif //PORYTILES_BGR15_H
