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

std::ostream& operator<<(std::ostream& os, const BGR15& myBgr) {
    os << "BGR15{" << std::to_string(myBgr.bgr) << "}";
    return os;
}

static inline BGR15 toBGR(RGBA32 rgba) {
    return {std::uint16_t(((rgba.blue / 8) << 10) | ((rgba.green / 8) << 5) | (rgba.red / 8))};
}

TEST_CASE("RGBA32 to BGR15 should lose precision") {
    RGBA32 rgb1 = {0, 1, 2, 3};
    RGBA32 rgb2 = {255, 255, 255, 255};

    BGR15 bgr1 = toBGR(rgb1);
    BGR15 bgr2 = toBGR(rgb2);

    CHECK(bgr1 == BGR15{0});
    CHECK(bgr2 == BGR15{32767}); // this value is uint16 max divided by two, i.e. 15 bits are set
}
}

#endif //PORYTILES_BGR15_H
