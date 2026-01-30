#include "porytiles2/domain/services/anim_key_frame_mangler.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

// Pixel priority order for mangling (least visually impactful first)
// Corners: (0,0), (0,7), (7,0), (7,7) → indices 0, 7, 56, 63
// Top edge: 1-6
// Left edge: 8, 16, 24, 32, 40, 48
// Right edge: 15, 23, 31, 39, 47, 55
// Bottom edge: 57-62
// Interior: remaining pixels
constexpr std::array<std::size_t, tile::size_pix> pixel_priority_order = {
    // Corners first (least visible)
    0,
    7,
    56,
    63,
    // Top edge
    1,
    2,
    3,
    4,
    5,
    6,
    // Left edge
    8,
    16,
    24,
    32,
    40,
    48,
    // Right edge
    15,
    23,
    31,
    39,
    47,
    55,
    // Bottom edge
    57,
    58,
    59,
    60,
    61,
    62,
    // Interior (most visible, last resort)
    9,
    10,
    11,
    12,
    13,
    14,
    17,
    18,
    19,
    20,
    21,
    22,
    25,
    26,
    27,
    28,
    29,
    30,
    33,
    34,
    35,
    36,
    37,
    38,
    41,
    42,
    43,
    44,
    45,
    46,
    49,
    50,
    51,
    52,
    53,
    54};

/**
 * @brief Calculates squared RGB distance between two colors.
 *
 * @details
 * Uses squared Euclidean distance in RGB space. Squared distance avoids the sqrt operation and is sufficient for
 * comparison purposes.
 *
 * @param a First color
 * @param b Second color
 * @return Squared RGB distance
 */
int color_distance_squared(const Rgba32 &a, const Rgba32 &b)
{
    const int dr = static_cast<int>(a.red()) - static_cast<int>(b.red());
    const int dg = static_cast<int>(a.green()) - static_cast<int>(b.green());
    const int db = static_cast<int>(a.blue()) - static_cast<int>(b.blue());
    return dr * dr + dg * dg + db * db;
}

/**
 * @brief Finds the most similar color in the palette to a given color.
 *
 * @details
 * Searches through all palette colors (indices 1-15) excluding the current color and returns the color index with the
 * smallest RGB distance. This allows the mangler to introduce new colors from the palette that aren't currently in the
 * tile.
 *
 * @param current_color_index The current color index to find an alternative for
 * @param palette The palette to look up actual RGB values
 * @return The color index of the most similar alternative, or nullopt if no alternative exists
 */
std::optional<std::size_t>
find_most_similar_color(std::size_t current_color_index, const Palette<Rgba32, pal::max_size> &palette)
{
    const Rgba32 current_color = palette.at(current_color_index);
    std::optional<std::size_t> best_index = std::nullopt;
    int best_distance = std::numeric_limits<int>::max();

    // Search ALL palette colors (1-15), not just those in the tile
    for (std::size_t candidate_index = 1; candidate_index < pal::max_size; ++candidate_index) {
        if (candidate_index == current_color_index) {
            continue; // Skip the current color
        }

        const Rgba32 candidate_color = palette.at(candidate_index);
        const int distance = color_distance_squared(current_color, candidate_color);

        if (distance < best_distance) {
            best_distance = distance;
            best_index = candidate_index;
        }
    }

    return best_index;
}

/**
 * @brief Attempts to mangle a single tile to make it unique.
 *
 * @details
 * Tries each pixel in priority order, attempting to swap it to a visually similar color. Modifies exactly one pixel
 * and returns the first successful modification that produces a unique tile. Uniqueness is checked against canonical
 * forms to ensure tiles that are flip-equivalent are also treated as duplicates.
 *
 * @param tile The tile to mangle
 * @param tile_index The index of the tile in the animation
 * @param palette The palette for color similarity calculations
 * @param all_existing_canonical_tiles Set of canonical base tiles that the result must be unique against
 * @return A pair of (mangled tile, mangle record), or nullopt if no valid mangle was found
 */
std::optional<std::pair<PixelTile<IndexPixel>, TileMangleRecord>> try_mangle_tile(
    const PixelTile<IndexPixel> &tile,
    std::size_t tile_index,
    const Palette<Rgba32, pal::max_size> &palette,
    const std::set<PixelTile<IndexPixel>> &all_existing_canonical_tiles)
{
    // Try each pixel in priority order
    for (std::size_t pixel_index : pixel_priority_order) {
        const IndexPixel original_pixel = tile.at(pixel_index);

        // Skip transparent pixels
        if (original_pixel.is_transparent()) {
            continue;
        }

        const std::size_t original_color_index = original_pixel.color_index();
        const std::size_t original_palette_index = original_pixel.palette_index();

        // Find the most similar alternative color from the entire palette
        const std::optional<std::size_t> alternative_color = find_most_similar_color(original_color_index, palette);

        if (!alternative_color.has_value()) {
            continue; // No alternative for this pixel
        }

        // Construct the new pixel preserving the palette index (upper 4 bits)
        const std::size_t new_index = (original_palette_index << 4) | alternative_color.value();
        const IndexPixel mangled_pixel{new_index};

        // Create candidate tile with the swap
        PixelTile<IndexPixel> candidate_tile = tile;
        candidate_tile.set(pixel_index, mangled_pixel);

        // Check if this produces a unique tile (in canonical form)
        CanonicalPixelTile<IndexPixel> canonical_candidate{candidate_tile};
        const PixelTile<IndexPixel> &candidate_base = canonical_candidate;
        if (!all_existing_canonical_tiles.contains(candidate_base)) {
            // Found a valid mangle!
            TileMangleRecord record{
                .tile_index = tile_index,
                .pixel_index = pixel_index,
                .original_pixel = original_pixel,
                .mangled_pixel = mangled_pixel};
            return std::make_pair(candidate_tile, record);
        }
    }

    // No valid mangle found for any pixel
    return std::nullopt;
}

} // namespace

