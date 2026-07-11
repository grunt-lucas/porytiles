#pragma once

#include <map>

#include "porytiles/domain/models/color_index.hpp"
#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/shape_mask.hpp"
#include "porytiles/domain/models/shape_tile.hpp"
#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

namespace details {

/// @brief Helper function implementing the core PixelTile to ShapeTile conversion logic.
///
/// @details
/// This private helper contains the common conversion logic shared by both from_pixel_tile() overloads. It accepts a
/// transparency predicate (function/lambda) that determines whether a pixel is transparent, allowing the same
/// implementation to work with both intrinsic and extrinsic transparency checking.
///
/// @tparam PixelType The pixel type of the input tile
/// @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
/// @param pixel_tile The PixelTile to convert
/// @param color_index_map The ColorIndexMap providing color-to-index mappings
/// @param is_transparent_pred A predicate function that returns true if a pixel is transparent
/// @pre All non-transparent pixels in pixel_tile must be present in color_index_map
/// @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
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

/// @brief Helper function implementing the core color-to-index tile conversion logic.
///
/// @details
/// This private helper converts a PixelTile<ColorType> to a PixelTile<IndexPixel> by mapping each pixel's color to its
/// corresponding index in the provided palette. It accepts a transparency predicate that determines whether a pixel is
/// transparent, allowing the same implementation to work with both intrinsic and extrinsic transparency checking.
///
/// The algorithm:
/// 1. Builds a color-to-index map from the palette for O(log n) lookup
/// 2. For each pixel in the tile:
///    - If transparent (per predicate), maps to index 0
///    - If not transparent, looks up the color in the palette and uses that index
///    - If not found, panics (caller should use match_tile_to_palette first to verify coverage)
///
/// @tparam ColorType The color type of the palette and tile
/// @tparam TransparencyPredicate A callable type that takes a ColorType and returns bool
/// @param tile The PixelTile to convert to indexed form
/// @param palette The Palette containing the color-to-index mapping
/// @param is_transparent_pred A predicate function that returns true if a color is transparent
/// @pre All non-transparent colors in the tile must exist in the palette's non-0 slots
/// @pre If tile contains non-transparent pixels, palette may not be empty
/// @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
template <SupportsTransparency ColorType, std::size_t N = 0, typename TransparencyPredicate>
[[nodiscard]] PixelTile<IndexPixel> index_tile_from_color_tile_impl(
    const PixelTile<ColorType> &tile, const Palette<ColorType, N> &palette, TransparencyPredicate is_transparent_pred)
{
    // Build a color-to-index map for efficient lookup
    // Note: palette.color_to_index_map() returns PaletteIndex, convert to std::size_t
    std::map<ColorType, std::size_t> color_to_index;
    for (const auto &[color, pal_idx] : palette.color_to_index_map()) {
        color_to_index[color] = pal_idx.value();
    }

    // Convert each pixel
    std::array<IndexPixel, tile::size_pix> index_pixels;

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &pixel = tile.at(i);

        if (is_transparent_pred(pixel)) {
            // Transparent pixels map to index 0
            index_pixels[i] = IndexPixel{0};
        }
        else {
            // Non-transparent pixel requires a non-empty palette
            if (palette.size() == 0) {
                panic("non-transparent pixel found but palette is empty");
            }

            // Look up the color in the palette
            auto it = color_to_index.find(pixel);
            if (it != color_to_index.end()) {
                index_pixels[i] = IndexPixel{it->second};
            }
            else {
                panic("color not found in palette");
            }
        }
    }

    return PixelTile{index_pixels};
}

