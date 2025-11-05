#pragma once

#include <array>
#include <cstdint>

#include "porytiles2/domain/models/pixel_tile.hpp"

namespace porytiles2 {

/**
 * @brief Represents which pixels in an 8x8 tile are non-transparent.
 *
 * @details
 * ShapeMask uses 1 bit per pixel to track the "shape" of 8x8 tile. Each of the 8 rows is stored as a uint8_t, where the
 * most significant bit represents the leftmost pixel (column 0) and the least significant bit represents the rightmost
 * pixel (column 7).
 *
 * The class provides flipping operations and bitwise manipulation to set or unset individual pixels. Comparison
 * operators are lexicographic on the rows array.
 *
 * In the tile canonicalization system, ShapeMask serves as a shape-based tile representation that separates tile
 * geometry from color assignments. It is the fundamental building block for representing tile shapes. Multiple
 * ShapeMask instances can be combined to define different color regions within a single tile, with each mask mapping
 * to a specific color index. The flipping operations and lexicographic comparison enable the compiler to find canonical
 * (minimal) orientations for tiles in a color-agnostic way, allowing deduplication of tiles that are identical under
 * flips or color transformations.
 *
 * @invariant Default-constructed ShapeMask is transparent (satisfies SupportsTransparency design invariant). That is,
 * `ShapeMask{}` produces a fully transparent mask with all bits unset (all rows are 0).
 */
class ShapeMask {
  public:
    ShapeMask() = default;

    /**
     * @brief Constructs a ShapeMask from an array of row data.
     *
     * @details
     * Each element in the array represents one row of the 8x8 tile, where bit 7 is the leftmost pixel and bit 0 is the
     * rightmost pixel.
     *
     * @param rows An array of 8 bytes representing the tile rows
     */
    explicit ShapeMask(const std::array<uint8_t, tile::side_length_pix> &rows) : rows_{rows} {}

    // (lexicographic on rows array)
    auto operator<=>(const ShapeMask &) const = default;

    /**
     * @brief Returns a flipped version of this mask.
     *
     * @details
     * Creates a new ShapeMask that is horizontally and/or vertically flipped. Horizontal flipping reverses the bits in
     * each row. Vertical flipping reverses the order of the rows. If neither h nor v is true, returns a copy of the
     * original mask.
     *
     * @param h True to flip horizontally
     * @param v True to flip vertically
     * @return The flipped mask
     */
    [[nodiscard]] ShapeMask flip(bool h, bool v) const;

    /**
     * @brief Sets the bit at the specified row and column to 1.
     *
     * @details
     * Marks the pixel at the given position as non-transparent. Column 0 is the leftmost pixel (bit 7) and column 7 is
     * the rightmost pixel (bit 0).
     *
     * @param row The row index (0-7)
     * @param col The column index (0-7)
     */
    void set(int row, int col);

    /**
     * @brief Sets the bit at the specified row and column to 0.
     *
     * @details
     * Marks the pixel at the given position as transparent. Column 0 is the leftmost pixel (bit 7) and column 7 is the
     * rightmost pixel (bit 0).
     *
     * @param row The row index (0-7)
     * @param col The column index (0-7)
     */
    void unset(int row, int col);

    /**
     * @brief Gets the bit value at the specified row and column.
     *
     * @details
     * Returns true if the pixel at the given position is marked as non-transparent (bit is 1), false otherwise. Column
     * 0 is the leftmost pixel (bit 7) and column 7 is the rightmost pixel (bit 0).
     *
     * @param row The row index (0-7)
     * @param col The column index (0-7)
     * @return True if the bit is set, false otherwise
     */
    [[nodiscard]] bool get(int row, int col) const;

    /**
     * @brief Checks if this entire ShapeMask is transparent.
     *
     * @details
     * A ShapeMask is transparent if every bit in every mask integer is unset.
     *
     * @return True if all bits in the mask are unset, false otherwise
     */
    [[nodiscard]] bool is_transparent() const;

    /**
     * @brief Returns the raw row data.
     *
     * @details
     * Returns a const reference to the internal array of 8 bytes representing the tile rows.
     *
     * @return A const reference to the rows array
     */
    [[nodiscard]] const std::array<uint8_t, 8> &rows() const
    {
        return rows_;
    }

  private:
    std::array<uint8_t, tile::side_length_pix> rows_{};
};

} // namespace porytiles2

/**
 * @brief std::hash specialization for ShapeMask.
 *
 * @details
 * Provides a hash function for ShapeMask objects to enable their use in standard hash-based containers like
 * std::unordered_set and std::unordered_map. The hash is computed by combining each byte of the mask.
 */
template <>
struct std::hash<porytiles2::ShapeMask> {
    size_t operator()(const porytiles2::ShapeMask &tm) const noexcept
    {
        // Simple hash combining all 8 bytes
        size_t h = 0;
        for (const uint8_t byte : tm.rows()) {
            h = h * 31 + byte;
        }
        return h;
    }
};
