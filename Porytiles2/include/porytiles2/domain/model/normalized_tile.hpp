#pragma once

#include <iterator>

#include "fmt/format.h"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/normalized_pal.hpp"
#include "porytiles2/domain/model/tile.hpp"
#include "porytiles2/templates/panic.hpp"

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

    /**
     * @brief Returns a const reference to the normalized palette.
     *
     * @return A const reference to the tile's palette
     */
    [[nodiscard]] const NormalizedPal<ColorType> &palette() const
    {
        return pal_;
    }

    /**
     * @brief Returns a mutable reference to the normalized palette.
     *
     * @return A mutable reference to the tile's palette
     */
    [[nodiscard]] NormalizedPal<ColorType> &palette()
    {
        return pal_;
    }

    /**
     * @brief Returns the resolved color at the specified linear index.
     *
     * @details
     * Resolves the IndexPixel at the given position to the actual color from the palette. IndexPixel value 0 maps to
     * the extrinsic transparency color, while values 1+ map to palette colors with a 1-based indexing (IndexPixel 1 =
     * first palette color).
     *
     * @param i The linear index (0-63 for an 8x8 tile)
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &color_at(std::size_t i) const
    {
        return resolve_index_pixel(at(i));
    }

    /**
     * @brief Returns the resolved color at the specified row and column.
     *
     * @details
     * Resolves the IndexPixel at the given position to the actual color from the palette. IndexPixel value 0 maps to
     * the extrinsic transparency color, while values 1+ map to palette colors with a 1-based indexing (IndexPixel 1 =
     * first palette color).
     *
     * @param row The row index (0-7)
     * @param col The column index (0-7)
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &color_at(std::size_t row, std::size_t col) const
    {
        return resolve_index_pixel(at(row, col));
    }

    // TODO: I don't think we need these operators?
    // /**
    //  * @brief Compares two NormalizedTile objects for ordering.
    //  *
    //  * @details
    //  * Comparison is performed first on the tile pixel data (via base class), then on the flip states, then on the
    //  * palette. This ensures that the lexicographically smallest tile representation is chosen as the normal form.
    //  *
    //  * @param other The other NormalizedTile to compare against
    //  * @return Ordering relationship between this tile and other
    //  */
    // auto operator<=>(const NormalizedTile &other) const
    // {
    //     // First compare the tile pixel data
    //     if (auto base_cmp = static_cast<const Tile &>(*this) <=> static_cast<const Tile &>(other); base_cmp != 0) {
    //         return base_cmp;
    //     }
    //
    //     // Then compare flip states
    //     if (auto flip_cmp = std::tie(h_flip_, v_flip_) <=> std::tie(other.h_flip_, other.v_flip_); flip_cmp != 0) {
    //         return flip_cmp;
    //     }
    //
    //     // Finally compare palettes
    //     return pal_.colors() <=> other.pal_.colors();
    // }
    //
    // bool operator==(const NormalizedTile &other) const = default;

  private:
    /**
     * @brief Resolves an IndexPixel to its corresponding color in the palette.
     *
     * @details
     * IndexPixel value 0 maps to the extrinsic transparency color, while values 1+ map to palette colors with 1-based
     * indexing (IndexPixel 1 = first palette color).
     *
     * @param pixel The IndexPixel to resolve
     * @return A const reference to the resolved color
     */
    [[nodiscard]] const ColorType &resolve_index_pixel(const IndexPixel &pixel) const
    {
        const auto index = pixel.index();

        if (index == 0) {
            return pal_.extrinsic_transparency();
        }

        const auto palette_index = index - 1;
        if (palette_index >= pal_.size()) {
            panic(fmt::format("IndexPixel value {} out of bounds for palette of size {}", index, pal_.size()));
        }

        auto it = std::next(pal_.colors().begin(), palette_index);
        return *it;
    }

    bool h_flip_;
    bool v_flip_;
    NormalizedPal<ColorType> pal_;
};

} // namespace porytiles2