/// @brief Helper function implementing the core index-to-color tile conversion logic.
///
/// @details
/// This private helper converts a PixelTile<IndexPixel> to a PixelTile<ColorType> by mapping each pixel's index to its
/// corresponding color in the provided palette. It accepts a transparent_color parameter that determines what color to
/// use for index 0 pixels, allowing the same implementation to work with both intrinsic and extrinsic transparency.
///
/// The algorithm:
/// 1. For each pixel in the tile:
///    - If index is 0, maps to transparent_color
///    - Otherwise, looks up the color in the palette's index-to-color map
///    - If not found, panics (indicates invalid index)
///
/// @tparam ColorType The color type of the palette and output tile
/// @param index_tile The PixelTile containing IndexPixel values to convert
/// @param palette The Palette containing the colors to look up
/// @param transparent_color The color to use for index 0 (transparent) pixels
/// @pre palette is not empty
/// @pre All non-zero indices in index_tile are within the bounds of the palette [1, palette.size())
/// @return A PixelTile<ColorType> where each pixel is the palette color corresponding to the index
template <SupportsTransparency ColorType, std::size_t N = 0>
[[nodiscard]] PixelTile<ColorType> color_tile_from_index_tile_impl(
    const PixelTile<IndexPixel> &index_tile, const Palette<ColorType, N> &palette, const ColorType &transparent_color)
{
    if (palette.size() == 0) {
        panic("palette is empty");
    }

    const auto index_to_color = palette.index_to_color_map();
    std::array<ColorType, tile::size_pix> color_pixels;

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &index_pixel = index_tile.at(i);
        // Use color_index() to extract the lower 4 bits, which is the actual palette color index.
        // This is critical for true-color mode where the full 8-bit value encodes both palette index (upper 4 bits)
        // and color index (lower 4 bits). For standard 4-bit pixels, color_index() == index().
        const std::size_t color_index = index_pixel.color_index();

        if (color_index == 0) {
            // Color index 0 is the transparent slot
            color_pixels[i] = transparent_color;
        }
        else {
            // Look up in index-to-color map for non-zero indices
            auto it = index_to_color.find(PaletteIndex{color_index});
            if (it == index_to_color.end()) {
                panic(
                    "color_index " + std::to_string(color_index) + " out of palette bounds [0, " +
                    std::to_string(palette.size()) + ")");
            }
            color_pixels[i] = it->second;
        }
    }

    return PixelTile<ColorType>{color_pixels};
}

} // namespace details

/// @brief Converts a PixelTile to a ShapeTile<ColorIndex> using a ColorIndexMap (intrinsic transparency).
///
/// @details
/// This function creates a ShapeTile<ColorIndex> from a PixelTile by mapping each unique non-transparent color to its
/// corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed that marks all
/// pixel positions containing that color.
///
/// This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
///
/// The conversion process:
/// 1. Iterates through all 64 pixels in the PixelTile
/// 2. Skips intrinsically transparent pixels
/// 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
/// 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
/// 5. Builds ShapeMasks for each unique color index
/// 6. Returns a ShapeTile<ColorIndex> mapping ShapeMasks to color indices
///
/// @tparam PixelType The pixel type of the input tile, must support intrinsic transparency
/// @param pixel_tile The PixelTile to convert
/// @param color_index_map The ColorIndexMap providing color-to-index mappings
/// @pre All non-transparent pixels in pixel_tile must be present in color_index_map
/// @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
template <SupportsTransparency PixelType>
[[nodiscard]] ShapeTile<ColorIndex>
from_pixel_tile(const PixelTile<PixelType> &pixel_tile, const ColorIndexMap<PixelType> &color_index_map)
    requires requires(const PixelType &p) { p.is_transparent(); }
{
    return details::from_pixel_tile_impl(
        pixel_tile, color_index_map, [](const PixelType &p) { return p.is_transparent(); });
}

/// @brief Converts a PixelTile to a ShapeTile<ColorIndex> using a ColorIndexMap (extrinsic transparency).
///
/// @details
/// This function creates a ShapeTile<ColorIndex> from a PixelTile by mapping each unique non-transparent color to its
/// corresponding color index from the ColorIndexMap. For each color index, a ShapeMask is constructed that marks all
/// pixel positions containing that color.
///
/// This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32). Transparency is
/// determined using both intrinsic (alpha=0) and extrinsic (matching the provided value) checks.
///
/// The conversion process:
/// 1. Iterates through all 64 pixels in the PixelTile
/// 2. Skips pixels that are either intrinsically or extrinsically transparent
/// 3. For each non-transparent pixel, looks up its color index in the ColorIndexMap
/// 4. Panics if a non-transparent pixel is not found in the ColorIndexMap
/// 5. Builds ShapeMasks for each unique color index
/// 6. Returns a ShapeTile<ColorIndex> mapping ShapeMasks to color indices
///
/// @tparam PixelType The pixel type of the input tile, must support extrinsic transparency
/// @param pixel_tile The PixelTile to convert
/// @param color_index_map The ColorIndexMap providing color-to-index mappings
/// @param extrinsic The extrinsic transparency value to check pixels against
/// @pre All non-transparent pixels in pixel_tile must be present in color_index_map
/// @return A ShapeTile<ColorIndex> with ShapeMasks mapped to color indices
template <SupportsTransparency PixelType>
[[nodiscard]] ShapeTile<ColorIndex> from_pixel_tile(
    const PixelTile<PixelType> &pixel_tile, const ColorIndexMap<PixelType> &color_index_map, const PixelType &extrinsic)
    requires requires(const PixelType &p) { p.is_transparent(p); }
{
    return details::from_pixel_tile_impl(
        pixel_tile, color_index_map, [&extrinsic](const PixelType &p) { return p.is_transparent(extrinsic); });
}

