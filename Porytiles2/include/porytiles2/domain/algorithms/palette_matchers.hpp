#pragma once

#include <algorithm>
#include <map>
#include <set>

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Result type for palette matching operations.
 *
 * @details
 * PaletteMatchResult encapsulates the outcome of matching a PixelTile against a Palette. It indicates whether the
 * palette completely covers all non-transparent colors in the tile, and if not, which colors are missing and which
 * are present. It also tracks the specific pixel positions that are not covered by the palette.
 *
 * @tparam ColorType The color type of the palette and tile
 */
template <SupportsTransparency ColorType>
struct PaletteMatchResult {
    /**
     * @brief True if the palette covers all non-transparent colors in the tile, false otherwise.
     */
    bool is_covered = false;

    /**
     * @brief The set of non-transparent colors from the tile that are NOT present in the palette.
     */
    std::set<ColorType> missing_colors;

    /**
     * @brief The set of non-transparent colors from the tile that ARE present in the palette.
     */
    std::set<ColorType> covered_colors;

    /**
     * @brief The linear indices of tile pixels whose colors are not covered by the palette.
     *
     * @details
     * This vector contains the indices [0, 64) of pixels in the tile that have non-transparent colors not present
     * in the palette. These indices can be converted to (row, col) coordinates using tile::index_to_row_col() if
     * needed. Empty if all non-transparent pixels are covered by the palette.
     */
    std::vector<std::size_t> uncovered_pixel_indices;

    /**
     * @brief The palette index of the match, useful in batch operations.
     */
    unsigned int pal_index = 0;
};

namespace details {

/**
 * @brief Helper function implementing the core palette matching logic.
 *
 * @details
 * This private helper contains the common matching logic shared by both match_tile_to_palette() overloads. It accepts a
 * transparency predicate (function/lambda) that determines whether a pixel is transparent, allowing the same
 * implementation to work with both intrinsic and extrinsic transparency checking.
 *
 * The algorithm:
 * 1. Extracts all unique non-transparent colors from the tile
 * 2. For each color, checks if it exists in the palette
 * 3. Categorizes colors as either "covered" (found in palette) or "missing" (not in palette)
 * 4. Sets is_covered to true if no colors are missing
 *
 * @tparam ColorType The color type of the palette and tile
 * @tparam TransparencyPredicate A callable type that takes a ColorType and returns bool
 * @param tile The PixelTile to match against the palette
 * @param palette The Palette to check for color coverage
 * @param is_transparent_pred A predicate function that returns true if a color is transparent
 * @return A PaletteMatchResult indicating coverage status and color sets
 */
template <SupportsTransparency ColorType, typename TransparencyPredicate>
[[nodiscard]] PaletteMatchResult<ColorType> match_tile_to_palette_impl(
    const PixelTile<ColorType> &tile, const Palette<ColorType> &palette, TransparencyPredicate is_transparent_pred)
{
    PaletteMatchResult<ColorType> result;

    // Get the palette colors as a set for efficient lookup
    const auto &palette_colors_vec = palette.colors();
    std::set<ColorType> palette_colors_set{palette_colors_vec.begin(), palette_colors_vec.end()};

    // Extract all unique non-transparent colors from the tile and track uncovered pixels
    std::set<ColorType> tile_colors;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &pixel = tile.at(i);
        if (!is_transparent_pred(pixel)) {
            tile_colors.insert(pixel);

            // If this pixel's color is not in the palette, record its index
            if (!palette_colors_set.contains(pixel)) {
                result.uncovered_pixel_indices.push_back(i);
            }
        }
    }

    // Categorize each tile color as covered or missing
    for (const auto &color : tile_colors) {
        if (palette_colors_set.contains(color)) {
            result.covered_colors.insert(color);
        }
        else {
            result.missing_colors.insert(color);
        }
    }

    // The tile is covered if there are no missing colors
    result.is_covered = result.missing_colors.empty();

    return result;
}

