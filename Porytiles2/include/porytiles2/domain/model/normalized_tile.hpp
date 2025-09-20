#pragma once

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/normalized_pal.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

/**
 * @brief A tile with normalized pixel data and flip state information.
 *
 * @details
 * NormalizedTile extends the base Tile class to store IndexPixel data along with horizontal and vertical flip states.
 * It also maintains an internal normalized palette.
 *
 * @tparam ColorType The type of color objects used in the tile's normalized palette
 */
template <typename ColorType>
class NormalizedTile final : public Tile<IndexPixel> {
  public:
    /**
     * @brief Constructs a NormalizedTile with the specified flip states.
     *
     * @param h_flip Whether the tile should be horizontally flipped
     * @param v_flip Whether the tile should be vertically flipped
     */
    NormalizedTile(bool h_flip, bool v_flip) : h_flip_{h_flip}, v_flip_{v_flip} {}

    /**
     * @brief Returns the horizontal flip state of the tile.
     *
     * @return True if the tile is horizontally flipped, false otherwise
     */
    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    /**
     * @brief Returns the vertical flip state of the tile.
     *
     * @return True if the tile is vertically flipped, false otherwise
     */
    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

  private:
    bool h_flip_;
    bool v_flip_;
    NormalizedPal<ColorType> pal_;
};

} // namespace porytiles2
