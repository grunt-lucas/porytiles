#pragma once

#include <algorithm>
#include <set>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

/*
 * TODO: this needs testing.
 *
 * TODO: Let's make a PalettePrinter class like TilePrinter
 *
 * TODO: It might also be useful to save which tile pixels weren't covered by a given palette, so we can display to the
 * user?
 *
 * TODO: should this be a service class? Or is it fine as a free function
 */

/**
 * @brief Result type for palette matching operations.
 *
 * @details
 * PaletteMatchResult encapsulates the outcome of matching a PixelTile against a Palette. It indicates whether the
 * palette completely covers all non-transparent colors in the tile, and if not, which colors are missing and which
 * are present.
 *
 * @tparam ColorType The color type of the palette and tile
 */
template <SupportsTransparency ColorType>
struct PaletteMatchResult {
    /**
     * @brief True if the palette covers all non-transparent colors in the tile, false otherwise.
     */
    bool is_covered;

    /**
     * @brief The set of non-transparent colors from the tile that are NOT present in the palette.
     */
    std::set<ColorType> missing_colors;

    /**
     * @brief The set of non-transparent colors from the tile that ARE present in the palette.
     */
    std::set<ColorType> covered_colors;
};

namespace details {

/**
 * @brief Helper function implementing the core palette matching logic.
 *
 * @details
 * This private helper contains the common matching logic shared by both match_tile_to_palette() overloads. It
 * accepts a transparency predicate (function/lambda) that determines whether a pixel is transparent, allowing the
 * same implementation to work with both intrinsic and extrinsic transparency checking.
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

    // Extract all unique non-transparent colors from the tile
    std::set<ColorType> tile_colors;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &pixel = tile.at(i);
        if (!is_transparent_pred(pixel)) {
            tile_colors.insert(pixel);
        }
    }

    // Get the palette colors as a set for efficient lookup
    const auto &palette_colors_vec = palette.colors();
    std::set<ColorType> palette_colors_set{palette_colors_vec.begin(), palette_colors_vec.end()};

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

} // namespace details

/**
 * @brief Matches a PixelTile against a Palette (intrinsic transparency only).
 *
 * @details
 * This function determines whether the provided palette contains all non-transparent colors present in the tile.
 * Only intrinsically transparent pixels (those reporting true from parameterless is_transparent()) are treated as
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
 * This function determines whether the provided palette contains all non-transparent colors present in the tile.
 * Both intrinsically transparent pixels (alpha=0) and extrinsically transparent pixels (matching the extrinsic
 * parameter) are treated as transparent.
 *
 * This overload is only available for color types that support extrinsic transparency.
 *
 * IMPORTANT: If the tile contains any extrinsically transparent pixels, palette slot 0 MUST match the extrinsic
 * transparency color. If not, this function will panic.
 *
 * The matching process:
 * 1. Checks if the tile contains any extrinsically transparent pixels
 * 2. If yes, verifies that palette slot 0 matches the extrinsic transparency color (panics if not)
 * 3. Extracts all unique non-transparent colors from the tile
 * 4. For each color, checks if it exists in the palette
 * 5. Categorizes colors as either "covered" or "missing"
 * 6. Returns a result indicating coverage status and the color sets
 *
 * @tparam ColorType The color type of the palette and tile, must support extrinsic transparency
 * @param tile The PixelTile to match against the palette
 * @param palette The Palette to check for color coverage
 * @param extrinsic The extrinsic transparency value to check pixels against
 * @return A PaletteMatchResult indicating whether the palette covers the tile and which colors are covered/missing
 * @throws Panics if the tile contains extrinsic transparency that does not match palette slot 0
 */
template <SupportsTransparency ColorType>
[[nodiscard]] PaletteMatchResult<ColorType>
match_tile_to_palette(const PixelTile<ColorType> &tile, const Palette<ColorType> &palette, const ColorType &extrinsic)
    requires requires(const ColorType &c) { c.is_transparent(c); }
{
    // Check if the tile contains any extrinsically transparent pixels
    bool has_extrinsic_transparency = false;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const auto &pixel = tile.at(i);
        // Check if pixel is extrinsically transparent but NOT intrinsically transparent
        // (We care about pixels that match the extrinsic color specifically)
        if (!pixel.is_transparent(ColorType{}) && pixel.is_transparent(extrinsic)) {
            has_extrinsic_transparency = true;
            break;
        }
    }

    // If the tile has extrinsic transparency, verify palette slot 0 matches
    if (has_extrinsic_transparency) {
        if (palette.size() == 0 || palette.colors().at(0) != extrinsic) {
            // TODO: this should be noted as a precondition instead of a throws
            // TODO: we should have an earlier compilation step that normalizes transpareny in Porymap pals, since their
            // default slot 0 transparency doesn't matter. When you import a vanilla set to Porytiles, all transparent
            // pixels get normalized to the configured extrinsic transparency.
            panic("Tile contains extrinsic transparency that does not match palette slot 0");
        }
    }

    return details::match_tile_to_palette_impl(
        tile, palette, [&extrinsic](const ColorType &c) { return c.is_transparent(extrinsic); });
}

} // namespace porytiles2
