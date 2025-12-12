#include "porytiles2/domain/packing/services/best_fusion_strategy.hpp"

#include <algorithm>

#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Finds the best palette for a tile based on palette-local weighted cost.
 *
 * @details
 * Uses cached palette color counts to compute weighted cost efficiently. The weighted cost
 * measures how well the tile's colors overlap with colors already in the palette.
 * Lower cost means better overlap.
 *
 * @return Index of the best palette, or nullopt if a new palette should be created
 */
[[nodiscard]] std::optional<std::size_t>
find_best_palette(const PackableTile &tile, const std::vector<PackedPalette> &palettes)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        const auto &pal = palettes[i];

        // Skip palettes that can't fit the tile
        if (!pal.can_fit(tile.color_set())) {
            continue;
        }

        // Use fast metric function with cached color counts - O(colors) instead of O(tiles × colors)
        double cost = compute_weighted_cost_in_palette_fast(tile.color_set(), pal);

        if (cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }

    // If best cost >= tile's color count, prefer creating a new palette
    // (no significant overlap benefit - each color contributes 1.0 when count is 0)
    if (best_idx.has_value() && best_cost >= static_cast<double>(tile.color_count())) {
        return std::nullopt;
    }

    return best_idx;
}

} // namespace

namespace porytiles2 {

ChainableResult<PackingOutput> BestFusionStrategy::pack(const PackingInput &input) const
{
    PackingOutput output;
    PalettePool pal_pool = input.pal_pool_;

    // Initialize output palettes from prefilled palettes
    output.pals_ = initialize_packed_palettes(input.prefilled_pals_, pal_pool, input.pal_capacity_);

    // Create additional empty palettes from the rest of the available PalettePool slots
    while (pal_pool.has_available_pal()) {
        output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
    }

    // Helper to assign a tile
    // Note: palette-local cost computation now uses cached color counts in PackedPalette,
    // eliminating the need for a separate tile_colors_map
    auto assign_tile = [&output](const PackableTile &tile) -> bool {
        const auto maybe_best_idx = find_best_palette(tile, output.pals_);

        if (maybe_best_idx.has_value()) {
            // Add to existing palette
            output.pals_[maybe_best_idx.value()].add_tile(tile);
            output.tile_to_pal_[tile.id()] = output.pals_[maybe_best_idx.value()].hardware_index();
            return true;
        }

        // Try to find an empty palette
        for (std::size_t i = 0; i < output.pals_.size(); ++i) {
            if (output.pals_[i].color_count() == 0 && output.pals_[i].can_fit(tile.color_set())) {
                output.pals_[i].add_tile(tile);
                output.tile_to_pal_[tile.id()] = output.pals_[i].hardware_index();
                return true;
            }
        }

        // Try to find ANY palette that can fit (even without good overlap)
        for (std::size_t i = 0; i < output.pals_.size(); ++i) {
            if (output.pals_[i].can_fit(tile.color_set())) {
                output.pals_[i].add_tile(tile);
                output.tile_to_pal_[tile.id()] = output.pals_[i].hardware_index();
                return true;
            }
        }

        // Cannot fit - would need more palettes
        return false;
    };

    // Create pool of tiles to be assigned
    std::vector<PackableTile> tile_pool{};
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
     * TODO: right now, we mix together the hints and regular tiles before sorting. Do we want this? I think it's
     * probably ok, since hints still guarantee that colors in the same hint will be in the same palette. And if the
     * user supplied hints that are larger than any individual tile, they'll go first as expected. However, I think it
     * makes sense to allow regular tiles that are large to go before smaller hints, since this probably helps to find
     * an optimal result. Since that larger tile *has* to get put somewhere in order for a solution to be found. No
     * sense running the hint first, only to block ourselves from finding a possible solution.
     */

    // Presort the tile pool so tiles with larger color counts come first (First Fit Decreasing heuristic)
    std::ranges::sort(
        tile_pool, [](const PackableTile &a, const PackableTile &b) { return a.color_count() > b.color_count(); });

    for (const auto &tile : tile_pool) {
        if (!assign_tile(tile)) {
            return FormattableError{"Best Fusion: cannot assign tile  - no palette has room"};
        }
    }

    return output;
}

} // namespace porytiles2
