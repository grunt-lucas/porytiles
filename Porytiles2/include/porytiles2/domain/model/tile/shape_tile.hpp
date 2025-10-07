#pragma once

#include <algorithm>
#include <map>
#include <ranges>

#include "porytiles2/domain/model/tile/shape_mask.hpp"

namespace porytiles2 {

/**
 * @brief An 8x8 tile backed by mask-based storage that maps shape regions to pixel values.
 *
 * @details
 * ShapeTile represents tiles as a collection of shape masks (ShapeMask) mapped to pixel values (PixelType). Unlike
 * PixelTile which stores literal per-pixel data, ShapeTile stores tiles as a set of shape regions where each region
 * is defined by a ShapeMask (which pixels are "on" in that region) and maps to a single pixel value.
 *
 * This representation enables:
 * - Shape-based tile comparison that separates geometry from color assignments
 * - Canonical orientation finding through lexicographic shape comparison
 * - Color-agnostic tile deduplication and analysis
 *
 * Multiple ShapeMask instances can be combined to define different color regions within a single tile. Each mask
 * identifies which pixels belong to a particular region, and the associated PixelType provides the color or data
 * for those pixels.
 *
 * Key design features:
 * - operator< compares ONLY shape masks (keys), ignoring pixel values - this is critical for canonical orientation
 *   finding
 * - operator== compares both shape masks AND pixel values for full equality
 * - Flipping operations transform all masks while preserving color mappings
 *
 * @tparam PixelType The pixel type stored for each shape region
 */
template <typename PixelType>
class ShapeTile {
  public:
    virtual ~ShapeTile() = default;

    ShapeTile() = default;

    bool operator==(const ShapeTile &other) const = default;

    /**
     * @brief Compares this ShapeTile with another based ONLY on shape masks, ignoring pixel values.
     *
     * @details
     * This operator performs a lexicographic comparison of the shape masks (keys) only, completely ignoring the
     * associated pixel values. This is the critical design feature that enables canonical orientation finding - tiles
     * with identical shapes but different colors will compare based solely on their geometric structure.
     *
     * This comparison is used to find the "minimal" or "canonical" orientation of a tile among its flipped variants,
     * ensuring that tiles with the same shape structure but different color assignments can be identified as having
     * equivalent geometry.
     *
     * Contrast with operator==, which compares both shape masks AND pixel values for full equality.
     *
     * @param other The ShapeTile to compare against
     * @return True if this tile's shape masks are lexicographically less than other's shape masks
     */
    bool operator<(const ShapeTile &other) const
    {
        auto keys1 = colors_ | std::views::keys;
        auto keys2 = other.colors_ | std::views::keys;
        return std::ranges::lexicographical_compare(keys1, keys2);
    }

    /**
     * @brief Checks if this entire ShapeTile is transparent.
     *
     * @details
     * A ShapeTile is transparent if all of its shape masks are transparent (i.e., every ShapeMask has all bits
     * unset). This check only examines the shape masks, not the pixel values associated with them.
     *
     * @return True if all shape masks in the tile are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent() const
    {
        auto keys = colors_ | std::views::keys;
        return std::ranges::all_of(keys, &ShapeMask::is_transparent);
    }

    /**
     * @brief Creates a flipped version of this ShapeTile.
     *
     * @details
     * Returns a new ShapeTile with all shape masks flipped according to the specified parameters, while preserving
     * the pixel value mappings. Horizontal flip reflects the tile across a vertical axis, vertical flip reflects
     * across a horizontal axis.
     *
     * Each ShapeMask in the tile is flipped individually using its flip() method, and the resulting flipped mask is
     * mapped to the same pixel value as the original mask. This preserves the color assignments while transforming
     * the tile's geometric structure.
     *
     * If neither h nor v is true, returns a copy of the original tile.
     *
     * @param h True to flip horizontally
     * @param v True to flip vertically
     * @return A new ShapeTile with the specified flips applied
     */
    [[nodiscard]] ShapeTile flip(bool h, bool v) const
    {
        if (!h && !v) {
            return *this;
        }

        ShapeTile result;
        for (const auto &[mask, color] : colors_) {
            result.colors_[mask.flip(h, v)] = color;
        }
        return result;
    }

    /**
     * @brief Returns the internal map of shape masks to pixel values.
     *
     * @details
     * Provides read-only access to the underlying std::map that stores the shape-to-pixel mappings. Each entry in the
     * map represents a shape region (ShapeMask) and its associated pixel value (PixelType).
     *
     * @return A const reference to the colors map
     */
    [[nodiscard]] const std::map<ShapeMask, PixelType> &colors() const
    {
        return colors_;
    }

    /**
     * @brief Sets or updates the pixel value for a specific shape mask.
     *
     * @details
     * Associates the given pixel value with the specified shape mask. If the mask already exists in the tile, its
     * pixel value is updated. If the mask does not exist, a new entry is created.
     *
     * @param mask The shape mask identifying which pixels belong to this region
     * @param color The pixel value to associate with this shape mask
     */
    void set(const ShapeMask &mask, const PixelType &color)
    {
        colors_[mask] = color;
    }

  private:
    std::map<ShapeMask, PixelType> colors_;
};

} // namespace porytiles2