namespace porytiles2 {

AnimKeyFrameMangler::AnimKeyFrameMangler(
    gsl::not_null<const UserDiagnostics *> diag, gsl::not_null<const TilePrinter *> tile_printer)
    : diag_{diag}, tile_printer_{tile_printer}
{
}

ChainableResult<MangleResult> AnimKeyFrameMangler::mangle_duplicates(
    const std::string &anim_name,
    std::vector<PixelTile<IndexPixel>> tiles,
    const Palette<Rgba32, pal::max_size> &palette,
    const Rgba32 &extrinsic_transparency,
    const std::set<PixelTile<IndexPixel>> &existing_canonical_tiles) const
{
    MangleResult result;
    result.tiles = std::move(tiles);

    /*
     * TODO: I think there is *STILL* actually a potential bug with the mangling process. Here, we are tracking
     * "existing" tiles using the canonical index version. The 'existing_canonical_tiles' variable is set to contain all
     * the tiles from tiles.png, which is supposed to prevent us from creating a mangled tile that matches one of the
     * existing tiles.
     *
     * TODO: what am I saying here?
     */

    /*
     * Build a set of all canonical tiles we need to be unique against. This includes the input existing_canonical_tiles
     * plus tiles we've already processed. Using canonical forms ensures tiles that are flip-equivalent are treated as
     * duplicates.
     */
    std::set<PixelTile<IndexPixel>> all_canonical_tiles = existing_canonical_tiles;

    // Map to track which canonical tiles we've seen at which indices (for duplicate detection)
    std::map<PixelTile<IndexPixel>, std::size_t> canonical_first_occurrence;

    /*
     * Each tile index is visited exactly once. A tile is mangled at most once, producing at most one TileMangleRecord.
     * This guarantees that mangle_records contains non-overlapping entries (no two records share the same tile_index),
     * so they can be applied independently in any order.
     */
    for (std::size_t i = 0; i < result.tiles.size(); ++i) {
        PixelTile<IndexPixel> &current_tile = result.tiles[i];

        // Canonicalize the current tile for duplicate checking
        CanonicalPixelTile canonical_current{current_tile};
        const PixelTile<IndexPixel> &current_base = canonical_current;

        // Check if this tile is a duplicate (either of existing_canonical_tiles or a previous tile in this batch)
        auto it = canonical_first_occurrence.find(current_base);
        const bool is_duplicate_of_previous = it != canonical_first_occurrence.end();
        const bool is_duplicate_of_existing =
            existing_canonical_tiles.contains(current_base) && !is_duplicate_of_previous;

        if (is_duplicate_of_previous || is_duplicate_of_existing) {
            // Need to mangle this tile
            const PixelTile<IndexPixel> original_tile = current_tile;
            const std::optional<std::pair<PixelTile<IndexPixel>, TileMangleRecord>> mangle_result =
                try_mangle_tile(current_tile, i, palette, all_canonical_tiles);

            if (!mangle_result.has_value()) {
                // Could not mangle the tile - this is an error
                std::vector<std::string> err_msg;
                err_msg.push_back(diag_->formatter().format(
                    "Failed to mangle duplicate key frame tile {} in animation '{}'",
                    FormatParam{i, Style::bold},
                    FormatParam{anim_name, Style::bold}));
                err_msg.emplace_back();
                err_msg.emplace_back("The tile could not be modified to be unique. This may occur if:");
                err_msg.emplace_back("  - All possible pixel swaps still produce duplicate tiles");
                return FormattableError{err_msg};
            }

            // Apply the mangle
            current_tile = mangle_result->first;
            result.mangle_records.insert(mangle_result->second);

            // Emit a remark about the mangle
            const auto [pixel_row, pixel_col] = tile::index_to_row_col(mangle_result->second.pixel_index);
            std::vector<std::string> remark_lines;
            remark_lines.push_back(diag_->formatter().format(
                "Mangled tile {} in animation '{}': pixel ({},{}) changed from index {} to {}.",
                FormatParam{i, Style::bold},
                FormatParam{anim_name, Style::bold},
                FormatParam{pixel_row},
                FormatParam{pixel_col},
                FormatParam{mangle_result->second.original_pixel.index()},
                FormatParam{mangle_result->second.mangled_pixel.index()}));

            const PixelTile<Rgba32> original_rgba =
                color_tile_from_index_tile(original_tile, palette, extrinsic_transparency);
            const PixelTile<Rgba32> mangled_rgba =
                color_tile_from_index_tile(current_tile, palette, extrinsic_transparency);

            remark_lines.emplace_back("");
            remark_lines.emplace_back("Original tile:");
            std::ranges::copy(
                tile_printer_->print_tile(original_rgba, extrinsic_transparency), std::back_inserter(remark_lines));

            remark_lines.emplace_back("Mangled tile:");
            std::ranges::copy(
                tile_printer_->print_tile(mangled_rgba, extrinsic_transparency), std::back_inserter(remark_lines));

            diag_->remark("anim-key-frame-mangle", remark_lines);
        }

        // Re-canonicalize after potential mangle and add to tracking sets
        CanonicalPixelTile final_canonical{current_tile};
        const PixelTile<IndexPixel> &final_base = final_canonical;
        canonical_first_occurrence.emplace(final_base, i);
        all_canonical_tiles.insert(final_base);
    }

    return result;
}

} // namespace porytiles2
