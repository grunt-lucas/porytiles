#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"

#include <deque>
#include <iostream>
#include <set>

#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

#include <algorithm>

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
 * @brief Finds the best palette for a tile, excluding forbidden palettes.
 *
 * @return Index of the best palette, or nullopt if none available
 */
[[nodiscard]] std::optional<std::size_t> find_best_palette_excluding_forbidden(
    const TileInfo &info,
    const std::vector<PackedPalette> &palettes,
    const std::map<std::size_t, std::size_t> &multiplicity)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        // Skip forbidden palettes
        if (info.forbidden_palettes.contains(i)) {
            continue;
        }

        if (const double cost = compute_relative_size(info.tile.color_set(), multiplicity); cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }

    // If best cost equals tile size (no overlap benefit), return nullopt to create new palette
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
        output.pals_.emplace_back(0, input.pal_capacity_);
    }

    // Build multiplicity map
    auto multiplicity = build_multiplicity_map(input.tiles_, input.hints_);

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

    // Main loop: process tiles from pool
    std::cerr << "Main loop: process tiles from pool" << std::endl;
    while (!tile_pool.empty()) {
        std::cerr << "Main loop top - tile_pool size: " << tile_pool.size() << std::endl;
        TileInfo tile_info = std::move(tile_pool.front());
        tile_pool.pop_front();

        // Find best palette excluding forbidden ones
        auto maybe_best_idx = find_best_palette_excluding_forbidden(tile_info, output.pals_, multiplicity);

        if (!maybe_best_idx.has_value()) {
            std::cerr << "No best index found - trying to create new pal" << std::endl;
            // Create new palette if possible
            if (pal_pool.has_available_pal()) {
                std::cerr << "New pal created!" << std::endl;
                output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
                output.pals_.back().add_tile(tile_info.tile);
                output.tile_to_pal_[tile_info.tile.id()] = output.pals_.back().hardware_index();
            }
            else {
                // No room - try first-fit as fallback
                std::cerr << "All pals created - using first fit fallback" << std::endl;
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
        std::cerr << "Trying to remove worst fitting tiles" << std::endl;
        while (best_palette.color_count() > input.pal_capacity_) {
            const auto &assigned_ids = best_palette.assigned_tile_ids();
            if (assigned_ids.size() <= 1) {
                /*
                 * TODO: is this a panic state? How would this ever happen? we already validated that no tile has more
                 * than pal_capacity_ unique colors
                 */
                break; // Can't remove the only tile
            }

            // Find tile with minimum efficiency
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
                    // TODO: is this a panic condition? this map should contain all tiles right?
                    continue;
                }

                double eff = compute_efficiency(it->second, multiplicity);
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
            std::cerr << "Found worst tile - forbidding it" << std::endl;
            best_palette.remove_tile(worst_tile_id);
            output.tile_to_pal_.erase(worst_tile_id);

            if (const auto colors_it = tile_colors_map.find(worst_tile_id); colors_it != tile_colors_map.end()) {
                TileInfo removed_info{PackableTile{worst_tile_id, colors_it->second}};
                removed_info.forbidden_palettes.insert(best_idx);
                std::cerr << "Pushed worst tile back into pool - " << to_string(worst_tile_id) << std::endl;
                tile_pool.push_back(std::move(removed_info));
            }
        }
    }

    // Final cleanup: remove tiles from any remaining overloaded palettes
    std::cerr << "Final cleanup: remove tiles from any remaining overloaded palettes" << std::endl;
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
    std::cerr << "First-Fit pass for remaining tiles - remaining_tile_pool: " << remaining_tile_pool.size()
              << std::endl;
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
                output.pals_.emplace_back(output.pals_.size(), input.pal_capacity_);
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
