#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <set>

#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Extended tile info for the overload-and-remove algorithm.
 */
struct TileInfo {
    PackableTile tile;
    std::set<std::size_t> forbidden_palettes;

    explicit TileInfo(PackableTile t) : tile{std::move(t)}, forbidden_palettes{} {}
};

/**
 * @brief Builds a palette-local multiplicity map (count of tiles containing each color).
 *
 * @details
 * For each color, counts how many tiles currently in the palette contain that color.
 * This is used for the weighted cost computation.
 *
 * @param palette The palette to analyze
 * @param tile_colors_map Map from tile ID to ColorSet for all tiles
 * @return Map from color index to count of tiles containing that color
 */
[[nodiscard]] std::map<std::size_t, std::size_t> build_palette_local_multiplicity(
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

/**
 * @brief Computes the weighted cost of placing a tile in a specific palette.
 *
 * @details
 * Uses palette-local multiplicity: for each color in the tile, compute
 * 1 / (1 + count_of_tiles_in_palette_with_this_color). Sum these values.
 * Lower values indicate better overlap with existing palette colors.
 *
 * @param tile_colors The colors of the tile to place
 * @param palette The candidate palette
 * @param tile_colors_map Map from tile ID to ColorSet for all tiles
 * @return The weighted cost (lower is better)
 */
[[nodiscard]] double compute_weighted_cost_in_palette(
    const ColorSet &tile_colors,
    const PackedPalette &palette,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map)
{
    auto local_mult = build_palette_local_multiplicity(palette, tile_colors_map);

    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &local_mult](std::size_t color_idx) {
        std::size_t count = 0;
        if (auto it = local_mult.find(color_idx); it != local_mult.end()) {
            count = it->second;
        }
        weighted_cost += 1.0 / static_cast<double>(1 + count);
    });

    return weighted_cost;
}

/**
 * @brief Computes the palette-local efficiency of a tile within its palette.
 *
 * @details
 * Efficiency measures how well a tile's colors are shared with other tiles in the same palette.
 * - 0 = no sharing (all colors unique to this tile)
 * - 1 = perfect sharing (all colors shared with many other tiles)
 *
 * This is used to identify the "worst fitting" tile when a palette is overloaded.
 *
 * @param tile_colors The colors of the tile
 * @param local_mult The palette-local multiplicity map
 * @return Efficiency value in [0, 1]
 */
[[nodiscard]] double
compute_palette_local_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &local_mult)
{
    std::size_t color_count = color_set_count(tile_colors);
    if (color_count == 0) {
        return 1.0;
    }

    // Compute weighted cost using palette-local multiplicity
    double weighted_cost = 0.0;
    for_each_color(tile_colors, [&weighted_cost, &local_mult](std::size_t color_idx) {
        std::size_t count = 1; // Default to 1 if color not found (this tile is the only one with it)
        if (auto it = local_mult.find(color_idx); it != local_mult.end()) {
            count = it->second;
        }
        weighted_cost += 1.0 / static_cast<double>(count);
    });

    return 1.0 - (weighted_cost / static_cast<double>(color_count));
}

/**
 * @brief Finds the best palette for a tile, excluding forbidden palettes.
 *
 * @details
 * Uses palette-local weighted cost to find the best-fitting palette. The weighted cost
 * measures how well the tile's colors overlap with colors already in the palette.
 * Lower cost means better overlap.
 *
 * @return Index of the best palette, or nullopt if none available or no palette offers overlap benefit
 */
[[nodiscard]] std::optional<std::size_t> find_best_palette_excluding_forbidden(
    const TileInfo &info,
    const std::vector<PackedPalette> &palettes,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        // Skip forbidden palettes
        if (info.forbidden_palettes.contains(i)) {
            continue;
        }

        double cost = compute_weighted_cost_in_palette(info.tile.color_set(), palettes[i], tile_colors_map);
        if (cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }

    // If best cost equals tile size (no overlap benefit), return nullopt to create new palette
    // This happens when all colors in the tile are new to the palette (each color contributes 1.0)
    if (best_idx.has_value() && best_cost >= static_cast<double>(info.tile.color_count())) {
        return std::nullopt;
    }

    return best_idx;
}

} // namespace

