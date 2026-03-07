#include "porytiles2/domain/services/anim_key_frame_mangler.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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

/*
 * Pixel priority order for mangling (least visually impactful first)
 * Corners: (0,0), (0,7), (7,0), (7,7) → indices 0, 7, 56, 63
 * Corners: (0,0), (0,7), (7,0), (7,7) → indices 0, 7, 56, 63
 * Top edge: 1-6
 * Left edge: 8, 16, 24, 32, 40, 48
 * Right edge: 15, 23, 31, 39, 47, 55
 * Bottom edge: 57-62
 * Interior: remaining pixels
 */
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
 * @brief Finds all alternative colors in the palette, sorted by RGB distance (ascending).
 *
 * @details
 * Searches through all palette colors (indices 1-15) excluding the current color and returns all alternative color
 * indices sorted by squared RGB distance from the current color. Ties in distance are broken by index (ascending) for
 * deterministic ordering. This allows the mangler to try multiple alternatives at each pixel position, greatly
 * expanding the mangle search space for symmetric tiles like solid-color tiles.
 *
 * @param current_color_index The current color index to find alternatives for
 * @param palette The palette to look up actual RGB values
 * @return A vector of alternative color indices sorted by distance (closest first), empty if no alternatives exist
 */
[[nodiscard]] std::vector<std::size_t>
find_alternative_colors_sorted(std::size_t current_color_index, const Palette<Rgba32, pal::max_size> &palette)
{
    struct ColorCandidate {
        std::size_t index;
        int distance;
    };

    const Rgba32 current_color = palette.at(current_color_index);
    std::vector<ColorCandidate> candidates;

    // Search ALL palette colors (1-15), not just those in the tile
    for (std::size_t candidate_index = 1; candidate_index < pal::max_size; ++candidate_index) {
        if (candidate_index == current_color_index) {
            continue;
        }

        const Rgba32 candidate_color = palette.at(candidate_index);
        const int distance = color_distance_squared(current_color, candidate_color);
        candidates.push_back({candidate_index, distance});
    }

    std::ranges::sort(candidates, [](const ColorCandidate &a, const ColorCandidate &b) {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        return a.index < b.index;
    });

    std::vector<std::size_t> result;
    result.reserve(candidates.size());
    for (const auto &c : candidates) {
        result.push_back(c.index);
    }
    return result;
}

/**
 * @brief Constructs a mangled IndexPixel preserving the palette index (upper 4 bits).
 *
 * @param original_palette_index The palette index from the original pixel
 * @param alt_color The new color index to swap to
 * @return The constructed IndexPixel
 */
[[nodiscard]] IndexPixel make_mangled_pixel(std::size_t original_palette_index, std::size_t alt_color)
{
    return IndexPixel{(original_palette_index << 4) | alt_color};
}

