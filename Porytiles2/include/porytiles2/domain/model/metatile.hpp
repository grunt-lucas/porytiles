#pragma once

#include <array>

#include "porytiles2/domain/model/supports_transparency.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

/**
 * @brief The core tileset entity - a 2x2 grid of Tile objects arranged into three layers.
 *
 * @details
 * Like its component Tile objects, the pixel type of Metatile is arbitrary.
 *
 * @tparam PixelType The pixel type of this Metatile's Tile objects
 */
template <typename PixelType>
    requires SupportsTransparency<PixelType>
class Metatile {
  public:
    static constexpr std::size_t tiles_per_side = 2;
    static constexpr std::size_t tiles_per_metatile = tiles_per_side * tiles_per_side;
    static constexpr std::size_t metatile_side_length = tiles_per_side * tile_side_length;

    Metatile() : id_{} {}

    bool operator==(const Metatile &) const = default;

    /**
     * @brief Checks if this entire metatile is transparent.
     *
     * @details
     * A metatile is transparent if all of its pixels are either intrinsically transparent or are extrinsically
     * transparent, according to the provided extrinsic transparency value.
     *
     * @param extrinsics The extrinsic transparency value to check each pixel against
     * @return True if all tiles in all layers are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent(const PixelType &extrinsic) const
    {
        const bool bottom_transparent =
            std::ranges::all_of(bottom(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        const bool middle_transparent =
            std::ranges::all_of(middle(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        const bool top_transparent =
            std::ranges::all_of(top(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        return bottom_transparent && middle_transparent && top_transparent;
    }

    /**
     * @brief Get a constant reference to a Tile from the bottom layer.
     *
     * @details
     * Retrieves the Tile at the specified index in the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @return Constant reference to the Tile at the specified index.
     */
    [[nodiscard]] const Tile<PixelType> &bottom(std::size_t i) const
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        return bottom_[i];
    }

    /**
     * @brief Get a constant reference to the entire bottom layer array.
     *
     * @details
     * Returns the complete array of tiles in the bottom layer, allowing for range-based iteration.
     *
     * @return Constant reference to the bottom layer tile array.
     */
    [[nodiscard]] const std::array<Tile<PixelType>, tiles_per_metatile> &bottom() const
    {
        return bottom_;
    }

    /**
     * @brief Set a Tile in the bottom layer.
     *
     * @details
     * Moves the provided Tile into the specified index of the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @param tile The Tile to move into the array.
     */
    void set_bottom(std::size_t i, Tile<PixelType> tile)
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        bottom_[i] = std::move(tile);
    }

    /**
     * @brief Get a constant reference to a Tile from the middle layer.
     *
     * @details
     * Retrieves the Tile at the specified index in the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @return Constant reference to the Tile at the specified index.
     */
    [[nodiscard]] const Tile<PixelType> &middle(std::size_t i) const
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        return middle_[i];
    }

    /**
     * @brief Get a constant reference to the entire middle layer array.
     *
     * @details
     * Returns the complete array of tiles in the middle layer, allowing for range-based iteration.
     *
     * @return Constant reference to the middle layer tile array.
     */
    [[nodiscard]] const std::array<Tile<PixelType>, tiles_per_metatile> &middle() const
    {
        return middle_;
    }

    /**
     * @brief Set a Tile in the middle layer.
     *
     * @details
     * Moves the provided Tile into the specified index of the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @param tile The Tile to move into the array.
     */
    void set_middle(std::size_t i, Tile<PixelType> tile)
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        middle_[i] = std::move(tile);
    }

    /**
     * @brief Get a constant reference to a Tile from the top layer.
     *
     * @details
     * Retrieves the Tile at the specified index in the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @return Constant reference to the Tile at the specified index.
     */
    [[nodiscard]] const Tile<PixelType> &top(std::size_t i) const
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        return top_[i];
    }

    /**
     * @brief Get a constant reference to the entire top layer array.
     *
     * @details
     * Returns the complete array of tiles in the top layer, allowing for range-based iteration.
     *
     * @return Constant reference to the top layer tile array.
     */
    [[nodiscard]] const std::array<Tile<PixelType>, tiles_per_metatile> &top() const
    {
        return top_;
    }

    /**
     * @brief Set a Tile in the top layer.
     *
     * @details
     * Moves the provided Tile into the specified index of the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @param tile The Tile to move into the array.
     */
    void set_top(std::size_t i, Tile<PixelType> tile)
    {
        if (i > 3) {
            panic(fmt::format("index {} out of bounds: must be [0,3]", i));
        }
        top_[i] = std::move(tile);
    }

  private:
    std::array<Tile<PixelType>, tiles_per_metatile> bottom_;
    std::array<Tile<PixelType>, tiles_per_metatile> middle_;
    std::array<Tile<PixelType>, tiles_per_metatile> top_;
    unsigned int id_;
};

} // namespace porytiles2