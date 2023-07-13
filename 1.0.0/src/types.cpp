#include "types.h"

namespace porytiles {

std::ostream& operator<<(std::ostream& os, const BGR15& bgr) {
    os << std::to_string(bgr.bgr);
    return os;
}

constexpr RGBA32 RGBA_BLACK = RGBA32{0, 0, 0, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_RED = RGBA32{255, 0, 0, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_GREEN = RGBA32{0, 255, 0, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_BLUE = RGBA32{0, 0, 255, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_YELLOW = RGBA32{255, 255, 0, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_MAGENTA = RGBA32{255, 0, 255, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_CYAN = RGBA32{0, 255, 255, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_WHITE = RGBA32{255, 255, 255, ALPHA_OPAQUE};
constexpr RGBA32 RGBA_GREY = RGBA32{128, 128, 128, ALPHA_OPAQUE};

std::ostream& operator<<(std::ostream& os, const RGBA32& rgba) {
    // For debugging purposes, print the solid colors with names rather than int values
    if (rgba == RGBA_BLACK) {
        os << "black";
    }
    else if (rgba == RGBA_RED) {
        os << "red";
    }
    else if (rgba == RGBA_GREEN) {
        os << "green";
    }
    else if (rgba == RGBA_BLUE) {
        os << "blue";
    }
    else if (rgba == RGBA_YELLOW) {
        os << "yellow";
    }
    else if (rgba == RGBA_MAGENTA) {
        os << "magenta";
    }
    else if (rgba == RGBA_CYAN) {
        os << "cyan";
    }
    else if (rgba == RGBA_WHITE) {
        os << "white";
    }
    else {
        os << std::to_string(rgba.red) << "," << std::to_string(rgba.green) << ","
           << std::to_string(rgba.blue);
        if (rgba.alpha != 255) {
            // Only show alpha if not opaque
            os << "," << std::to_string(rgba.alpha);
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const RGBATile& tile) {
    os << "{";
    for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
        if (i % TILE_SIDE_LENGTH == 0) {
            os << "[" << i / 8 << "]=";
        }
        os << tile.pixels[i] << ";";
    }
    os << "}";
    return os;
}

consteval RGBATile uniformTile(const RGBA32& color) {
    RGBATile tile{};
    for (size_t i = 0; i < TILE_NUM_PIX; i++) {
        tile.pixels[i] = color;
    }
    return tile;
}

constexpr RGBATile RGBA_TILE_BLACK = uniformTile(RGBA_BLACK);
constexpr RGBATile RGBA_TILE_RED = uniformTile(RGBA_RED);
constexpr RGBATile RGBA_TILE_GREEN = uniformTile(RGBA_GREEN);
constexpr RGBATile RGBA_TILE_BLUE = uniformTile(RGBA_BLUE);
constexpr RGBATile RGBA_TILE_YELLOW = uniformTile(RGBA_YELLOW);
constexpr RGBATile RGBA_TILE_MAGENTA = uniformTile(RGBA_MAGENTA);
constexpr RGBATile RGBA_TILE_CYAN = uniformTile(RGBA_CYAN);
constexpr RGBATile RGBA_TILE_WHITE = uniformTile(RGBA_WHITE);
//constexpr RGBATile RGBA_TILE_GREY = uniformTile(RGBA_GREY);

BGR15 rgbaToBgr(const RGBA32& rgba) {
    // Convert each color channel from 8-bit to 5-bit, then shift into the right position
    return {std::uint16_t(((rgba.blue / 8) << 10) | ((rgba.green / 8) << 5) | (rgba.red / 8))};
}

RGBA32 bgrToRgba(const BGR15& bgr) {
    RGBA32 rgba{};
    rgba.red = (bgr.bgr & 0x1f) * 8;
    rgba.green = ((bgr.bgr >> 5) & 0x1f) * 8;
    rgba.blue = ((bgr.bgr >> 10) & 0x1f) * 8;
    rgba.alpha = 255;
    return rgba;
}

}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("RGBA32 to BGR15 should lose precision") {
    porytiles::RGBA32 rgb1 = {0, 1, 2, 3};
    porytiles::RGBA32 rgb2 = {255, 255, 255, 255};

    porytiles::BGR15 bgr1 = rgbaToBgr(rgb1);
    porytiles::BGR15 bgr2 = rgbaToBgr(rgb2);

    CHECK(bgr1 == porytiles::BGR15{0});
    CHECK(bgr2 == porytiles::BGR15{32767}); // this value is uint16 max divided by two, i.e. 15 bits are set
}

TEST_CASE("RGBA32 should be ordered component-wise") {
    porytiles::RGBA32 rgb1 = {0, 1, 2, 3};
    porytiles::RGBA32 rgb2 = {1, 2, 3, 4};
    porytiles::RGBA32 rgb3 = {2, 3, 4, 5};
    porytiles::RGBA32 zeros = {0, 0, 0, 0};

    CHECK(zeros == zeros);
    CHECK(zeros < rgb1);
    CHECK(rgb1 < rgb2);
    CHECK(rgb2 < rgb3);
}

TEST_CASE("BGR15 to RGBA should upconvert RGB channels to multiples of 8") {
    porytiles::RGBA32 rgb1 = {0, 8, 80, 255};
    porytiles::RGBA32 rgb2 = {255, 255, 255, 255};
    porytiles::RGBA32 rgb3 = {2, 165, 96, 255};

    porytiles::BGR15 bgr1 = rgbaToBgr(rgb1);
    porytiles::BGR15 bgr2 = rgbaToBgr(rgb2);
    porytiles::BGR15 bgr3 = rgbaToBgr(rgb3);

    CHECK(porytiles::bgrToRgba(bgr1) == porytiles::RGBA32{0, 8, 80, 255});
    CHECK(porytiles::bgrToRgba(bgr2) == porytiles::RGBA32{248, 248, 248, 255});
    CHECK(porytiles::bgrToRgba(bgr3) == porytiles::RGBA32{0, 160, 96, 255});
}
