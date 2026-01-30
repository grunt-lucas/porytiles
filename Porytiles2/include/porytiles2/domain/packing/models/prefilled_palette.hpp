#pragma once

#include <compare>
#include <cstddef>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/palette.hpp"

namespace porytiles2 {

/**
 * @brief Pre-assigned palette with wildcard support for palette packing.
 *
 * @details
 * PrefilledPalette represents a hardware palette that has some or all of its slots pre-assigned. This is used for:
 *
 * - **Primary palette reuse**: Fixed colors from primary tileset; secondary tiles
 *   can use the palette if their colors are subsets
 * - **Partial override**: User locks some colors (e.g., DNS window colors) but
 *   allows the packer to fill remaining slots
 * - **Fully locked**: Override specifies entire palette, no room for additional colors
 *
 * Use the static factory methods to construct instances.
 */
class PrefilledPalette {
  public:
    /**
     * @brief Creates a fully locked PrefilledPalette.
     *
     * @details
     * A fully locked palette has all its color slots occupied. The packer cannot add any new colors to this palette.
     * Tiles can only use this palette if their colors are already a subset of the fixed colors.
     *
     * @param hardware_index The hardware palette index
     * @param color_set The fixed colors occupying all slots
     * @param occupied_slots The number of physical slots occupied in the palette (may differ from unique color count
     *        if palette contains duplicate colors)
     * @return A fully locked PrefilledPalette
     */
    [[nodiscard]] static PrefilledPalette
    fully_locked(std::size_t hardware_index, ColorSet color_set, std::size_t occupied_slots);

    /**
     * @brief Creates a partially locked palette with available capacity.
     *
     * @details
     * A partially locked palette has some colors fixed but still has room for the packer to add more colors. The
     * available capacity is computed as total_capacity - occupied_slots.
     *
     * @param hardware_index The hardware palette index
     * @param fixed_colors The colors that are already locked in this palette
     * @param occupied_slots The number of physical slots occupied in the palette (may differ from unique color count
     *        if palette contains duplicate colors)
     * @param total_capacity The total number of color slots (default pal::max_size - 1)
     * @return A partially locked PrefilledPalette
     * @pre occupied_slots <= total_capacity
     */
    [[nodiscard]] static PrefilledPalette partially_locked(
        std::size_t hardware_index,
        ColorSet fixed_colors,
        std::size_t occupied_slots,
        std::size_t total_capacity = pal::max_size - 1);

    [[nodiscard]] bool operator==(const PrefilledPalette &other) const
    {
        return hardware_index_ == other.hardware_index_;
    }

    [[nodiscard]] std::strong_ordering operator<=>(const PrefilledPalette &other) const
    {
        return hardware_index_ <=> other.hardware_index_;
    }

    /**
     * @brief Gets the palette's hardware index.
     *
     * @return The hardware index
     */
    [[nodiscard]] std::size_t hardware_index() const
    {
        return hardware_index_;
    }

    /**
     * @brief Gets the fixed colors in this palette.
     *
     * @return A const reference to the fixed ColorSet
     */
    [[nodiscard]] const ColorSet &fixed_colors() const
    {
        return fixed_colors_;
    }

    /**
     * @brief Gets the number of fixed colors.
     *
     * @return The count of fixed colors
     */
    [[nodiscard]] std::size_t fixed_color_count() const;

    /**
     * @brief Gets the number of occupied slots in the original prefilled palette.
     *
     * @details
     * This may differ from fixed_color_count() if the prefilled palette contains duplicate colors. The occupied_slots
     * value reflects how many physical slots are taken in the hardware palette.
     *
     * @return The number of occupied slots
     */
    [[nodiscard]] std::size_t occupied_slots() const
    {
        return occupied_slots_;
    }

    /**
     * @brief Gets the number of slots available for the packer to fill.
     *
     * @return The available capacity (0 if fully locked)
     */
    [[nodiscard]] std::size_t available_capacity() const
    {
        return available_capacity_;
    }

    /**
     * @brief Checks if this palette is fully locked.
     *
     * @return true if no slots are available for packing, false otherwise
     */
    [[nodiscard]] bool is_fully_locked() const
    {
        return available_capacity_ == 0;
    }

    /**
     * @brief Checks if a tile's colors can be accommodated by this palette.
     *
     * @details
     * A tile can be accommodated if:
     * - Its colors are a subset of the fixed colors (always works), OR
     * - The new colors (colors not already fixed) fit in the available capacity
     *
     * @param tile_colors The tile's colors to check
     * @return true if the tile can use this palette, false otherwise
     */
    [[nodiscard]] bool can_accommodate(const ColorSet &tile_colors) const;

  private:
    PrefilledPalette(
        std::size_t hardware_index, ColorSet fixed_colors, std::size_t available_capacity, std::size_t occupied_slots);

    std::size_t hardware_index_;
    ColorSet fixed_colors_;
    std::size_t available_capacity_;
    std::size_t occupied_slots_;
};

} // namespace porytiles2