/**
 * @brief Helper function implementing the core color-to-index tile conversion logic.
 *
 * @details
 * This private helper converts a PixelTile<ColorType> to a PixelTile<IndexPixel> by mapping each pixel's color to its
 * corresponding index in the provided palette. It accepts a transparency predicate that determines whether a pixel is
 * transparent, allowing the same implementation to work with both intrinsic and extrinsic transparency checking.
 *
 * The algorithm:
 * 1. Builds a color-to-index map from the palette for O(log n) lookup
 * 2. For each pixel in the tile:
 *    - If transparent (per predicate), maps to index 0
 *    - If not transparent, looks up the color in the palette and uses that index
 *    - If not found, panics (caller should use match_tile_to_palette first to verify coverage)
 *
 * @tparam ColorType The color type of the palette and tile
 * @tparam TransparencyPredicate A callable type that takes a ColorType and returns bool
 * @param tile The PixelTile to convert to indexed form
 * @param palette The Palette containing the color-to-index mapping
 * @param is_transparent_pred A predicate function that returns true if a color is transparent
 * @pre All non-transparent colors in the tile must exist in the palette
 * @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
 */
template <SupportsTransparency ColorType, typename TransparencyPredicate>
[[nodiscard]] PixelTile<IndexPixel> index_tile_from_color_tile_impl(
    const PixelTile<ColorType> &tile, const Palette<ColorType> &palette, TransparencyPredicate is_transparent_pred)
{
    // Build a color-to-index map for efficient lookup
    const auto &palette_colors = palette.colors();
    std::map<ColorType, unsigned int> color_to_index;
    for (std::size_t i = 0; i < palette_colors.size(); ++i) {
        // Only store first occurrence of each color (in case of duplicates)
        if (!color_to_index.contains(palette_colors[i])) {
            color_to_index[palette_colors[i]] = static_cast<unsigned int>(i);
        }
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
            /*
             * If a non-transparent pixel matches palette slot 0, we have an issue. In some cases, pal slot 0 won't
             * contain the actual extrinsically transparent color. E.g. if this palette has a PLA file for DNS, slot 0
             * might contain a blending color. We have to panic here because if the user specified a blending color in
             * their tile, marking it as IndexPixel{0} will make it transparent, which is probably not intended.
             */
            // TODO: wouldn't it be possible for users to accidentally hit this case? We'll need to figure this out.
            // See: https://discord.com/channels/976252009114140682/1023424424713650196/1441433315679801386
            // Maybe in this case, we can search the palette for another usage of this color, and use that if present.
            // And if it's not, then we have to fail. This should probably return a Chainable since this is a
            // user-reachable error condition.
            if (pixel == palette_colors[0]) {
                panic("Used non-transparent pixel that matched pal slot 0");
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

} // namespace details

/**
 * @brief Matches a PixelTile against a Palette (intrinsic transparency only).
 *
 * @details
 * This function determines whether the provided palette contains all non-transparent colors present in the tile. Only
 * intrinsically transparent pixels (those reporting true from parameterless is_transparent()) are treated as
 * transparent.
 *
 * This overload is only available for color types that support intrinsic transparency.
 *
 * The matching process:
 * 1. Extracts all unique non-transparent colors from the tile
 * 2. For each color, checks if it exists in the palette
 * 3. Categorizes colors as either "covered" or "missing"
 * 4. Returns a result indicating coverage status and the color sets
 *
 * @tparam ColorType The color type of the palette and tile, must support intrinsic transparency
 * @param tile The PixelTile to match against the palette
 * @param palette The Palette to check for color coverage
 * @return A PaletteMatchResult indicating whether the palette covers the tile and which colors are covered/missing
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PaletteMatchResult<ColorType>
match_tile_to_palette(const PixelTile<ColorType> &tile, const Palette<ColorType> &palette)
    requires requires(const ColorType &c) { c.is_transparent(); }
{
    return details::match_tile_to_palette_impl(tile, palette, [](const ColorType &c) { return c.is_transparent(); });
}

/**
 * @brief Matches a PixelTile against a Palette (extrinsic transparency).
 *
 * @details
 * This function determines whether the provided palette contains all non-transparent colors present in the tile. Both
 * intrinsically transparent pixels (alpha=0) and extrinsically transparent pixels (matching the extrinsic parameter)
 * are treated as transparent.
 *
 * This overload is only available for color types that support extrinsic transparency.
 *
 * @tparam ColorType The color type of the palette and tile, must support extrinsic transparency
 * @param tile The PixelTile to match against the palette
 * @param palette The Palette to check for color coverage
 * @param extrinsic The extrinsic transparency value to check pixels against
 * @pre The Palette is not empty
 * @pre The extrinsic transparency must match the color in slot 0 of the Palette
 * @return A PaletteMatchResult indicating whether the palette covers the tile and which colors are covered/missing
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PaletteMatchResult<ColorType>
match_tile_to_palette(const PixelTile<ColorType> &tile, const Palette<ColorType> &palette, const ColorType &extrinsic)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    if (palette.size() == 0) {
        panic("palette is empty");
    }

    if (palette.colors().at(0) != extrinsic) {
        // TODO: we need to change this. We have to allow slot 0 of a palette to be anything, since the PLA files allow
        // users to specify arbitrary colors in slot 0 for blending. We want Porytiles to be surgical. Even though we'll
        // normalize the transparency color upon import, we don't want to clobber the values in the user's provided pal
        // files.
        panic("palette slot 0 did not match provided extrinsic transparency value");
    }

    return details::match_tile_to_palette_impl(
        tile, palette, [&extrinsic](const ColorType &c) { return c.is_transparent(extrinsic); });
}

/**
 * @brief Converts a PixelTile<IndexPixel> to a PixelTile<ColorType> using a palette.
 *
 * @details
 * This function takes an indexed tile (where each pixel contains a palette index) and converts it to a color tile by
 * looking up the actual color for each index in the provided palette. Each pixel's index value is used to retrieve the
 * corresponding color from the palette's color vector.
 *
 * @tparam ColorType The color type of the palette and output tile
 * @param index_tile The PixelTile containing IndexPixel values to convert
 * @param palette The Palette containing the colors to look up
 * @pre palette is not empty
 * @pre All indices in index_tile are within the bounds of the palette [0, palette.size())
 * @return A PixelTile<ColorType> where each pixel is the palette color corresponding to the index in index_tile
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PixelTile<ColorType>
color_tile_from_index_tile(const PixelTile<IndexPixel> &index_tile, const Palette<ColorType> &palette)
{
    if (palette.size() == 0) {
        panic("palette is empty");
    }

    const auto &palette_colors = palette.colors();
    std::array<ColorType, tile::size_pix> color_pixels;

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &index_pixel = index_tile.at(i);
        const unsigned int index = index_pixel.index();

        if (index >= palette_colors.size()) {
            panic(
                "index " + std::to_string(index) + " out of palette bounds [0, " +
                std::to_string(palette_colors.size()) + ")");
        }

        color_pixels[i] = palette_colors.at(index);
    }

    return PixelTile<ColorType>{color_pixels};
}

/**
 * @brief Converts a PixelTile<ColorType> to indexed form using a palette (intrinsic transparency only).
 *
 * @details
 * This function converts a color tile to an indexed tile by finding each non-transparent pixel's color in the palette
 * and storing the corresponding palette index. Intrinsically transparent pixels (those reporting true from
 * parameterless is_transparent()) are mapped to index 0.
 *
 * This overload is only available for color types that support intrinsic transparency.
 *
 * @tparam ColorType The color type of the tile and palette, must support intrinsic transparency
 * @param tile The PixelTile to convert to indexed form
 * @param palette The Palette containing the color-to-index mapping
 * @pre All non-transparent colors in the tile must exist in the palette
 * @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PixelTile<IndexPixel>
index_tile_from_color_tile(const PixelTile<ColorType> &tile, const Palette<ColorType> &palette)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    return details::index_tile_from_color_tile_impl(
        tile, palette, [](const ColorType &c) { return c.is_transparent(); });
}

/**
 * @brief Converts a PixelTile<ColorType> to indexed form using a palette (extrinsic transparency).
 *
 * @details
 * This function converts a color tile to an indexed tile by finding each non-transparent pixel's color in the palette
 * and storing the corresponding palette index. Both intrinsically transparent pixels (alpha=0) and extrinsically
 * transparent pixels (matching the extrinsic parameter) are mapped to index 0.
 *
 * This overload is only available for color types that support extrinsic transparency.
 *
 * @tparam ColorType The color type of the tile and palette, must support extrinsic transparency
 * @param tile The PixelTile to convert to indexed form
 * @param palette The Palette containing the color-to-index mapping
 * @param extrinsic The extrinsic transparency value to check pixels against
 * @pre All non-transparent colors in the tile must exist in the palette
 * @return A PixelTile<IndexPixel> where each pixel is the palette index corresponding to the color
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PixelTile<IndexPixel> index_tile_from_color_tile(
    const PixelTile<ColorType> &tile, const Palette<ColorType> &palette, const ColorType &extrinsic)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    return details::index_tile_from_color_tile_impl(
        tile, palette, [&extrinsic](const ColorType &c) { return c.is_transparent(extrinsic); });
}

/**
 * @brief Finds the best palette match(es) for a tile (extrinsic transparency).
 *
 * @details
 * This function matches a tile against a vector of palettes and returns the best match(es):
 * - If any palettes completely cover the tile, returns ALL complete matches (ignoring top_n)
 * - If no palettes completely cover the tile, returns up to top_n best matches sorted by quality
 *
 * Quality is determined by the number of missing_colors (fewer is better). If multiple palettes have the same number of
 * missing colors, they maintain their original order in the palettes vector.
 *
 * This overload supports both intrinsic (alpha=0) and extrinsic transparency checking. This overload is only available
 * for color types that support extrinsic transparency.
 *
 * @tparam ColorType The color type of the palette and tile, must support extrinsic transparency
 * @param tile The PixelTile to match against the palettes
 * @param palettes The vector of Palettes to check for color coverage
 * @param extrinsic The extrinsic transparency value to check pixels against
 * @param top_n Maximum number of results to return when no complete match exists (ignored if complete matches found)
 * @pre palettes is not empty
 * @pre top_n > 0
 * @pre All palettes are not empty
 * @pre All palettes have extrinsic color in slot 0
 * @return A vector of PaletteMatchResult, either all complete matches or top_n best non-matches
 * @post The returned vector is non-empty and exhibits coverage homogeneity: all PaletteMatchResult elements
 * possess identical is_covered values. This property enables deterministic match classification via examination of any
 * single element, conventionally the first: `results.at(0).is_covered`. The function partitions the result space into
 * two mutually exclusive sets—complete matches (is_covered = true) or partial matches (is_covered = false)—never
 * returning a heterogeneous mixture.
 */
template <SupportsTransparency ColorType>
[[nodiscard]] std::vector<PaletteMatchResult<ColorType>> match_or_best(
    const PixelTile<ColorType> &tile,
    const std::vector<Palette<ColorType>> &palettes,
    const ColorType &extrinsic,
    std::size_t top_n)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    if (palettes.empty()) {
        panic("palettes vector is empty");
    }
    if (top_n == 0) {
        panic("top_n must be greater than 0");
    }

    // Match tile against all palettes
    std::vector<PaletteMatchResult<ColorType>> complete_matches;
    std::vector<PaletteMatchResult<ColorType>> incomplete_matches;

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        auto result = match_tile_to_palette(tile, palettes[i], extrinsic);
        result.pal_index = static_cast<unsigned int>(i);

        if (result.is_covered) {
            complete_matches.push_back(result);
        }
        else {
            incomplete_matches.push_back(result);
        }
    }

    // If we found any complete matches, return all of them (ignore top_n)
    if (!complete_matches.empty()) {
        return complete_matches;
    }

    // No complete matches found, sort incomplete matches by quality (fewer missing_colors is better)
    std::sort(incomplete_matches.begin(), incomplete_matches.end(), [](const auto &a, const auto &b) {
        return a.missing_colors.size() < b.missing_colors.size();
    });

    // Return top_n results (or all if fewer than top_n)
    if (incomplete_matches.size() > top_n) {
        incomplete_matches.resize(top_n);
    }

    return incomplete_matches;
}

} // namespace porytiles2
