#pragma once

#include <algorithm>
#include <array>

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

/**
 * @brief A single 8x8 pixel tile with an arbitrary pixel data type.
 *
 * @tparam P The pixel type for this Tile
 */
template <typename P>
class Tile {
  public:
    static constexpr std::size_t tile_side_length = 8;
    static constexpr std::size_t tile_size = tile_side_length * tile_side_length;

    virtual ~Tile() = default;

    explicit Tile() : pix_{} {}

    [[nodiscard]] virtual bool is_transparent(const P &transparency) const {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel == transparency; });
    }

    [[nodiscard]] P at(std::size_t i) const {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        return pix_[i];
    }

    [[nodiscard]] P at(std::size_t row, std::size_t col) const {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        return pix_[row * tile_side_length + col];
    }

    void set(std::size_t i, const P &p) {
        if (i >= tile_size) {
            panic(fmt::format("index {} out of bounds", i));
        }
        pix_[i] = p;
    }

    void set(std::size_t row, std::size_t col, const P &p) {
        if (row >= tile_side_length) {
            panic(fmt::format("row index {} out of bounds", row));
        }
        if (col >= tile_side_length) {
            panic(fmt::format("col index {} out of bounds", col));
        }
        pix_[row * tile_side_length + col] = p;
    }

  protected:
    [[nodiscard]] const std::array<P, tile_size> &pix() const {
        return pix_;
    }

  private:
    std::array<P, tile_size> pix_;
};

} // namespace porytiles2
