#pragma once

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

class RgbaTile final : public Tile<Rgba32> {
  public:
    RgbaTile() = default;

    /**
     * @brief Construct RgbaTile from base Tile<Rgba32>.
     *
     * @details
     * Conversion constructor that allows implicit conversion from Tile<Rgba32> to RgbaTile. This enables seamless
     * interoperability between the base tile type and RGBA-specific tile type.
     *
     * @param base_tile The base tile to convert from
     */
    explicit RgbaTile(const Tile &base_tile)
    {
        for (std::size_t i = 0; i < tile_size; ++i) {
            set(i, base_tile.at(i));
        }
    }
};

} // namespace porytiles2
