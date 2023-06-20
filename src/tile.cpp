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