#pragma once

#include <algorithm>
#include <array>

#include "porytiles2/domain/model/supports_transparency.hpp"
#include "porytiles2/domain/model/tile/tile_constants.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief An 8x8 tile backed by literal-array-based per-pixel storage of an arbitrary pixel type.
 *
 * @tparam PixelType The pixel type of this Tile
 */
template <typename PixelType>
    requires SupportsTransparency<PixelType>
class PixelTile {
  public:
    virtual ~PixelTile() = default;

    explicit PixelTile() : pix_{} {}

    explicit PixelTile(const std::array<PixelType, tile::size_pix> &pix) : pix_{pix} {}

    bool operator==(const PixelTile &) const = default;

    auto operator<=>(const PixelTile &) const = default;

    /**
     * @brief Checks if this entire PixelTile is transparent.
     *
     * @details
     * A PixelTile is transparent if all of its pixels are either intrinsically or extrinsically transparent, according
     * to the provided extrinsic transparency value.
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return True if all pixels in the PixelTile are transparent, false otherwise
     */
    [[nodiscard]] virtual bool is_transparent(const PixelType &extrinsic) const
    {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel.is_transparent(extrinsic); });
    }

    [[nodiscard]] PixelType at(std::size_t i) const
    {
        if (i >= tile::size_pix) {
            panic("index out of bounds: " + std::to_string(i));
        }
        return pix_[i];
    }

    [[nodiscard]] PixelType at(std::size_t row, std::size_t col) const
    {
        if (row >= tile::side_length_pix) {
            panic("row index out of bounds: " + std::to_string(row));
        }
        if (col >= tile::side_length_pix) {
            panic("col index out of bounds: " + std::to_string(col));
        }
        return pix_[row * tile::side_length_pix + col];
    }

    void set(std::size_t i, const PixelType &p)
    {
        if (i >= tile::size_pix) {
            panic("index out of bounds: " + std::to_string(i));
        }
        pix_[i] = p;
    }

    void set(std::size_t row, std::size_t col, const PixelType &p)
    {
        if (row >= tile::side_length_pix) {
            panic("row index out of bounds: " + std::to_string(row));
        }
        if (col >= tile::side_length_pix) {
            panic("col index out of bounds: " + std::to_string(col));
        }
        pix_[row * tile::side_length_pix + col] = p;
    }

    /**
     * @brief Creates a flipped copy of this PixelTile.
     *
     * @details
     * Returns a new PixelTile flipped according to the specified parameters. Horizontal flip reflects the tile across a
     * vertical axis, vertical flip reflects across a horizontal axis.
     *
     * @param h_flip Whether to flip horizontally
     * @param v_flip Whether to flip vertically
     * @return A new PixelTile with the specified flips applied
     */
    [[nodiscard]] PixelTile flip(bool h_flip, bool v_flip) const
    {
        PixelTile flipped_tile{};
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const std::size_t src_row = v_flip ? (tile::side_length_pix - 1 - row) : row;
                const std::size_t src_col = h_flip ? (tile::side_length_pix - 1 - col) : col;
                flipped_tile.set(row, col, at(src_row, src_col));
            }
        }
        return flipped_tile;
    }

  protected:
    [[nodiscard]] const std::array<PixelType, tile::size_pix> &pix() const
    {
        return pix_;
    }

  private:
    std::array<PixelType, tile::size_pix> pix_;
};

} // namespace porytiles2