/// @brief Converts a ShapeTile<ColorIndex> to a PixelTile using a ColorIndexMap.
///
/// @details
/// This function creates a PixelTile<InputPixelType> from a ShapeTile<ColorIndex> by looking up the actual color for
/// each ColorIndex in the ColorIndexMap and setting the corresponding pixels in the result tile.
///
/// The conversion process:
/// 1. Creates a default PixelTile<InputPixelType> (all pixels transparent)
/// 2. Iterates through each (ShapeMask, ColorIndex) pair in the ShapeTile
/// 3. Looks up the actual color from the ColorIndexMap using the ColorIndex
/// 4. Panics if a ColorIndex is not found in the ColorIndexMap
/// 5. For each bit set in the ShapeMask, sets the corresponding pixel in the PixelTile to that color
/// 6. Panics if masks overlap (indicates programmer error in ShapeTile construction)
/// 7. Returns the completed PixelTile
///
/// @tparam PixelType The pixel type of the output tile, must support transparency
/// @param shape_tile The ShapeTile<ColorIndex> to convert
/// @param color_index_map The ColorIndexMap providing index-to-color mappings
/// @pre All ColorIndex values in shape_tile must be present in color_index_map
/// @pre ShapeMasks in shape_tile must not overlap
/// @return A PixelTile<PixelType> with pixels set according to the ShapeTile's masks and colors
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

/// @brief Converts a ShapeTile<ColorIndex> to a ShapeTile<PixelType> using a ColorIndexMap.
///
/// @details
/// This function creates a ShapeTile<PixelType> from a ShapeTile<ColorIndex> by looking up the actual color for each
/// ColorIndex in the ColorIndexMap. The ShapeMasks remain the same, but the color values change from indices to actual
/// pixel colors.
///
/// The conversion process:
/// 1. Creates an empty ShapeTile<PixelType> result
/// 2. Iterates through each (ShapeMask, ColorIndex) pair in the input ShapeTile
/// 3. Looks up the actual color from the ColorIndexMap using the ColorIndex
/// 4. Panics if a ColorIndex is not found in the ColorIndexMap
/// 5. Sets the ShapeMask with the pixel color in the result ShapeTile
/// 6. Returns the completed ShapeTile<PixelType>
///
/// @tparam PixelType The pixel type of the output tile, must support transparency
/// @param shape_tile The ShapeTile<ColorIndex> to convert
/// @param color_index_map The ColorIndexMap providing index-to-color mappings
/// @pre All ColorIndex values in shape_tile must be present in color_index_map
/// @return A ShapeTile<PixelType> with the same masks but pixel colors instead of color indices
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

/// @brief Converts a PixelTile<IndexPixel> to a PixelTile<ColorType> using a palette (intrinsic transparency).
///
/// @details
/// This function takes an indexed tile (where each pixel contains a palette index) and converts it to a color tile by
/// looking up the actual color for each index in the provided palette. Index 0 pixels are mapped to the default-
/// constructed ColorType{} (intrinsic transparency representation).
///
/// This overload is only available for color types that support intrinsic transparency.
///
/// @tparam ColorType The color type of the palette and output tile, must support intrinsic transparency
/// @param index_tile The PixelTile containing IndexPixel values to convert
/// @param palette The Palette containing the colors to look up
/// @pre palette is not empty
/// @pre All non-zero indices in index_tile are within the bounds of the palette [1, palette.size())
/// @return A PixelTile<ColorType> where each pixel is the palette color corresponding to the index in index_tile
template <SupportsTransparency ColorType, std::size_t N = 0>
[[nodiscard]] PixelTile<ColorType>
color_tile_from_index_tile(const PixelTile<IndexPixel> &index_tile, const Palette<ColorType, N> &palette)
    requires requires(const ColorType &c) { c.is_transparent(); }
{
    return details::color_tile_from_index_tile_impl(index_tile, palette, ColorType{});
}

