#include "rgb_color.h"

#include <string>

namespace porytiles {
bool RgbColor::operator==(const RgbColor& other) const {
    return red == other.red && green == other.green && blue == other.blue;
}

bool RgbColor::operator!=(const RgbColor& other) const {
    return !(*this == other);
}

std::string RgbColor::prettyString() const {
    return std::to_string(red) + "," + std::to_string(green) + "," + std::to_string(blue);
}

std::string RgbColor::jascString() const {
    return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
}

doctest::String toString(const RgbColor& color) {
    std::string colorString = "RgbColor{" + std::to_string(color.getRed()) + "," +
                              std::to_string(color.getGreen()) + "," + std::to_string(color.getBlue()) + "}";
    return colorString.c_str();
}
} // namespace porytiles

/*
 * Test Cases
 */
TEST_CASE("Logically equivalent RgbColor objects should be equivalent under op==") {
    SUBCASE("Logically equivalent RgbColors should match") {
        porytiles::RgbColor color1{255, 0, 0};
        porytiles::RgbColor color2{255, 0, 0};
        CHECK(color1 == color2);
    }
    SUBCASE("Logically divergent RgbColors should not match") {
        porytiles::RgbColor color1{255, 0, 0};
        porytiles::RgbColor color2{0, 255, 0};
        CHECK(color1 != color2);
    }
}

TEST_CASE("RgbColor string-ify methods should yield expected values") {
    SUBCASE("RgbColor prettyString should yield expected result") {
        porytiles::RgbColor white{255, 255, 255};
        CHECK(white.prettyString() == "255,255,255");
    }
    SUBCASE("RgbColor jascString should yield expected result") {
        porytiles::RgbColor white{255, 255, 255};
        CHECK(white.jascString() == "255 255 255");
    }
}
