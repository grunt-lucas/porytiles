#pragma once

#include <algorithm>
#include <array>

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

    /**
     * @brief Checks if this entire tile is transparent.
     *
     * @details
     * A tile is considered transparent if all of its pixels are either intrinsically transparent or are extrinsically
     * transparent, according to the provided extrinsic transparency value.
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return True if all pixels in the tile are transparent, false otherwise
     */
    [[nodiscard]] virtual bool is_transparent(const PixelType &extrinsic) const {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel.is_transparent(extrinsic); });
    }

    [[nodiscard]] PixelType at(std::size_t i) const {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        return pix_[i];
    }

    [[nodiscard]] PixelType at(std::size_t row, std::size_t col) const {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        return pix_[row * tile_side_length + col];
    }

    void set(std::size_t i, const PixelType &p) {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        pix_[i] = p;
    }

    void set(std::size_t row, std::size_t col, const PixelType &p) {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        pix_[row * tile_side_length + col] = p;
    }

  protected:
    [[nodiscard]] const std::array<PixelType, tile_size> &pix() const {
        return pix_;
    }

  private:
    std::array<PixelType, tile_size> pix_;
};

} // namespace porytiles2
