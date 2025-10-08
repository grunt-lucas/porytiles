#pragma once

#include <utility>

#include "porytiles2/domain/models/tile/metatile.hpp"
#include "porytiles2/domain/models/tile/pixel_tile.hpp"
#include "porytiles2/domain/models/tile/rgba_tile.hpp"

namespace porytiles2 {

class RgbaMetatile : public Metatile<Rgba32> {
  public:
    RgbaMetatile() = default;

    /**
     * @brief Set a RgbaTile in the bottom layer.
     *
     * @details
     * Convenience overload that accepts RgbaTile directly, avoiding the need for explicit casting at call sites.
     * The RgbaTile is converted to Tile<Rgba32> for storage in the base class.
     *
     * @param i The index into the bottom layer array (must be 0-3)
     * @param tile The RgbaTile to move into the array
     */
    void set_bottom(std::size_t i, const RgbaTile &tile)
    {
        Metatile::set_bottom(i, static_cast<PixelTile<Rgba32>>(tile));
    }

    /**
     * @brief Set a RgbaTile in the middle layer.
     *
     * @details
     * Convenience overload that accepts RgbaTile directly, avoiding the need for explicit casting at call sites.
     * The RgbaTile is converted to Tile<Rgba32> for storage in the base class.
     *
     * @param i The index into the middle layer array (must be 0-3)
     * @param tile The RgbaTile to move into the array
     */
    void set_middle(std::size_t i, const RgbaTile &tile)
    {
        Metatile::set_middle(i, static_cast<PixelTile<Rgba32>>(tile));
    }

    /**
     * @brief Set a RgbaTile in the top layer.
     *
     * @details
     * Convenience overload that accepts RgbaTile directly, avoiding the need for explicit casting at call sites.
     * The RgbaTile is converted to Tile<Rgba32> for storage in the base class.
     *
     * @param i The index into the top layer array (must be 0-3)
     * @param tile The RgbaTile to move into the array
     */
    void set_top(std::size_t i, const RgbaTile &tile)
    {
        Metatile::set_top(i, static_cast<PixelTile<Rgba32>>(tile));
    }
};

} // namespace porytiles2
