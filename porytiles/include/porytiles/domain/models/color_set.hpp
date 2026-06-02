#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "porytiles/domain/models/color_index.hpp"
#include "porytiles/domain/models/palette.hpp"

namespace porytiles {

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

    [[nodiscard]] bool test(ColorIndex index) const;

    void set(ColorIndex index, bool value = true);

    void reset(ColorIndex index);

    [[nodiscard]] const std::bitset<num_colors> &colors() const
    {
        return colors_;
    }

    [[nodiscard]] bool operator==(const ColorSet &other) const = default;

  private:
    std::bitset<num_colors> colors_;
};

/**
 * @brief Computes the union of two ColorSets.
 *
 * @details
 * Returns a new ColorSet containing all colors present in either set.
 *
 * @param a The first ColorSet
 * @param b The second ColorSet
 * @return A ColorSet containing the union of both sets
 */
[[nodiscard]] ColorSet color_set_union(const ColorSet &a, const ColorSet &b);

/**
 * @brief Computes the intersection of two ColorSets.
 *
 * @details
 * Returns a new ColorSet containing only colors present in both sets.
 *
 * @param a The first ColorSet
 * @param b The second ColorSet
 * @return A ColorSet containing the intersection of both sets
 */
[[nodiscard]] ColorSet color_set_intersection(const ColorSet &a, const ColorSet &b);

/**
 * @brief Counts the number of colors in a ColorSet.
 *
 * @details
 * Returns the number of bits set to true in the ColorSet.
 *
 * @param set The ColorSet to count
 * @return The number of colors in the set
 */
[[nodiscard]] std::size_t color_set_count(const ColorSet &set);

/**
 * @brief Checks if one ColorSet is a subset of another.
 *
 * @details
 * Returns true if every color in set 'a' is also present in set 'b'. An empty set is a subset of any set.
 *
 * @param a The potential subset
 * @param b The potential superset
 * @return true if a is a subset of b, false otherwise
 */
[[nodiscard]] bool is_subset(const ColorSet &a, const ColorSet &b);

/**
 * @brief Computes the intersection size between two ColorSets.
 *
 * @details
 * Returns the count of colors present in both sets.
 *
 * @param a The first ColorSet
 * @param b The second ColorSet
 * @return The number of colors in the intersection
 */
[[nodiscard]] std::size_t intersection_size(const ColorSet &a, const ColorSet &b);

/**
 * @brief Computes the union size of two ColorSets.
 *
 * @details
 * Returns the count of colors present in either set.
 *
 * @param a The first ColorSet
 * @param b The second ColorSet
 * @return The number of colors in the union
 */
[[nodiscard]] std::size_t union_size(const ColorSet &a, const ColorSet &b);

/**
 * @brief Iterates over each color index in a ColorSet.
 *
 * @details
 * Calls the provided function for each color index that is set in the ColorSet. The function is called with a
 * std::size_t representing the color index.
 *
 * This implementation uses efficient bit scanning to skip over zero bits, reducing iteration from O(256) to O(k)
 * where k is the number of set bits (typically 5-15 for tiles).
 *
 * @tparam Func A callable type accepting std::size_t
 * @param set The ColorSet to iterate over
 * @param func The function to call for each set color index
 */
template <typename Func>
void for_each_color(const ColorSet &set, Func &&func)
{
    static_assert(num_colors % 64 == 0, "num_colors must be a multiple of 64 for efficient word-aligned bit scanning");

    const auto &bits = set.colors();

    /*
     * Process 64 bits at a time using efficient bit scanning:
     *
     *     num_colors = 256, so we have 4 64-bit words
     *
     * Note: The +63 ceiling division is a defensive pattern; not strictly necessary due to the static_assert above.
     */
    constexpr std::size_t words = (num_colors + 63) / 64;

    for (std::size_t word = 0; word < words; ++word) {
        // Extract 64-bit chunk from bitset
        std::uint64_t chunk = 0;
        const std::size_t base = word * 64;
        for (std::size_t b = 0; b < 64 && (base + b) < num_colors; ++b) {
            if (bits.test(base + b)) {
                chunk |= (1ULL << b);
            }
        }

        // Process only set bits - skip zeros efficiently using Brian Kernighan's technique
        while (chunk != 0) {
            // GCC/Clang builtin to find lowest set bit position (count trailing zeros)
            const int bit = __builtin_ctzll(chunk);
            func(base + static_cast<std::size_t>(bit));
            chunk &= chunk - 1; // Clear lowest set bit
        }
    }
}

} // namespace porytiles

/**
 * @brief std::hash specialization for ColorSet.
 *
 * @details
 * Provides a hash function for ColorSet objects to enable their use in standard hash-based containers like
 * std::unordered_set and std::unordered_map. The hash is computed by converting the bitset to a string representation.
 */
template <>
struct std::hash<porytiles::ColorSet> {
    /**
     * @brief Computes the hash value for a ColorSet.
     *
     * @details
     * Converts the internal bitset to a string and hashes the resulting string.
     *
     * @param color_set The ColorSet to hash
     * @return The hash value
     */
    [[nodiscard]] std::size_t operator()(const porytiles::ColorSet &color_set) const noexcept
    {
        return std::hash<std::string>{}(color_set.colors().to_string());
    }
};
