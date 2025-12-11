#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"

#include <deque>
#include <set>

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

        double cost = compute_relative_size(info.tile.color_set(), multiplicity);

        if (cost < best_cost) {
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
    std::deque<TileInfo> pool;
    for (const auto &hint : input.hints_) {
        pool.emplace_back(hint);
    }
    for (const auto &tile : input.tiles_) {
        pool.emplace_back(tile);
    }

    if (pool.empty()) {
        return output;
    }

    // Pop first tile and assign to first available palette
    TileInfo first_info = std::move(pool.front());
    pool.pop_front();

    // TODO: impl the rest

    return output;
}

} // namespace porytiles2
