#include "tile.h"

namespace porytiles {
template<>
RgbTile::Tile() {
    for (size_t i = 0; i < PIXEL_COUNT; i++) {
        pixels[i] = RgbColor{0, 0, 0};
    }
}

template<>
IndexedTile::Tile() {
    for (size_t i = 0; i < PIXEL_COUNT; i++) {
        pixels[i] = 0;
    }
}

template<>
std::unordered_set<RgbColor> RgbTile::pixelsNotInPalette(const Palette& palette) const {
    std::unordered_set<RgbColor> uniquePixels = this->uniquePixels(gOptTransparentColor);
    std::unordered_set<RgbColor> paletteIndex = palette.getIndex();
    std::unordered_set<RgbColor> pixelsNotInPalette;
    std::copy_if(uniquePixels.begin(), uniquePixels.end(),
                 std::inserter(pixelsNotInPalette, pixelsNotInPalette.begin()),
                 [&paletteIndex](const RgbColor& needle) { return paletteIndex.find(needle) == paletteIndex.end(); });
    return pixelsNotInPalette;
}

template<>
std::unordered_set<png::byte> IndexedTile::pixelsNotInPalette(const Palette& palette) const {
    throw std::runtime_error{"internal: invalid operation IndexedTile::pixelsNotInPalette"};
}

doctest::String toString(const RgbTile& tile) {
    std::string tileString = "Tile<RgbColor>{" + tile.getPixel(0).prettyString() + "..." +
                             tile.getPixel(PIXEL_COUNT - 1).prettyString() + "}";
    return tileString.c_str();
}

doctest::String toString(const IndexedTile& tile) {
    std::string tileString = "Tile<png::byte>{" + std::to_string(tile.getPixel(0)) + "..." +
                             std::to_string(tile.getPixel(PIXEL_COUNT - 1)) + "}";
    return tileString.c_str();
}
} // namespace porytiles

/*
 * Test Cases
 */
TEST_CASE("RgbTile pixelsNotInPalette should return the pixels from this tile that are not in the supplied palette") {
    porytiles::RgbColor red = {255, 0, 0};
    porytiles::RgbColor green = {0, 255, 0};
    porytiles::RgbColor blue = {0, 0, 255};
    porytiles::RgbColor white = {255, 255, 255};
    porytiles::RgbColor black = {0, 0, 0};
    porytiles::RgbColor magenta = {255, 0, 255};
    porytiles::Palette palette{};
    palette.addColorAtEnd(red);
    palette.addColorAtEnd(green);
    palette.addColorAtEnd(blue);
    palette.pushFrontTransparencyColor();
    porytiles::RgbTile tile{magenta};

    SUBCASE("It should return a set of size 2 with colors black and white") {
        tile.setPixel(0, red);
        tile.setPixel(1, green);
        tile.setPixel(2, blue);
        tile.setPixel(3, black);
        tile.setPixel(4, white);

        std::unordered_set<porytiles::RgbColor> pixelsNotInPalette = tile.pixelsNotInPalette(palette);
        CHECK(pixelsNotInPalette.size() == 2);
        CHECK(pixelsNotInPalette.find(black) != pixelsNotInPalette.end());
        CHECK(pixelsNotInPalette.find(white) != pixelsNotInPalette.end());
    }
    SUBCASE("It should return a set of size 0") {
        tile.setPixel(0, red);
        tile.setPixel(1, green);
        tile.setPixel(2, blue);
        std::unordered_set<porytiles::RgbColor> pixelsNotInPalette = tile.pixelsNotInPalette(palette);
        CHECK(pixelsNotInPalette.empty());
    }
}
