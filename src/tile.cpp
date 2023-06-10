#include "tile.h"

namespace tscreate {
template<>
bool RgbTile::isControlTile() const {
    return isUniformly(gOptTransparentColor) || isUniformly(gOptPrimerColor);
}

template<>
bool IndexedTile::isControlTile() const {
    throw std::runtime_error{"internal: invalid operation IndexedTile::isControlTile"};
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
}