#include "tiles/rgba_tile.hpp"

#include "colors/rgba32.hpp"

namespace porytiles {

bool RgbaTile::IsTransparent(const Rgba32 &transparency_rgba) const {
    return std::ranges::all_of(pix(), [=](const auto &pixel) {
        return pixel == transparency_rgba || pixel.alpha() == Rgba32::kAlphaTransparent;
    });
}

bool RgbaTile::EqualsBgr(const RgbaTile &other) const {
    // for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
    //     if (rgbaToBgr(this->pixels.at(i)) != rgbaToBgr(other.pixels.at(i))) {
    //         return false;
    //     }
    // }
    return true;
}

}; // namespace porytiles
