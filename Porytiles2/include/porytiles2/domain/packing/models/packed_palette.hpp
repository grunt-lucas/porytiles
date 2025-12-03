#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace porytiles2 {

/**
 * @brief Represents a hardware palette after packing with accumulated colors and assigned tiles.
 *
 * @details
 * PackedPalette tracks the colors that have been assigned to a single hardware palette during the packing process. It
 * maintains both the accumulated color set and the list of tile IDs that have been assigned to use this palette.
 *
 * GBA hardware palettes have 16 slots, but slot 0 is always transparency, leaving 15 usable color slots (the default
 * capacity).
 */
class PackedPalette {
  public:
    /**
     * @brief Constructs a PackedPalette with a given index and capacity.
     *
     * @param hardware_index The hardware palette index (0-5 for primary, etc.)
     * @param capacity Maximum number of colors this palette can hold (default pal::max_size - 1, i.e. 15)
     */
    explicit PackedPalette(std::size_t hardware_index, std::size_t capacity = pal::max_size - 1);

    /**
     * @brief Gets the palette's hardware index.
     *
     * @return The hardware palette index
     */
    [[nodiscard]] std::size_t hw_index() const
    {
        return hw_index_;
    }

    /**
     * @brief Gets the accumulated color set in this palette.
     *
     * @return A const reference to the ColorSet
     */
    [[nodiscard]] const ColorSet &color_set() const
    {
        return color_set_;
    }

    /**
     * @brief Gets the list of tile IDs assigned to this palette.
     *
     * @return A const reference to the vector of tile IDs (PackableTile::Id variants)
     */
    [[nodiscard]] const std::vector<PackableTile::Id> &assigned_tile_ids() const
    {
        return assigned_tile_ids_;
    }

    /**
     * @brief Gets the number of colors currently in this palette.
     *
     * @return The number of colors
     */
    [[nodiscard]] std::size_t color_count() const;

    /**
     * @brief Gets the number of remaining color slots.
     *
     * @return The remaining capacity
     */
    [[nodiscard]] std::size_t remaining_capacity() const;

    /**
     * @brief Checks if a tile's colors can fit in this palette.
     *
     * @details
     * Returns true if adding the tile's colors would not exceed the palette's capacity. Takes into account colors that
     * are already in the palette (overlap).
     *
     * @param tile_colors The colors to check
     * @return true if the colors can fit, false otherwise
     */
    [[nodiscard]] bool can_fit(const ColorSet &tile_colors) const;

    /**
     * @brief Computes the size of the union of this palette's colors with the given colors.
     *
     * @details
     * Returns the total number of unique colors that would result from adding the given colors to this palette.
     *
     * @param tile_colors The colors to compute union with
     * @return The number of colors in the union
     */
    [[nodiscard]] std::size_t union_size(const ColorSet &tile_colors) const;

    /**
     * @brief Adds a tile to this palette.
     *
     * @details
     * Adds the tile's ID to the assigned list and unions the tile's colors with this palette's color set.
     *
     * @param tile The tile to add
     * @pre can_fit(tile.color_set()) must be true
     */
    void add_tile(const PackableTile &tile);

    /**
     * @brief Removes a tile from this palette and recalculates the color set.
     *
     * @details
     * Removes the tile's ID from the assigned list and rebuilds the color set from the remaining tiles. Colors that
     * were unique to the removed tile will no longer be in the palette's color set.
     *
     * @param tile The tile to remove
     */
    void remove_tile(const PackableTile &tile);

  private:
    std::size_t hw_index_;
    std::size_t capacity_;
    ColorSet color_set_;
    std::vector<PackableTile::Id> assigned_tile_ids_;
    std::unordered_map<PackableTile::Id, ColorSet> tile_colors_;
};

} // namespace porytiles2
