#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <utility>

#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"
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
 * @brief Finds the best palette for a tile, excluding forbidden palettes.
 *
 * @details
 * Uses cached palette color counts to compute weighted cost efficiently. The weighted cost
 * measures how well the tile's colors overlap with colors already in the palette.
 * Lower cost means better overlap.
 *
 * @param info The tile info with forbidden palette set
 * @param palettes The current set of packed palettes
 * @param force_assignment When true, returns the lowest-cost non-forbidden palette even if no overlap benefit exists.
 *     When false (default), returns nullopt if no palette offers overlap benefit, signaling the caller to create a new
 *     palette.
 * @return Index of the best palette, or nullopt if none available (or no overlap benefit when not forcing)
 */
[[nodiscard]] std::optional<std::size_t> find_best_palette_excluding_forbidden(
    const TileInfo &info, const std::vector<PackedPalette> &palettes, bool force_assignment = false)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        // Skip forbidden palettes
        if (info.forbidden_palettes.contains(i)) {
            continue;
        }

        // Use fast metric function with cached color counts - O(colors) instead of O(tiles × colors)
        double cost = compute_weighted_cost_in_palette_fast(info.tile.color_set(), palettes[i]);
        if (cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }

    // If best cost equals tile size (no overlap benefit), return nullopt to create new palette.
    // This happens when all colors in the tile are new to the palette (each color contributes 1.0).
    // Skip this check when force_assignment is true — the caller wants a palette regardless.
    if (!force_assignment && best_idx.has_value() && best_cost >= static_cast<double>(info.tile.color_count())) {
        return std::nullopt;
    }

    return best_idx;
}

} // namespace