/// @brief Converts a PixelTile<IndexPixel> to a PixelTile<ColorType> using a palette (extrinsic transparency).
///
/// @details
/// This function takes an indexed tile (where each pixel contains a palette index) and converts it to a color tile by
/// looking up the actual color for each index in the provided palette. Index 0 pixels are mapped to the provided
/// extrinsic transparency color.
///
/// This overload is only available for color types that support extrinsic transparency.
///
/// @tparam ColorType The color type of the palette and output tile, must support extrinsic transparency
/// @param index_tile The PixelTile containing IndexPixel values to convert
/// @param palette The Palette containing the colors to look up
/// @param extrinsic The extrinsic transparency color to use for index 0 pixels
/// @pre palette is not empty
/// @pre All non-zero indices in index_tile are within the bounds of the palette [1, palette.size())
/// @return A PixelTile<ColorType> where each pixel is the palette color corresponding to the index in index_tile
template <SupportsTransparency ColorType, std::size_t N = 0>
[[nodiscard]] PixelTile<ColorType> color_tile_from_index_tile(
    const PixelTile<IndexPixel> &index_tile, const Palette<ColorType, N> &palette, const ColorType &extrinsic)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    return details::color_tile_from_index_tile_impl(index_tile, palette, extrinsic);
}

/// @brief Converts a PixelTile<ColorType> to indexed form using a palette (intrinsic transparency only).
///
/// @details
/// This function converts a color tile to an indexed tile by finding each non-transparent pixel's color in the palette
/// and storing the corresponding palette index. Intrinsically transparent pixels (those reporting true from
/// parameterless is_transparent()) are mapped to index 0.
///
/// This overload is only available for color types that support intrinsic transparency.
///
/// @tparam ColorType The color type of the tile and palette, must support intrinsic transparency
/// @param tile The PixelTile to convert to indexed form
/// @param palette The Palette containing the color-to-index mapping
/// @pre All non-transparent colors in the tile must exist in the palette
/// @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
template <SupportsTransparency ColorType, std::size_t N = 0>
[[nodiscard]] PixelTile<IndexPixel>
index_tile_from_color_tile(const PixelTile<ColorType> &tile, const Palette<ColorType, N> &palette)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    return details::index_tile_from_color_tile_impl(
        tile, palette, [](const ColorType &c) { return c.is_transparent(); });
}

/// @brief Converts a PixelTile<ColorType> to indexed form using a palette (extrinsic transparency).
///
/// @details
/// This function converts a color tile to an indexed tile by finding each non-transparent pixel's color in the palette
/// and storing the corresponding palette index. Both intrinsically transparent pixels (alpha=0) and extrinsically
/// transparent pixels (matching the extrinsic parameter) are mapped to index 0.
///
/// This overload is only available for color types that support extrinsic transparency.
///
/// @tparam ColorType The color type of the tile and palette, must support extrinsic transparency
/// @param tile The PixelTile to convert to indexed form
/// @param palette The Palette containing the color-to-index mapping
/// @param extrinsic The extrinsic transparency value to check pixels against
/// @pre All non-transparent colors in the tile must exist in the palette
/// @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
template <SupportsTransparency ColorType, std::size_t N = 0>
[[nodiscard]] PixelTile<IndexPixel> index_tile_from_color_tile(
    const PixelTile<ColorType> &tile, const Palette<ColorType, N> &palette, const ColorType &extrinsic)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    return details::index_tile_from_color_tile_impl(
        tile, palette, [&extrinsic](const ColorType &c) { return c.is_transparent(extrinsic); });
}

} // namespace porytiles
