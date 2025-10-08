#pragma once

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/tile_constants.hpp"

namespace porytiles2 {

class RgbaTile final : public PixelTile<Rgba32> {
  public:
    RgbaTile() = default;

    /**
     * @brief Construct RgbaTile from base PixelTile<Rgba32>.
     *
     * @details
     * Conversion constructor that allows implicit conversion from PixelTile<Rgba32> to RgbaTile. This enables seamless
     * interoperability between the base tile type and RGBA-specific tile type.
     *
     * @param base_tile The base tile to convert from
     */
    explicit RgbaTile(const PixelTile &base_tile)
    {
        for (std::size_t i = 0; i < tile::size_pix; i++) {
            set(i, base_tile.at(i));
        }
    }
};

} // namespace porytiles2