namespace porytiles2 {

ChainableResult<PackingOutput> OverloadAndRemoveStrategy::pack(const PackingInput &input) const
{
    // First attempt: FFD ordering (deterministic, theoretically best for bin packing)
    auto first_result = try_pack(input, std::nullopt);
    if (first_result.has_value() || shuffle_strategy_ == ShuffleStrategy::single_ffd || max_attempts_ <= 1) {
        return first_result;
    }

    // Subsequent attempts: orderings determined by shuffle_strategy_ with seeded PRNG
    std::mt19937_64 seed_generator{seed_};
    for (std::size_t attempt = 1; attempt < max_attempts_; ++attempt) {
        std::uint64_t shuffle_seed = seed_generator();
        auto result = try_pack(input, shuffle_seed);
        if (result.has_value()) {
            return result;
        }
    }

    // All attempts failed — return the first attempt's error (most informative)
    return std::move(first_result);
}

ChainableResult<PackingOutput>
OverloadAndRemoveStrategy::try_pack(const PackingInput &input, std::optional<std::uint64_t> shuffle_seed) const
{
    PackingOutput output;
    PalettePool pal_pool = input.pal_pool_;

    // Initialize output palettes from prefilled palettes
    output.pals_ = initialize_packed_palettes(input.prefilled_pals_, pal_pool, input.pal_capacity_);

    // Ensure we have at least one palette
    if (output.pals_.empty()) {
        if (!pal_pool.has_available_pal()) {
            return FormattableError{"Overload-And-Remove: no palettes available in pool."};
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

    /*
     * Right now, we mix together the hints and regular tiles before sorting. Do we want this? I think it's probably ok,
     * since hints still guarantee that colors in the same hint will be in the same palette. And if the user supplied
     * hints that are larger than any individual tile, they'll go first as expected. However, I think it makes sense to
     * allow regular tiles that are large to go before smaller hints, since this probably helps to find an optimal
     * result -- that larger tile *has to* get put somewhere in order for a solution to be found. No sense placing the
     * hint first, only to block ourselves from finding a possible solution down the line. In other words, the promised
     * hint precondition is not violated, and we potentially get a better solution.
     */

    // Order tiles based on shuffle strategy
    if (shuffle_seed.has_value()) {
        std::mt19937_64 rng{shuffle_seed.value()};
        std::ranges::shuffle(tile_pool, rng);
        if (shuffle_strategy_ == ShuffleStrategy::noisy_ffd) {
            // Noisy FFD: shuffle first for random tiebreaking, then stable_sort by color_count descending.
            // This preserves the large-first FFD property while randomly reordering tiles of equal size.
            std::ranges::stable_sort(tile_pool, [](const TileInfo &a, const TileInfo &b) {
                return a.tile.color_count() > b.tile.color_count();
            });
        }
        // ShuffleStrategy::random: just the shuffle above (original behavior)
    }
    else {
        // FFD: sort by color count descending (deterministic first attempt for all strategies)
        std::ranges::stable_sort(tile_pool, [](const TileInfo &a, const TileInfo &b) {
            return a.tile.color_count() > b.tile.color_count();
        });
    }

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
            return FormattableError{"Overload-And-Remove: first tile cannot fit in any palette."};
        }
    }

    // Build a map of tile_id -> ColorSet from all input tiles.
    // Note: This is only needed for recreating TileInfo when tiles are removed from palettes.
    // Palette-local cost computation now uses cached color counts in PackedPalette.
    std::map<PackableTile::Id, ColorSet> tile_colors_map;
    for (const auto &hint : input.hints_) {
        tile_colors_map[hint.id()] = hint.color_set();
    }
    for (const auto &tile : input.tiles_) {
        tile_colors_map[tile.id()] = tile.color_set();
    }

    // Track forbidden palettes for each tile across removal cycles
    // This ensures termination: a tile can never return to a palette it was removed from
    std::map<PackableTile::Id, std::set<std::size_t>> forbidden_map;

    // Main loop: process tiles from pool
    while (!tile_pool.empty()) {
        TileInfo tile_info = std::move(tile_pool.front());
        tile_pool.pop_front();

        // Find best palette excluding forbidden ones (using cached palette color counts)
        auto maybe_best_idx = find_best_palette_excluding_forbidden(tile_info, output.pals_);

        if (!maybe_best_idx.has_value()) {
            // Create new palette if possible
            if (pal_pool.has_available_pal()) {
                output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
                output.pals_.back().add_tile(tile_info.tile);
                output.tile_to_pal_[tile_info.tile.id()] = output.pals_.back().hardware_index();
                continue;
            }

            /*
             * Pool exhausted and no palette offers overlap benefit. Try two fallback strategies:
             *
             * 1. First-fit with can_fit: fast, no cascading removals. Succeeds when a palette has physical room.
             * 2. Force-assignment with overload/remove: slower but more capable. Allows the overload/remove mechanism
             *    to redistribute tiles. Termination guaranteed because forbidden sets grow monotonically.
             */

            // Fallback 1: strict first-fit (no overload)
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
            if (assigned) {
                continue;
            }

            // Fallback 2: force-assign to least-bad palette, let overload/remove handle it
            maybe_best_idx = find_best_palette_excluding_forbidden(tile_info, output.pals_, true);
            if (!maybe_best_idx.has_value()) {
                return FormattableError{
                    "Overload-and-Remove: cannot assign tile - all palettes forbidden - " +
                    to_string(tile_info.tile.id())};
            }
            // Fall through to add_tile + overload/remove loop below
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

            // Find tile with minimum efficiency using fast O(colors) computation
            // Uses cached color counts in PackedPalette instead of rebuilding multiplicity map
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

                // Use fast efficiency function with cached color counts
                double eff = compute_palette_local_efficiency_fast(it->second, best_palette);
                if (eff < min_efficiency) {
                    min_efficiency = eff;
                    worst_tile_id = tid;
                }
                if (eff > max_efficiency) {
                    max_efficiency = eff;
                }
            }

            // If all tiles have same efficiency, use tiebreakers instead of giving up
            if (std::abs(min_efficiency - max_efficiency) < 1e-9) {
                // Primary tiebreaker: remove tile with most colors (frees most palette capacity)
                // Secondary tiebreaker: among equal color counts, remove most recently added (LIFO)
                std::optional<std::size_t> best_removal_pos;
                std::size_t best_color_count = 0;

                for (std::size_t pos = 0; pos < assigned_ids.size(); ++pos) {
                    const auto &tid = assigned_ids[pos];
                    // Never remove prefilled palette tiles
                    if (std::holds_alternative<PackableTile::PrefilledPaletteId>(tid)) {
                        continue;
                    }
                    const auto it = tile_colors_map.find(tid);
                    if (it == tile_colors_map.end()) {
                        continue;
                    }

                    std::size_t cc = color_set_count(it->second);
                    // Prefer higher color count (frees more capacity), then later position (LIFO)
                    if (!best_removal_pos.has_value() || cc > best_color_count ||
                        (cc == best_color_count && pos > best_removal_pos.value())) {
                        best_removal_pos = pos;
                        best_color_count = cc;
                    }
                }

                // If no removable tile found (only prefilled tiles), truly stuck
                if (!best_removal_pos.has_value()) {
                    break;
                }

                worst_tile_id = assigned_ids[best_removal_pos.value()];
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
            // Search from the back for the last removable (non-prefilled) tile
            const auto &ids = pal.assigned_tile_ids();
            std::optional<PackableTile::Id> removable_tid;
            for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
                if (!std::holds_alternative<PackableTile::PrefilledPaletteId>(*it)) {
                    removable_tid = *it;
                    break;
                }
            }
            if (!removable_tid.has_value()) {
                break; // Only prefilled tiles remain, nothing left to remove
            }

            pal.remove_tile(removable_tid.value());
            output.tile_to_pal_.erase(removable_tid.value());

            if (const auto it = tile_colors_map.find(removable_tid.value()); it != tile_colors_map.end()) {
                remaining_tile_pool.emplace_back(PackableTile{removable_tid.value(), it->second});
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
