#pragma once

#include <algorithm>
#include <array>

#include "../panic/panic.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

constexpr std::size_t kTileSideLength = 8;
constexpr std::size_t kTileSize = kTileSideLength * kTileSideLength;

/// @brief A single 8x8 pixel tile with an arbitrary pixel data type.
template <typename P>
class Tile {
    std::array<P, kTileSize> pix_;

  protected:
    [[nodiscard]] const std::array<P, kTileSize> &pix() const {
        return pix_;
    }

  public:
    virtual ~Tile() = default;

    explicit Tile() : pix_{} {}

    [[nodiscard]] virtual bool IsTransparent(const P &transparency) const {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel == transparency; });
    }

    [[nodiscard]] P At(std::size_t i) const {
        if (i >= kTileSize) {
            Panic(fmt::format("Index {} out of bounds", i));
        }
        return pix_[i];
    }

    [[nodiscard]] P At(std::size_t row, std::size_t col) const {
        if (row >= kTileSideLength) {
            Panic(fmt::format("Row index {} out of bounds", row));
        }
        if (col >= kTileSideLength) {
            Panic(fmt::format("Col index {} out of bounds", col));
        }
        return pix_[row * kTileSideLength + col];
    }

    void Set(std::size_t i, const P &p) {
        if (i >= kTileSize) {
            Panic(fmt::format("Index {} out of bounds", i));
        }
        pix_[i] = p;
    }

    void Set(std::size_t row, std::size_t col, const P &p) {
        if (row >= kTileSideLength) {
            Panic(fmt::format("Row index {} out of bounds", row));
        }
        if (col >= kTileSideLength) {
            Panic(fmt::format("Col index {} out of bounds", col));
        }
        pix_[row * kTileSideLength + col] = p;
    }
};

} // namespace porytiles
