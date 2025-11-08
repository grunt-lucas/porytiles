#pragma once

#include <map>

#include "porytiles2/domain/models/color_index.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/domain/models/shape_tile.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

namespace details {

/**
 * @brief Helper function implementing the core PixelTile to ShapeTile conversion logic.
 *
 * @details
 * This private helper contains the common conversion logic shared by both from_pixel_tile() overloads. It accepts a
 * transparency predicate (function/lambda) that determines whether a pixel is transparent, allowing the same
 * implementation to work with both intrinsic and extrinsic transparency checking.
 *
 * @tparam PixelType The pixel type of the input tile
 * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
 * @param pixel_tile The PixelTile to convert
 * @param color_index_map The ColorIndexMap providing color-to-index mappings
 * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
 * @pre All non-transparent pixels in pixel_tile must be present in color_index_map
 * @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
 */
template <SupportsTransparency PixelType, typename TransparencyPredicate>
[[nodiscard]] ShapeTile<ColorIndex> from_pixel_tile_impl(
    const PixelTile<PixelType> &pixel_tile,
    const ColorIndexMap<PixelType> &color_index_map,
    TransparencyPredicate is_transparent_pred)
{
    // Map from color index to ShapeMask
    std::map<ColorIndex, ShapeMask> index_to_mask;

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

            const ColorIndex &index = *index_opt;

            // Create mask if it doesn't exist
            if (index_to_mask.find(index) == index_to_mask.end()) {
                index_to_mask[index] = ShapeMask{};
            }

            // Set the bit for this position
            index_to_mask[index].set(row, col);
        }
    }

    // Build the result ShapeTile
    ShapeTile<ColorIndex> result;
    for (const auto &[index, mask] : index_to_mask) {
        result.set(mask, index);
    }

    return result;
}

} // namespace details

/**
 * @brief Converts a PixelTile to a ShapeTile<ColorIndex> using a ColorIndexMap (intrinsic transparency).
 *
 * @details
 * This function creates a ShapeTile<ColorIndex> from a PixelTile by mapping each unique non-transparent color to its
 * corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed that marks all
 * pixel positions containing that color.
 *
 * This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
 *
 * The conversion process:
 * 1. Iterates through all 64 pixels in the PixelTile
 * 2. Skips intrinsically transparent pixels
 * 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
 * 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
 * 5. Builds ShapeMasks for each unique color index
 * 6. Returns a ShapeTile<ColorIndex> mapping ShapeMasks to color indices
 *
 * @tparam PixelType The pixel type of the input tile, must support intrinsic transparency
 * @param pixel_tile The PixelTile to convert
 * @param color_index_map The ColorIndexMap providing color-to-index mappings
 * @pre All non-transparent pixels in pixel_tile must be present in color_index_map
 * @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
 */
template <SupportsTransparency PixelType>
[[nodiscard]] ShapeTile<ColorIndex>
from_pixel_tile(const PixelTile<PixelType> &pixel_tile, const ColorIndexMap<PixelType> &color_index_map)
    requires requires(const PixelType &p) { p.is_transparent(); }
{
    return details::from_pixel_tile_impl(
        pixel_tile, color_index_map, [](const PixelType &p) { return p.is_transparent(); });
}

/**
 * @brief Converts a PixelTile to a ShapeTile<ColorIndex> using a ColorIndexMap (extrinsic transparency).
 *
 * @details
 * This function creates a ShapeTile<ColorIndex> from a PixelTile by mapping each unique non-transparent color to its
 * corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed that marks all
 * pixel positions containing that color.
 *
 * This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32). Transparency is
 * determined using both intrinsic (alpha=0) and extrinsic (matching the provided value) checks.
 *
 * The conversion process:
 * 1. Iterates through all 64 pixels in the PixelTile
 * 2. Skips pixels that are either intrinsically or extrinsically transparent
 * 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
 * 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
 * 5. Builds ShapeMasks for each unique color index
 * 6. Returns a ShapeTile<ColorIndex> mapping ShapeMasks to color indices
 *
 * @tparam PixelType The pixel type of the input tile, must support extrinsic transparency
 * @param pixel_tile The PixelTile to convert
 * @param color_index_map The ColorIndexMap providing color-to-index mappings
 * @param extrinsic The extrinsic transparency value to check pixels against
 * @pre All non-transparent pixels in pixel_tile must be present in color_index_map
 * @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
 */
template <SupportsTransparency PixelType>
[[nodiscard]] ShapeTile<ColorIndex> from_pixel_tile(
    const PixelTile<PixelType> &pixel_tile, const ColorIndexMap<PixelType> &color_index_map, const PixelType &extrinsic)
    requires requires(const PixelType &p) { p.is_transparent(p); }
{
    return details::from_pixel_tile_impl(
        pixel_tile, color_index_map, [&extrinsic](const PixelType &p) { return p.is_transparent(extrinsic); });
}

