#include "porytiles2/domain/packing/services/best_fusion_strategy.hpp"

#include "porytiles2/domain/packing/models/packable_tile.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Builds a map from color index to multiplicity (how many tiles use that color).
 */
[[nodiscard]] std::map<std::size_t, std::size_t>
build_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints)
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

/**
 * @brief Computes the weighted cost of placing a tile in a palette.
 *
 * @details
 * Weighted cost = sum(1 / multiplicity[color]) for each color in the tile that
 * would be NEW to the palette. Colors already in the palette have zero cost.
 */
[[nodiscard]] double compute_weighted_cost(
    const ColorSet &tile_colors, const ColorSet &palette_colors, const std::map<std::size_t, std::size_t> &multiplicity)
{
    double cost = 0.0;

    for_each_color(tile_colors, [&cost, &palette_colors, &multiplicity](const std::size_t color_idx) {
        // Only count colors not already in the palette
        if (!palette_colors.test(ColorIndex{static_cast<std::uint8_t>(color_idx)})) {
            const auto it = multiplicity.find(color_idx);
            const std::size_t mult = it != multiplicity.end() ? it->second : 1;
            cost += 1.0 / static_cast<double>(mult);
        }
    });

    return cost;
}

/**
 * @brief Finds the best palette for a tile based on weighted cost.
 *
 * @return Index of the best palette, or nullopt if a new palette should be created
 */
[[nodiscard]] std::optional<std::size_t> find_best_palette(
    const PackableTile &tile,
    const std::vector<PackedPalette> &palettes,
    const std::map<std::size_t, std::size_t> &multiplicity)
{
    std::optional<std::size_t> best_idx;
    double best_cost = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < palettes.size(); ++i) {
        const auto &pal = palettes[i];

        // Skip palettes that can't fit the tile
        if (!pal.can_fit(tile.color_set())) {
            continue;
        }

        const double cost = compute_weighted_cost(tile.color_set(), pal.color_set(), multiplicity);

        if (cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }

    // If best cost >= tile's color count, prefer creating a new palette
    // (no significant overlap benefit)
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
    for (const auto &prefilled_pal : input.prefilled_pals_) {
        // Only set up prefilled palettes for slots that were requested via PalettePool
        if (pal_pool.is_available(prefilled_pal.hardware_index())) {
            pal_pool.checkout(prefilled_pal.hardware_index());
            PackedPalette pal{prefilled_pal.hardware_index(), input.pal_capacity_};
            ColorSet fixed_colors = prefilled_pal.fixed_colors();
            if (color_set_count(fixed_colors) > 0) {
                // Pre-populate with fixed colors using a "system" tile with a special ID
                const auto system_id = std::numeric_limits<std::size_t>::max() - prefilled_pal.hardware_index();
                PackableTile system_tile{PackableTile::PrefilledPaletteId{system_id}, fixed_colors};
                pal.add_tile(system_tile);
            }
            output.pals_.push_back(std::move(pal));
        }
    }

    // Create additional empty palettes from the rest of the available PalettePool slots
    while (pal_pool.has_available_index()) {
        output.pals_.emplace_back(pal_pool.checkout(), input.pal_capacity_);
    }

    // Build multiplicity map
    std::map<std::size_t, std::size_t> multiplicity = build_multiplicity_map(input.tiles_, input.hints_);

    // Helper to assign a tile
    auto assign_tile = [&output, &multiplicity](const PackableTile &tile) -> bool {
        auto best_idx = find_best_palette(tile, output.pals_, multiplicity);

        if (best_idx.has_value()) {
            // Add to existing palette
            output.pals_[*best_idx].add_tile(tile);
            output.tile_to_pal_[tile.id()] = output.pals_[best_idx.value()].hardware_index();
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
