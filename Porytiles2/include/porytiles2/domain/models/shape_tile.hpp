#pragma once

#include <algorithm>
#include <map>
#include <ranges>

#include "porytiles2/domain/models/color_index.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

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
 * - operator== and operator<=> compare both shape masks AND pixel values for full equality and ordering
 * - compare_shape_only() provides specialized shape-only comparison, ignoring pixel values - this is critical for
 *   canonical orientation finding
 * - Flipping operations transform all masks while preserving color mappings
 *
 * @invariant Default-constructed ShapeTile is transparent (satisfies SupportsTransparency design invariant). That is,
 * `ShapeTile{}` produces a transparent tile with an empty colors_ map, and an empty map is considered fully transparent
 * by is_transparent().
 *
 * @tparam PixelType The pixel type stored for each shape region
 */
template <typename PixelType>
class ShapeTile {
  public:
    virtual ~ShapeTile() = default;

    ShapeTile() = default;

    /**
     * @brief Converts a PixelTile to a ShapeTile<unsigned int> using a ColorIndexMap (intrinsic transparency).
     *
     * @details
     * This static method creates a ShapeTile<unsigned int> from a PixelTile by mapping each unique non-transparent
     * color to its corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed
     * that marks all pixel positions containing that color.
     *
     * This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
     *
     * The conversion process:
     * 1. Iterates through all 64 pixels in the PixelTile
     * 2. Skips intrinsically transparent pixels
     * 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
     * 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
     * 5. Builds ShapeMasks for each unique color index
     * 6. Returns a ShapeTile<unsigned int> mapping ShapeMasks to color indices
     *
     * @tparam InputPixelType The pixel type of the input tile, must support intrinsic transparency
     * @param pixel_tile The PixelTile to convert
     * @param color_index_map The ColorIndexMap providing color-to-index mappings
     * @return A ShapeTile<unsigned int> with ShapeMasks mapped to color indices
     * @throws Panics if any non-transparent pixel in pixel_tile is not found in color_index_map
     */
    template <SupportsTransparency InputPixelType>
    [[nodiscard]] static ShapeTile<unsigned int>
    from_pixel_tile(const PixelTile<InputPixelType> &pixel_tile, const ColorIndexMap<InputPixelType> &color_index_map)
        requires requires(const InputPixelType &p) { p.is_transparent(); }
    {
        return from_pixel_tile_impl(
            pixel_tile, color_index_map, [](const InputPixelType &p) { return p.is_transparent(); });
    }

    /**
     * @brief Converts a PixelTile to a ShapeTile<unsigned int> using a ColorIndexMap (extrinsic transparency).
     *
     * @details
     * This static method creates a ShapeTile<unsigned int> from a PixelTile by mapping each unique non-transparent
     * color to its corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed
     * that marks all pixel positions containing that color.
     *
     * This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32). Transparency
     * is determined using both intrinsic (alpha=0) and extrinsic (matching the provided value) checks.
     *
     * The conversion process:
     * 1. Iterates through all 64 pixels in the PixelTile
     * 2. Skips pixels that are either intrinsically or extrinsically transparent
     * 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
     * 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
     * 5. Builds ShapeMasks for each unique color index
     * 6. Returns a ShapeTile<unsigned int> mapping ShapeMasks to color indices
     *
     * @tparam InputPixelType The pixel type of the input tile, must support extrinsic transparency
     * @param pixel_tile The PixelTile to convert
     * @param color_index_map The ColorIndexMap providing color-to-index mappings
     * @param extrinsic The extrinsic transparency value to check pixels against
     * @return A ShapeTile<unsigned int> with ShapeMasks mapped to color indices
     * @throws Panics if any non-transparent pixel in pixel_tile is not found in color_index_map
     */
    template <SupportsTransparency InputPixelType>
    [[nodiscard]] static ShapeTile<unsigned int> from_pixel_tile(
        const PixelTile<InputPixelType> &pixel_tile,
        const ColorIndexMap<InputPixelType> &color_index_map,
        const InputPixelType &extrinsic)
        requires requires(const InputPixelType &p) { p.is_transparent(p); }
    {
        return from_pixel_tile_impl(
            pixel_tile, color_index_map, [&extrinsic](const InputPixelType &p) { return p.is_transparent(extrinsic); });
    }

    auto operator<=>(const ShapeTile &other) const = default;

    /**
     * @brief Compares two ShapeTiles based ONLY on shape masks, ignoring pixel values.
     *
     * @details
     * This static method performs a lexicographic comparison of the shape masks (keys) only, completely ignoring the
     * associated pixel values. This is a specialized comparison used for canonical orientation finding - tiles with
     * identical shapes but different colors will compare based solely on their geometric structure.
     *
     * This comparison is used to find the "minimal" or "canonical" orientation of a tile among its flipped variants,
     * ensuring that tiles with the same shape structure but different color assignments can be identified as having
     * equivalent geometry.
     *
     * Note: This is different from operator< and operator<=>, which compare both shape masks AND pixel values.
     *
     * @param lhs The left-hand ShapeTile to compare
     * @param rhs The right-hand ShapeTile to compare
     * @return True if lhs's shape masks are lexicographically less than rhs's shape masks
     */
    [[nodiscard]] static bool compare_shape_only(const ShapeTile &lhs, const ShapeTile &rhs)
    {
        auto keys1 = lhs.colors_ | std::views::keys;
        auto keys2 = rhs.colors_ | std::views::keys;
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
            result.colors_.insert_or_assign(mask.flip(h, v), color);
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
        colors_.insert_or_assign(mask, color);
    }

  private:
    /**
     * @brief Helper method implementing the core PixelTile to ShapeTile conversion logic.
     *
     * @details
     * This private helper contains the common conversion logic shared by both from_pixel_tile() overloads. It accepts
     * a transparency predicate (function/lambda) that determines whether a pixel is transparent, allowing the same
     * implementation to work with both intrinsic and extrinsic transparency checking.
     *
     * @tparam InputPixelType The pixel type of the input tile
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param pixel_tile The PixelTile to convert
     * @param color_index_map The ColorIndexMap providing color-to-index mappings
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return A ShapeTile<unsigned int> with ShapeMasks mapped to color indices
     * @throws Panics if any non-transparent pixel is not found in the ColorIndexMap
     */
    template <SupportsTransparency InputPixelType, typename TransparencyPredicate>
    [[nodiscard]] static ShapeTile<unsigned int> from_pixel_tile_impl(
        const PixelTile<InputPixelType> &pixel_tile,
        const ColorIndexMap<InputPixelType> &color_index_map,
        TransparencyPredicate is_transparent_pred)
    {
        // Map from color index to ShapeMask
        std::map<unsigned int, ShapeMask> index_to_mask;

        // Iterate through all pixels
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const auto pixel = pixel_tile.at(row, col);

                // Skip transparent pixels
                if (is_transparent_pred(pixel)) {
                    continue;
                }

                // Look up the color index
                auto index_opt = color_index_map.index_at_color(pixel);
                if (!index_opt) {
                    panic("Pixel not found in ColorIndexMap");
                }

                unsigned int index = index_opt->index();

                // Create mask if it doesn't exist
                if (index_to_mask.find(index) == index_to_mask.end()) {
                    index_to_mask[index] = ShapeMask{};
                }

                // Set the bit for this position
                index_to_mask[index].set(row, col);
            }
        }

        // Build the result ShapeTile
        ShapeTile<unsigned int> result;
        for (const auto &[index, mask] : index_to_mask) {
            result.set(mask, index);
        }

        return result;
    }

    std::map<ShapeMask, PixelType> colors_;
};

} // namespace porytiles2