namespace porytiles2 {

ChainableResult<PackingOutput> OverloadAndRemoveStrategy::pack(const PackingInput &input) const
{
    PackingOutput output;
    PalettePool pal_pool = input.pal_pool_;

    // Initialize output palettes from prefilled palettes
    output.pals_ = initialize_packed_palettes(input.prefilled_pals_, pal_pool, input.pal_capacity_);

    // Ensure we have at least one palette
    if (output.pals_.empty()) {
        if (!pal_pool.has_available_pal()) {
            return FormattableError{"Overload-And-Remove: no palettes available in pool"};
        }
        output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
    }

    // Build tile pool (hints first, then regular tiles)
    std::deque<TileInfo> tile_pool;
    for (const auto &hint : input.hints_) {
        tile_pool.emplace_back(hint);
    }
    for (const auto &tile : input.tiles_) {
        tile_pool.emplace_back(tile);
    }

    if (tile_pool.empty()) {
        return output;
    }

    // Presort the tile pool so tiles with larger color counts come first (First Fit Decreasing heuristic)
    std::ranges::sort(
        tile_pool, [](const TileInfo &a, const TileInfo &b) { return a.tile.color_count() > b.tile.color_count(); });

    // Pop first tile and assign to first available palette
    TileInfo first_tile_info = std::move(tile_pool.front());
    tile_pool.pop_front();

    /*
     * Find or create a palette for the first tile. First, we try searching through the current state output pals. If we
     * find one, use it! If we don't find one, then try checking a new one out from our PalettePool if one is available.
     * If there is no pal available, fail.
     */
    bool first_assigned = false;
    for (std::size_t i = 0; i < output.pals_.size(); ++i) {
        if (output.pals_[i].can_fit(first_tile_info.tile.color_set())) {
            output.pals_[i].add_tile(first_tile_info.tile);
            output.tile_to_pal_[first_tile_info.tile.id()] = output.pals_[i].hardware_index();
            first_assigned = true;
            break;
        }
    }
    if (!first_assigned) {
        if (pal_pool.has_available_pal()) {
            output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
            output.pals_.back().add_tile(first_tile_info.tile);
            output.tile_to_pal_[first_tile_info.tile.id()] = output.pals_.back().hardware_index();
        }
        else {
            return FormattableError{"Overload-And-Remove: first tile cannot fit in any palette"};
        }
    }

    // We need to find the original tile data to compute efficiency
    // Build a map of tile_id -> ColorSet from all tiles
    std::map<PackableTile::Id, ColorSet> tile_colors_map;
    for (const auto &hint : input.hints_) {
        tile_colors_map[hint.id()] = hint.color_set();
    }
    for (const auto &tile : input.tiles_) {
        tile_colors_map[tile.id()] = tile.color_set();
    }
    // Add prefilled palette system tiles to the color map
    // This ensures palette-local cost computation accounts for prefilled colors
    for (const auto &prefilled_pal : input.prefilled_pals_) {
        PackableTile::Id system_id = PackableTile::PrefilledPaletteId{prefilled_pal.hardware_index()};
        tile_colors_map[system_id] = prefilled_pal.fixed_colors();
    }

    // Track forbidden palettes for each tile across removal cycles
    // This ensures termination: a tile can never return to a palette it was removed from
    std::map<PackableTile::Id, std::set<std::size_t>> forbidden_map;

    // Main loop: process tiles from pool
    while (!tile_pool.empty()) {
        TileInfo tile_info = std::move(tile_pool.front());
        tile_pool.pop_front();

        // Find best palette excluding forbidden ones (using palette-local weighted cost)
        auto maybe_best_idx = find_best_palette_excluding_forbidden(tile_info, output.pals_, tile_colors_map);

        if (!maybe_best_idx.has_value()) {
            // Create new palette if possible
            if (pal_pool.has_available_pal()) {
                output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
                output.pals_.back().add_tile(tile_info.tile);
                output.tile_to_pal_[tile_info.tile.id()] = output.pals_.back().hardware_index();
            }
            else {
                // No room - try first-fit as fallback
                bool assigned = false;
                for (std::size_t i = 0; i < output.pals_.size(); ++i) {
                    if (!tile_info.forbidden_palettes.contains(i) &&
                        output.pals_[i].can_fit(tile_info.tile.color_set())) {
                        output.pals_[i].add_tile(tile_info.tile);
                        output.tile_to_pal_[tile_info.tile.id()] = output.pals_[i].hardware_index();
                        assigned = true;
                        break;
                    }
                }
                if (!assigned) {
                    return FormattableError{
                        "Overload-and-Remove: cannot assign tile - no palette has room - " +
                        to_string(tile_info.tile.id())};
                }
            }
            continue;
        }

        auto best_idx = maybe_best_idx.value();

        // Add tile to best palette (may cause overload)
        auto &best_palette = output.pals_[best_idx];
        best_palette.add_tile(tile_info.tile);
        output.tile_to_pal_[tile_info.tile.id()] = best_palette.hardware_index();

        // Handle overload by removing worst-fitting tiles
        while (best_palette.color_count() > input.pal_capacity_) {
            const auto &assigned_ids = best_palette.assigned_tile_ids();
            if (assigned_ids.size() <= 1) {
                break; // Can't remove the only tile
            }

            // Build palette-local multiplicity for efficiency computation
            auto local_mult = build_palette_local_multiplicity(best_palette, tile_colors_map);

            // Find tile with minimum efficiency (using palette-local weights)
            double min_efficiency = std::numeric_limits<double>::max();
            double max_efficiency = std::numeric_limits<double>::lowest();

            PackableTile::Id worst_tile_id = assigned_ids.front();
            for (PackableTile::Id tid : assigned_ids) {
                // Skip system tiles from fixed palettes -- these cannot be changed
                if (std::holds_alternative<PackableTile::PrefilledPaletteId>(tid)) {
                    continue;
                }

                const auto it = tile_colors_map.find(tid);
                if (it == tile_colors_map.end()) {
                    continue;
                }

                double eff = compute_palette_local_efficiency(it->second, local_mult);
                if (eff < min_efficiency) {
                    min_efficiency = eff;
                    worst_tile_id = tid;
                }
                if (eff > max_efficiency) {
                    max_efficiency = eff;
                }
            }

            // If all tiles have same efficiency, we can't make progress
            if (std::abs(min_efficiency - max_efficiency) < 1e-9) {
                break;
            }

            // Remove worst tile and re-add to pool with forbidden marker
            best_palette.remove_tile(worst_tile_id);
            output.tile_to_pal_.erase(worst_tile_id);

            // Record this palette as forbidden for this tile (persists across removal cycles)
            forbidden_map[worst_tile_id].insert(best_idx);

            if (const auto colors_it = tile_colors_map.find(worst_tile_id); colors_it != tile_colors_map.end()) {
                TileInfo removed_info{PackableTile{worst_tile_id, colors_it->second}};
                // Restore ALL accumulated forbidden palettes for this tile
                removed_info.forbidden_palettes = forbidden_map[worst_tile_id];
                tile_pool.push_back(std::move(removed_info));
            }
        }
    }

    // Final cleanup: remove tiles from any remaining overloaded palettes
    std::vector<TileInfo> remaining_tile_pool{};
    for (auto &pal : output.pals_) {
        while (pal.color_count() > input.pal_capacity_ && !pal.assigned_tile_ids().empty()) {
            PackableTile::Id tid = pal.assigned_tile_ids().back();
            // Skip system tiles from fixed palettes -- these cannot be changed
            if (std::holds_alternative<PackableTile::PrefilledPaletteId>(tid)) {
                continue;
            }

            pal.remove_tile(tid);
            output.tile_to_pal_.erase(tid);

            // Find the tile's colors
            for (const auto &hint : input.hints_) {
                if (hint.id() == tid) {
                    remaining_tile_pool.emplace_back(hint);
                    break;
                }
            }
            for (const auto &tile : input.tiles_) {
                if (tile.id() == tid) {
                    remaining_tile_pool.emplace_back(tile);
                    break;
                }
            }
        }
    }

    // First-Fit pass for remaining tiles
    for (auto &tile_info : remaining_tile_pool) {
        bool assigned = false;
        for (std::size_t i = 0; i < output.pals_.size(); ++i) {
            if (output.pals_[i].can_fit(tile_info.tile.color_set())) {
                output.pals_[i].add_tile(tile_info.tile);
                output.tile_to_pal_[tile_info.tile.id()] = output.pals_[i].hardware_index();
                assigned = true;
                break;
            }
        }
        if (!assigned) {
            if (pal_pool.has_available_pal()) {
                output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
                output.pals_.back().add_tile(tile_info.tile);
                output.tile_to_pal_[tile_info.tile.id()] = output.pals_.back().hardware_index();
            }
            else {
                return FormattableError{
                    "Overload-and-Remove: cannot assign tile in final pass - " + to_string(tile_info.tile.id())};
            }
        }
    }

    return output;
}

} // namespace porytiles2