/**
 * @brief Converts a ShapeTile<ColorIndex> to a PixelTile using a ColorIndexMap.
 *
 * @details
 * This function creates a PixelTile<InputPixelType> from a ShapeTile<ColorIndex> by looking up the actual color for
 * each ColorIndex in the ColorIndexMap and setting the corresponding pixels in the result tile.
 *
 * The conversion process:
 * 1. Creates a default PixelTile<InputPixelType> (all pixels transparent)
 * 2. Iterates through each (ShapeMask, ColorIndex) pair in the ShapeTile
 * 3. Looks up the actual color from the ColorIndexMap using the ColorIndex
 * 4. Panics if a ColorIndex is not found in the ColorIndexMap
 * 5. For each bit set in the ShapeMask, sets the corresponding pixel in the PixelTile to that color
 * 6. Panics if masks overlap (indicates programmer error in ShapeTile construction)
 * 7. Returns the completed PixelTile
 *
 * @tparam PixelType The pixel type of the output tile, must support transparency
 * @param shape_tile The ShapeTile<ColorIndex> to convert
 * @param color_index_map The ColorIndexMap providing index-to-color mappings
 * @pre All ColorIndex values in shape_tile must be present in color_index_map
 * @pre ShapeMasks in shape_tile must not overlap
 * @return A PixelTile<PixelType> with pixels set according to the ShapeTile's masks and colors
 */
template <SupportsTransparency PixelType>
[[nodiscard]] PixelTile<PixelType>
from_shape_tile(const ShapeTile<ColorIndex> &shape_tile, const ColorIndexMap<PixelType> &color_index_map)
{
    // Start with a default (transparent) PixelTile
    PixelTile<PixelType> result;

    // Track which pixels have been set to detect overlaps
    std::array<bool, tile::size_pix> pixel_set{};

    // Iterate through each (ShapeMask, ColorIndex) pair
    for (const auto &[mask, index] : shape_tile.colors()) {
        // Look up the actual color from the ColorIndexMap
        auto color_opt = color_index_map.color_at_index(index);
        if (!color_opt) {
            panic("ColorIndex not found in ColorIndexMap");
        }

        const PixelType &color = *color_opt;

        // Set all pixels marked by this ShapeMask
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                if (mask.get(row, col)) {
                    std::size_t pixel_index = row * tile::side_length_pix + col;

                    // Check for overlap
                    if (pixel_set[pixel_index]) {
                        panic("Overlapping masks detected in ShapeTile - programmer error");
                    }

                    result.set(row, col, color);
                    pixel_set[pixel_index] = true;
                }
            }
        }
    }

    return result;
}

/**
 * @brief Converts a ShapeTile<ColorIndex> to a ShapeTile<PixelType> using a ColorIndexMap.
 *
 * @details
 * This function creates a ShapeTile<PixelType> from a ShapeTile<ColorIndex> by looking up the actual color for each
 * ColorIndex in the ColorIndexMap. The ShapeMasks remain the same, but the color values change from indices to actual
 * pixel colors.
 *
 * The conversion process:
 * 1. Creates an empty ShapeTile<PixelType> result
 * 2. Iterates through each (ShapeMask, ColorIndex) pair in the input ShapeTile
 * 3. Looks up the actual color from the ColorIndexMap using the ColorIndex
 * 4. Panics if a ColorIndex is not found in the ColorIndexMap
 * 5. Sets the ShapeMask with the pixel color in the result ShapeTile
 * 6. Returns the completed ShapeTile<PixelType>
 *
 * @tparam PixelType The pixel type of the output tile, must support transparency
 * @param shape_tile The ShapeTile<ColorIndex> to convert
 * @param color_index_map The ColorIndexMap providing index-to-color mappings
 * @pre All ColorIndex values in shape_tile must be present in color_index_map
 * @return A ShapeTile<PixelType> with the same masks but pixel colors instead of color indices
 */
template <SupportsTransparency PixelType>
[[nodiscard]] ShapeTile<PixelType>
shape_tile_to_pixel_colors(const ShapeTile<ColorIndex> &shape_tile, const ColorIndexMap<PixelType> &color_index_map)
{
    ShapeTile<PixelType> result;

    // Iterate through each (ShapeMask, ColorIndex) pair
    for (const auto &[mask, index] : shape_tile.colors()) {
        // Look up the actual color from the ColorIndexMap
        auto color_opt = color_index_map.color_at_index(index);
        if (!color_opt) {
            panic("ColorIndex not found in ColorIndexMap");
        }

        const PixelType &color = *color_opt;

        // Set the mask with the pixel color in the result
        result.set(mask, color);
    }

    return result;
}

} // namespace porytiles2
