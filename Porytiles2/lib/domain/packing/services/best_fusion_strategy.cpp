#include "porytiles2/domain/packing/services/best_fusion_strategy.hpp"

#include <map>

#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Finds the best palette for a tile based on palette-local weighted cost.
 *
 * @details
 * Uses palette-local multiplicity to find the best-fitting palette. The weighted cost
 * measures how well the tile's colors overlap with colors already in the palette.
 * Lower cost means better overlap.
 *
 * @return Index of the best palette, or nullopt if a new palette should be created
 */
[[nodiscard]] std::optional<std::size_t> find_best_palette(
    const PackableTile &tile,
    const std::vector<PackedPalette> &palettes,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        const auto &pal = palettes[i];

        // Skip palettes that can't fit the tile
        if (!pal.can_fit(tile.color_set())) {
            continue;
        }

        double cost = compute_weighted_cost_in_palette(tile.color_set(), pal, tile_colors_map);

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

    // Build tile colors map for palette-local cost computation
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

    // Helper to assign a tile
    auto assign_tile = [&output, &tile_colors_map](const PackableTile &tile) -> bool {
        auto maybe_best_idx = find_best_palette(tile, output.pals_, tile_colors_map);

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

    /*
     * TODO: do we want to create a sorted tile pool here like overload-and-remove?
     */

    // Process hints first
    for (const auto &hint : input.hints_) {
        if (!assign_tile(hint)) {
            // TODO: better error message
            return FormattableError{"Best Fusion: cannot assign hint tile  - no palette has room"};
        }
    }

    // Process regular tiles
    for (const auto &tile : input.tiles_) {
        if (!assign_tile(tile)) {
            // TODO: better error message
            return FormattableError{"Best Fusion: cannot assign regular tile  - no palette has room"};
        }
    }

    return output;
}

} // namespace porytiles2