/**
 * @brief Attempts to mangle a single tile to make it unique.
 *
 * @details
 * Uses a two-phase approach to find a unique mangle:
 *
 * Phase 1 (single-pixel): Tries each pixel in priority order, attempting all alternative palette colors in order of
 * visual similarity (closest first). Returns the first successful single-pixel modification.
 *
 * Phase 2 (two-pixel fallback): If all single-pixel swaps produce collisions (common with solid-color tiles that have
 * many duplicates), tries swapping two pixels simultaneously. The first pixel is chosen at the least visible position,
 * paired with each subsequent pixel position, exhausting all color combinations at each pair.
 *
 * Uniqueness is checked against canonical forms to ensure tiles that are flip-equivalent are also treated as
 * duplicates.
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
    // Phase 1: single-pixel swaps (preferred — minimal visual impact)
    for (std::size_t pixel_index : pixel_priority_order) {
        const IndexPixel original_pixel = tile.at(pixel_index);

        /*
         * TODO: we used to have this check here, since it doesn't really make sense to mangle a transparent pixel.
         * However, FireRed General animations have unused transparent tiles in their key frames, so if we leave this
         * check here the mangling breaks for those tiles. We might want to consider handling this differently in the
         * future. For now, allowing mangling of transparent pixels works to solve this edge case.
         */
        // if (original_pixel.is_transparent()) {
        //     continue;
        // }

        const std::vector<std::size_t> alternatives =
            find_alternative_colors_sorted(original_pixel.color_index(), palette);

        for (const std::size_t alt_color : alternatives) {
            const IndexPixel mangled_pixel = make_mangled_pixel(original_pixel.palette_index(), alt_color);

            PixelTile<IndexPixel> candidate_tile = tile;
            candidate_tile.set(pixel_index, mangled_pixel);

            CanonicalPixelTile<IndexPixel> canonical_candidate{candidate_tile};
            const PixelTile<IndexPixel> &candidate_base = canonical_candidate;
            if (!all_existing_canonical_tiles.contains(candidate_base)) {
                TileMangleRecord record{
                    .tile_index = tile_index,
                    .pixel_changes = {PixelMangleChange{
                        .pixel_index = pixel_index, .original_pixel = original_pixel, .mangled_pixel = mangled_pixel}}};
                return std::make_pair(candidate_tile, record);
            }
        }
    }

    /*
     * Phase 2: two-pixel swaps (fallback for heavily saturated canonical tile sets).
     * Loop ordering: position pair (p1, p2) outermost, then color alternatives innermost. This ensures we prefer the
     * least visible pixel positions before trying more visible ones, consistent with the Phase 1 priority ordering.
     */
    for (std::size_t p1_idx = 0; p1_idx < pixel_priority_order.size(); ++p1_idx) {
        const std::size_t p1 = pixel_priority_order[p1_idx];
        const IndexPixel p1_original = tile.at(p1);

        const std::vector<std::size_t> p1_alternatives =
            find_alternative_colors_sorted(p1_original.color_index(), palette);

        for (std::size_t p2_idx = p1_idx + 1; p2_idx < pixel_priority_order.size(); ++p2_idx) {
            const std::size_t p2 = pixel_priority_order[p2_idx];
            const IndexPixel p2_original = tile.at(p2);

            const std::vector<std::size_t> p2_alternatives =
                find_alternative_colors_sorted(p2_original.color_index(), palette);

            for (const std::size_t p1_alt : p1_alternatives) {
                const IndexPixel p1_mangled = make_mangled_pixel(p1_original.palette_index(), p1_alt);

                for (const std::size_t p2_alt : p2_alternatives) {
                    const IndexPixel p2_mangled = make_mangled_pixel(p2_original.palette_index(), p2_alt);

                    PixelTile<IndexPixel> candidate_tile = tile;
                    candidate_tile.set(p1, p1_mangled);
                    candidate_tile.set(p2, p2_mangled);

                    CanonicalPixelTile<IndexPixel> canonical_candidate{candidate_tile};
                    const PixelTile<IndexPixel> &candidate_base = canonical_candidate;
                    if (!all_existing_canonical_tiles.contains(candidate_base)) {
                        TileMangleRecord record{
                            .tile_index = tile_index,
                            .pixel_changes = {
                                PixelMangleChange{
                                    .pixel_index = p1, .original_pixel = p1_original, .mangled_pixel = p1_mangled},
                                PixelMangleChange{
                                    .pixel_index = p2, .original_pixel = p2_original, .mangled_pixel = p2_mangled}}};
                        return std::make_pair(candidate_tile, record);
                    }
                }
            }
        }
    }

    // No valid mangle found
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
    const std::vector<const Palette<Rgba32, pal::max_size> *> &palettes,
    const Rgba32 &extrinsic_transparency,
    const std::set<PixelTile<IndexPixel>> &existing_canonical_tiles) const
{
    if (palettes.size() != tiles.size()) {
        panic("palettes size " + std::to_string(palettes.size()) + " != tiles size " + std::to_string(tiles.size()));
    }

    MangleResult result;
    result.tiles = std::move(tiles);

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
                try_mangle_tile(current_tile, i, *palettes[i], all_canonical_tiles);

            if (!mangle_result.has_value()) {
                // Could not mangle the tile - this is an error
                std::vector<std::string> err_msg;
                err_msg.push_back(diag_->formatter().format(
                    "Failed to mangle duplicate key frame tile {} in animation '{}'.",
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
            std::vector<std::string> remark_lines;
            for (const auto &change : mangle_result->second.pixel_changes) {
                const auto [pixel_row, pixel_col] = tile::index_to_row_col(change.pixel_index);
                remark_lines.push_back(diag_->formatter().format(
                    "Mangled tile {} in animation '{}': pixel ({},{}) changed from index {} to {}.",
                    FormatParam{i, Style::bold},
                    FormatParam{anim_name, Style::bold},
                    FormatParam{pixel_row},
                    FormatParam{pixel_col},
                    FormatParam{change.original_pixel.index()},
                    FormatParam{change.mangled_pixel.index()}));
            }

            const PixelTile<Rgba32> original_rgba =
                color_tile_from_index_tile(original_tile, *palettes[i], extrinsic_transparency);
            const PixelTile<Rgba32> mangled_rgba =
                color_tile_from_index_tile(current_tile, *palettes[i], extrinsic_transparency);

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
