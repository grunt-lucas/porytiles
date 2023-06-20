#include "palette.h"

#include "cli_parser.h"
#include "rgb_color.h"

#include <doctest.h>

namespace porytiles {
void Palette::addColorAtEnd(const RgbColor& color) {
    if (colors.size() >= PAL_SIZE_4BPP) {
        throw std::length_error{"internal: tried to add color " + color.prettyString() + " to full palette"};
    }
    index.insert(color);
    colors.push_back(color);
}

RgbColor Palette::colorAt(size_t i) const {
    if (i >= colors.size()) {
        throw std::out_of_range{"internal: Palette::colorAt request index " + std::to_string(i) + " out of range"};
    }
    return colors[i];
}

size_t Palette::indexOf(const RgbColor& color) const {
    size_t idx = 0;
    for (const auto& iterColor: colors) {
        if (iterColor == color)
            return idx;
        idx++;
    }
    throw std::runtime_error{"internal: color " + color.prettyString() + " not found in palette"};
}

void Palette::pushFrontTransparencyColor() {
    index.insert(gOptTransparentColor);
    colors.push_front(gOptTransparentColor);
}

void Palette::pushBackZeroedColor() {
    index.insert(RgbColor{0, 0, 0});
    colors.emplace_back(0, 0, 0);
}

size_t Palette::size() const {
    return colors.size();
}

size_t Palette::unusedColorsCount() const {
    return PAL_SIZE_4BPP - colors.size();
}

int Palette::paletteWithFewestColors(const std::vector<Palette>& palettes) {
    int indexOfMin = 0;
    // Palettes can only have 15 colors, so 1000 will work fine as a starting value
    size_t minColors = 1000;
    for (size_t i = 0; i < palettes.size(); i++) {
        auto size = palettes.at(i).size();
        if (size < minColors) {
            indexOfMin = static_cast<int>(i);
            minColors = size;
        }
    }
    return indexOfMin;
}
} // namespace porytiles

/*
 * Test Cases
 */
TEST_CASE("Palette addColorAtEnd should add the color at the end or throw if palette is full") {
    porytiles::RgbColor red{255, 0, 0};
    porytiles::RgbColor green{0, 255, 0};
    porytiles::RgbColor blue{0, 0, 255};
    porytiles::Palette palette{};

    SUBCASE("It should have size of three and contain red, green, blue in that order") {
        palette.addColorAtEnd(red);
        palette.addColorAtEnd(green);
        palette.addColorAtEnd(blue);

        CHECK(palette.size() == 3);
        CHECK(palette.indexOf(red) == 0);
        CHECK(palette.indexOf(green) == 1);
        CHECK(palette.indexOf(blue) == 2);
    }
    SUBCASE("It should throw a std::length_error when the palette is full") {
        for (size_t i = 0; i < porytiles::PAL_SIZE_4BPP; i++) {
            palette.addColorAtEnd(red);
        }
        CHECK_THROWS_WITH_AS(palette.addColorAtEnd(red), "internal: tried to add color 255,0,0 to full palette",
                             const std::length_error&);
    }
}

TEST_CASE("Palette colorAt should return the requested index or throw if out of range") {
    porytiles::RgbColor red{255, 0, 0};
    porytiles::RgbColor green{0, 255, 0};
    porytiles::RgbColor blue{0, 0, 255};
    porytiles::RgbColor magenta{255, 0, 255};
    porytiles::Palette palette{};
    palette.addColorAtEnd(red);
    palette.addColorAtEnd(green);
    palette.addColorAtEnd(blue);
    palette.addColorAtEnd(magenta);

    SUBCASE("It should return green, at index 1") {
        CHECK(palette.colorAt(1) == green);
    }
    SUBCASE("It should throw std::out_of_range if index is not in range") {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(palette.colorAt(12), "internal: Palette::colorAt request index 12 out of range",
                             const std::out_of_range&);
#pragma GCC diagnostic pop
    }
}

TEST_CASE("Palette indexOf should return the index of the requested color or throw if not present") {
    porytiles::RgbColor red{255, 0, 0};
    porytiles::RgbColor green{0, 255, 0};
    porytiles::RgbColor blue{0, 0, 255};
    porytiles::Palette palette{};
    palette.addColorAtEnd(red);
    palette.addColorAtEnd(green);
    palette.addColorAtEnd(blue);

    SUBCASE("It should return the index of the requested color") {
        CHECK(palette.indexOf(red) == 0);
        CHECK(palette.indexOf(green) == 1);
        CHECK(palette.indexOf(blue) == 2);
    }
    SUBCASE("It should throw if the color is not present in the palette") {
        porytiles::RgbColor magenta{255, 0, 255};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(palette.indexOf(magenta), "internal: color 255,0,255 not found in palette",
                             const std::runtime_error&);
#pragma GCC diagnostic pop
    }
}
