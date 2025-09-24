#pragma once

#include <algorithm>
#include <array>
#include <set>

#include "porytiles2/domain/model/supports_transparency.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

/**
 * @brief An 8x8 pixel value object with an arbitrary pixel data type.
 *
 * @tparam PixelType The pixel type of this Tile
 */
template <typename PixelType>
    requires SupportsTransparency<PixelType>
class Tile {
  public:
    static constexpr std::size_t tile_side_length = 8;
    static constexpr std::size_t tile_size = tile_side_length * tile_side_length;

    virtual ~Tile() = default;

    explicit Tile() : pix_{} {}

    bool operator==(const Tile &) const = default;
    auto operator<=>(const Tile &) const = default;

    /**
     * @brief Checks if this entire tile is transparent.
     *
     * @details
     * A tile is transparent if all of its pixels are either intrinsically transparent or are extrinsically transparent,
     * according to the provided extrinsic transparency values.
     *
     * @param extrinsics The extrinsic transparency values to check each pixel against
     * @return True if all pixels in the tile are transparent, false otherwise
     */
    [[nodiscard]] virtual bool is_transparent(const std::set<PixelType> &extrinsics) const
    {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel.is_transparent(extrinsics); });
    }

    [[nodiscard]] PixelType at(std::size_t i) const
    {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        return pix_[i];
    }

    [[nodiscard]] PixelType at(std::size_t row, std::size_t col) const
    {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        return pix_[row * tile_side_length + col];
    }

    void set(std::size_t i, const PixelType &p)
    {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        pix_[i] = p;
    }

    void set(std::size_t row, std::size_t col, const PixelType &p)
    {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        pix_[row * tile_side_length + col] = p;
    }

    /**
     * @brief Creates a flipped copy of this tile.
     *
     * @details
     * Returns a new tile flipped according to the specified parameters. Horizontal flip reflects the tile across a
     * vertical axis, vertical flip reflects across a horizontal axis.
     *
     * @param h_flip Whether to flip horizontally
     * @param v_flip Whether to flip vertically
     * @return A new tile with the specified flips applied
     */
    [[nodiscard]] Tile flip(bool h_flip, bool v_flip) const
    {
        Tile flipped_tile{};
        for (std::size_t row = 0; row < tile_side_length; ++row) {
            for (std::size_t col = 0; col < tile_side_length; ++col) {
                const std::size_t src_row = v_flip ? (tile_side_length - 1 - row) : row;
                const std::size_t src_col = h_flip ? (tile_side_length - 1 - col) : col;
                flipped_tile.set(row, col, at(src_row, src_col));
            }
        }
        return flipped_tile;
    }

  protected:
    [[nodiscard]] const std::array<PixelType, tile_size> &pix() const
    {
        return pix_;
    }

  private:
    std::array<PixelType, tile_size> pix_;
};

} // namespace porytiles2
