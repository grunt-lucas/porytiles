#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"

#include <ranges>

namespace porytiles2 {

// ============================================================================
// GLOBAL MULTIPLICITY FUNCTIONS
// ============================================================================

std::map<std::size_t, std::size_t>
build_global_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints)
{
    std::map<std::size_t, std::size_t> multiplicity;

    auto count_colors = [&multiplicity](const std::vector<PackableTile> &tile_list) {
        for (const auto &tile : tile_list) {
            for_each_color(tile.color_set(), [&multiplicity](std::size_t color_idx) { multiplicity[color_idx]++; });
        }
    };

    count_colors(tiles);
    count_colors(hints);

    return multiplicity;
}

double compute_average_multiplicity(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints)
{
    auto global_mult = build_global_multiplicity_map(tiles, hints);

    if (global_mult.empty()) {
        return 0.0;
    }

    // Card(T) = sum of all multiplicities = sum of tile sizes
    std::size_t cardinality = 0;
    for (const auto &count : global_mult | std::views::values) {
        cardinality += count;
    }

    // |A| = number of unique colors
    const std::size_t num_colors = global_mult.size();

    // Average multiplicity = Card(T) / |A|
    return static_cast<double>(cardinality) / static_cast<double>(num_colors);
}

// ============================================================================
// PALETTE-LOCAL MULTIPLICITY FUNCTIONS
// ============================================================================

std::map<std::size_t, std::size_t> build_palette_local_multiplicity(
    const PackedPalette &palette, const std::map<PackableTile::Id, ColorSet> &tile_colors_map)
{
    std::map<std::size_t, std::size_t> local_mult;

    for (const auto &tile_id : palette.assigned_tile_ids()) {
        if (auto it = tile_colors_map.find(tile_id); it != tile_colors_map.end()) {
            for_each_color(it->second, [&local_mult](std::size_t color_idx) { local_mult[color_idx]++; });
        }
    }

    return local_mult;
}

double compute_weighted_cost_in_palette(
    const ColorSet &tile_colors,
    const PackedPalette &palette,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map)
{
    auto local_mult = build_palette_local_multiplicity(palette, tile_colors_map);

    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &local_mult](std::size_t color_idx) {
        std::size_t count = 0;
        if (const auto it = local_mult.find(color_idx); it != local_mult.end()) {
            count = it->second;
        }
        weighted_cost += 1.0 / static_cast<double>(1 + count);
    });

    return weighted_cost;
}

double
compute_palette_local_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &local_mult)
{
    const std::size_t color_count = color_set_count(tile_colors);
    if (color_count == 0) {
        return 1.0;
    }

    // Compute weighted cost using palette-local multiplicity
    // Note: Here we use count directly (not 1+count) because the tile IS in the palette,
    // so its colors are already counted in local_mult
    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &local_mult](std::size_t color_idx) {
        std::size_t count = 1; // Default to 1 if color not found (this tile is the only one with it)
        if (const auto it = local_mult.find(color_idx); it != local_mult.end()) {
            count = it->second;
        }
        weighted_cost += 1.0 / static_cast<double>(count);
    });

    return 1.0 - (weighted_cost / static_cast<double>(color_count));
}

// ============================================================================
// FAST METRIC FUNCTIONS (using PackedPalette's cached color counts)
// ============================================================================

double compute_weighted_cost_in_palette_fast(const ColorSet &tile_colors, const PackedPalette &palette)
{
    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &palette](std::size_t color_idx) {
        const std::size_t count = palette.color_multiplicity(color_idx);
        weighted_cost += 1.0 / static_cast<double>(1 + count);
    });
    return weighted_cost;
}

double compute_palette_local_efficiency_fast(const ColorSet &tile_colors, const PackedPalette &palette)
{
    const std::size_t color_count = color_set_count(tile_colors);
    if (color_count == 0) {
        return 1.0;
    }

    // Compute weighted cost using palette's cached color counts
    // Note: Here we use count directly (not 1+count) because the tile IS in the palette,
    // so its colors are already counted in the palette's color_counts
    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &palette](std::size_t color_idx) {
        const std::size_t count = std::max(palette.color_multiplicity(color_idx), std::size_t{1});
        weighted_cost += 1.0 / static_cast<double>(count);
    });

    return 1.0 - (weighted_cost / static_cast<double>(color_count));
}

} // namespace porytiles2
