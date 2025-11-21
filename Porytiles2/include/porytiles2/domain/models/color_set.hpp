#pragma once

#include <bitset>
#include <cstddef>
#include <functional>
#include <string>

#include "porytiles2/domain/models/color_index.hpp"
#include "porytiles2/domain/models/palette.hpp"

namespace porytiles2 {

/**
 * @brief Maximum allowable color count for GBA hardware.
 */
inline constexpr std::size_t num_colors = pal::max_size * pal::num_pals;

/**
 * @brief A set of colors represented as a bitset.
 *
 * @details
 * ColorSet tracks which colors are present in a set using a fixed-size bitset. Each bit position corresponds to a
 * specific color index.
 */
class ColorSet {
  public:
    ColorSet() = default;

    /**
     * @brief Tests whether a bit at the given index is set.
     *
     * @details
     * Returns true if the bit at the specified index is set, false otherwise.
     *
     * @param index The index of the bit to test
     * @return true if the bit is set, false otherwise
     */
    [[nodiscard]] bool test(ColorIndex index) const;

    /**
     * @brief Sets a bit at the given index.
     *
     * @details
     * Sets the bit at the specified index to the given value (default true).
     *
     * @param index The index of the bit to set
     * @param value The value to set (true or false)
     */
    void set(ColorIndex index, bool value = true);

    /**
     * @brief Resets a bit at the given index to false.
     *
     * @details
     * Clears the bit at the specified index, setting it to false.
     *
     * @param index The index of the bit to reset
     */
    void reset(ColorIndex index);

    /**
     * @brief Gets the underlying bitset.
     *
     * @details
     * Returns a const reference to the internal bitset representation.
     *
     * @return A const reference to the bitset
     */
    [[nodiscard]] const std::bitset<num_colors> &colors() const
    {
        return colors_;
    }

    /**
     * @brief Compares two ColorSet objects for equality.
     *
     * @details
     * Returns true if both ColorSet objects have identical bitsets.
     *
     * @param other The ColorSet to compare with
     * @return true if equal, false otherwise
     */
    [[nodiscard]] bool operator==(const ColorSet &other) const = default;

  private:
    std::bitset<num_colors> colors_;
};

} // namespace porytiles2

/**
 * @brief std::hash specialization for ColorSet.
 *
 * @details
 * Provides a hash function for ColorSet objects to enable their use in standard hash-based containers like
 * std::unordered_set and std::unordered_map. The hash is computed by converting the bitset to a string representation.
 */
template <>
struct std::hash<porytiles2::ColorSet> {
    /**
     * @brief Computes the hash value for a ColorSet.
     *
     * @details
     * Converts the internal bitset to a string and hashes the resulting string.
     *
     * @param color_set The ColorSet to hash
     * @return The hash value
     */
    [[nodiscard]] std::size_t operator()(const porytiles2::ColorSet &color_set) const noexcept
    {
        return std::hash<std::string>{}(color_set.colors().to_string());
    }
};
