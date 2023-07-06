#include "types.h"

namespace porytiles {
std::ostream& operator<<(std::ostream& os, const BGR15& bgr) {
    os << "bgr{" << std::to_string(bgr.bgr) << "}";
    return os;
}

constexpr RGBA32 RGBA_BLACK = RGBA32{0, 0, 0, 255};
constexpr RGBA32 RGBA_RED = RGBA32{255, 0, 0, 255};
constexpr RGBA32 RGBA_GREEN = RGBA32{0, 255, 0, 255};
constexpr RGBA32 RGBA_BLUE = RGBA32{0, 0, 255, 255};
constexpr RGBA32 RGBA_YELLOW = RGBA32{255, 255, 0, 255};
constexpr RGBA32 RGBA_MAGENTA = RGBA32{255, 0, 255, 255};
constexpr RGBA32 RGBA_CYAN = RGBA32{0, 255, 255, 255};
constexpr RGBA32 RGBA_WHITE = RGBA32{255, 255, 255, 255};

std::ostream& operator<<(std::ostream& os, const RGBA32& rgba) {
    // For debugging purposes, print the solid colors with names rather than int values
    std::string rgbString = "rgb";
    if (rgba == RGBA_BLACK) {
        os << rgbString << "{black}";
    }
    else if (rgba == RGBA_RED) {
        os << rgbString << "{red}";
    }
    else if (rgba == RGBA_GREEN) {
        os << rgbString << "{green}";
    }
    else if (rgba == RGBA_BLUE) {
        os << rgbString << "{blue}";
    }
    else if (rgba == RGBA_YELLOW) {
        os << rgbString << "{yellow}";
    }
    else if (rgba == RGBA_MAGENTA) {
        os << rgbString << "{magenta}";
    }
    else if (rgba == RGBA_CYAN) {
        os << rgbString << "{cyan}";
    }
    else if (rgba == RGBA_WHITE) {
        os << rgbString << "{white}";
    }
    else {
        os << rgbString << "{" << std::to_string(rgba.red) << "," << std::to_string(rgba.green) << ","
           << std::to_string(rgba.blue) << "," << std::to_string(rgba.alpha) << "}";
    }
    return os;
}

BGR15 rgbaToBGR(const RGBA32& rgba) {
    // Convert each color channel from 8-bit to 5-bit, then shift into the right position
    return {std::uint16_t(((rgba.blue / 8) << 10) | ((rgba.green / 8) << 5) | (rgba.red / 8))};
}


// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("RGBA32 to BGR15 should lose precision") {
    RGBA32 rgb1 = {0, 1, 2, 3};
    RGBA32 rgb2 = {255, 255, 255, 255};

    BGR15 bgr1 = rgbaToBGR(rgb1);
    BGR15 bgr2 = rgbaToBGR(rgb2);

    CHECK(bgr1 == BGR15{0});
    CHECK(bgr2 == BGR15{32767}); // this value is uint16 max divided by two, i.e. 15 bits are set
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