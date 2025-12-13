#include "porytiles2/domain/packing/services/classic_dfs_strategy.hpp"

#include <algorithm>
#include <vector>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"
#include "porytiles2/domain/packing/models/prefilled_palette.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Result of a DFS assignment attempt.
 */
enum class AssignResult { success, no_solution, cutoff_reached };

/**
 * @brief State for the DFS search.
 *
 * @details
 * We track only the accumulated colors in each palette, not tile IDs. This allows efficient backtracking by copying the
 * state. Tile-to-palette mapping is reconstructed after a solution is found.
 */
struct DfsState {
    std::vector<ColorSet> palette_colors;
    std::size_t remaining_hint_count;
    std::size_t remaining_tile_count;
};

/**
 * @brief Recursive DFS assignment function.
 *
 * @details
 * Attempts to assign all tiles to palettes using depth-first search with backtracking.
 *
 * @param state Current search state (modified in place, restored on backtrack)
 * @param tiles Regular tiles to assign
 * @param hints Priority tiles to assign first
 * @param prefilled Prefilled palettes (e.g., from primary tileset)
 * @param palette_capacity Maximum colors per palette
 * @param node_cutoff Maximum nodes to explore
 * @param explored_nodes Counter for nodes explored (incremented by reference)
 * @param solution Output: the solution palette colors if found
 * @return AssignResult indicating success, failure, or cutoff
 */
AssignResult assign_depth_first(
    DfsState &state,
    const std::vector<PackableTile> &tiles,
    const std::vector<PackableTile> &hints,
    const std::vector<PrefilledPalette> &prefilled,
    std::size_t palette_capacity,
    std::size_t node_cutoff,
    std::size_t &explored_nodes,
    std::vector<ColorSet> &solution)
{
    ++explored_nodes;
    if (explored_nodes > node_cutoff) {
        return AssignResult::cutoff_reached;
    }

    // Base case: all tiles assigned
    if (state.remaining_hint_count == 0 && state.remaining_tile_count == 0) {
        solution = state.palette_colors;
        return AssignResult::success;
    }

    // Get the next tile to assign (hints first, then regular tiles)
    // We assign from the back of the vectors for efficiency
    ColorSet to_assign;
    std::size_t new_hint_count = state.remaining_hint_count;
    std::size_t new_tile_count = state.remaining_tile_count;

    if (state.remaining_hint_count != 0) {
        to_assign = hints.at(state.remaining_hint_count - 1).color_set();
        new_hint_count = state.remaining_hint_count - 1;
    }
    else {
        to_assign = tiles.at(state.remaining_tile_count - 1).color_set();
        new_tile_count = state.remaining_tile_count - 1;
    }

    // Check if tile fits entirely within a prefilled palette (can skip assignment)
    for (const auto &pf : prefilled) {
        if (is_subset(to_assign, pf.fixed_colors())) {
            // Tile's colors are fully contained in this prefilled palette
            // Continue without modifying palette_colors
            DfsState updated_state{state.palette_colors, new_hint_count, new_tile_count};
            AssignResult result = assign_depth_first(
                updated_state, tiles, hints, prefilled, palette_capacity, node_cutoff, explored_nodes, solution);
            if (result == AssignResult::success) {
                return AssignResult::success;
            }
            if (result == AssignResult::cutoff_reached) {
                return AssignResult::cutoff_reached;
            }
        }
    }

    // Sort palettes by intersection size (descending), with tie-breaker on palette size (ascending)
    // We work on indices to avoid copying ColorSets repeatedly
    std::vector<std::size_t> palette_order(state.palette_colors.size());
    for (std::size_t i = 0; i < palette_order.size(); ++i) {
        palette_order[i] = i;
    }

    std::ranges::stable_sort(palette_order, [&](std::size_t idx1, std::size_t idx2) {
        const ColorSet &pal1 = state.palette_colors[idx1];
        const ColorSet &pal2 = state.palette_colors[idx2];

        std::size_t intersect1 = intersection_size(pal1, to_assign);
        std::size_t intersect2 = intersection_size(pal2, to_assign);

        if (intersect1 == intersect2) {
            // Tie-breaker: prefer smaller palettes
            return color_set_count(pal1) < color_set_count(pal2);
        }
        // Prefer larger intersections
        return intersect1 > intersect2;
    });

    // Smart pruning: stop after the first palette with zero intersection
    std::size_t stop_limit = palette_order.size();
    for (std::size_t i = 0; i < palette_order.size(); ++i) {
        std::size_t idx = palette_order[i];
        if (intersection_size(state.palette_colors[idx], to_assign) == 0) {
            stop_limit = i + 1; // Include this one, but stop after
            break;
        }
    }

    // Try assigning to each candidate palette
    for (std::size_t i = 0; i < stop_limit; ++i) {
        std::size_t pal_idx = palette_order[i];
        const ColorSet &palette = state.palette_colors[pal_idx];

        // Check if tile fits in this palette
        if (union_size(palette, to_assign) > palette_capacity) {
            continue; // Doesn't fit, try next
        }

        // Create updated state with tile assigned to this palette
        std::vector<ColorSet> updated_colors = state.palette_colors;
        updated_colors[pal_idx] = color_set_union(palette, to_assign);

        DfsState updated_state{updated_colors, new_hint_count, new_tile_count};
        AssignResult result = assign_depth_first(
            updated_state, tiles, hints, prefilled, palette_capacity, node_cutoff, explored_nodes, solution);
        if (result == AssignResult::success) {
            return AssignResult::success;
        }
        if (result == AssignResult::cutoff_reached) {
            return AssignResult::cutoff_reached;
        }
        // Otherwise backtrack: updated_state is discarded, try next palette
    }

    // No valid assignment found from this state
    return AssignResult::no_solution;
}

/**
 * @brief Builds the packing result from the solution ColorSets.
 *
 * @details
 * After DFS finds valid palette colors, this function creates PackedPalette objects and determines which palette each
 * tile belongs to by checking subset relationships.
 */
[[nodiscard]] PackingResult build_packing_result(
    const std::vector<ColorSet> &solution_colors,
    const std::vector<PackableTile> &tiles,
    const std::vector<PackableTile> &hints,
    const std::vector<PrefilledPalette> &prefilled,
    std::size_t palette_capacity)
{
    PackingResult result;

    // Create PackedPalettes from solution colors
    for (std::size_t i = 0; i < solution_colors.size(); ++i) {
        PackedPalette pal{i, palette_capacity};

        // If this palette has prefilled colors, add them first with a system tile ID
        for (const auto &pf : prefilled) {
            if (pf.palette_index() == i && color_set_count(pf.fixed_colors()) > 0) {
                pal.add_tile(std::numeric_limits<std::size_t>::max() - i, pf.fixed_colors());
                break;
            }
        }

        result.palettes().push_back(std::move(pal));
    }

    // Helper to assign a tile to its palette
    auto assign_tile_to_palette = [&result, &solution_colors](const PackableTile &tile) {
        const ColorSet &tile_colors = tile.color_set();

        // Find which solution palette contains this tile's colors
        for (std::size_t i = 0; i < solution_colors.size(); ++i) {
            if (is_subset(tile_colors, solution_colors[i])) {
                result.palettes()[i].add_tile(tile.tile_id(), tile_colors);
                result.tile_to_palette()[tile.tile_id()] = result.palettes()[i].palette_index();
                return;
            }
        }
    };

    // Assign all tiles
    for (const auto &hint : hints) {
        assign_tile_to_palette(hint);
    }
    for (const auto &tile : tiles) {
        assign_tile_to_palette(tile);
    }

    return result;
}

} // namespace

ChainableResult<PackingResult> ClassicDfsStrategy::pack(const PackingInput &input) const
{
    // Initialize palette colors
    std::vector<ColorSet> initial_palette_colors(input.num_palettes());

    // Pre-populate with prefilled palette colors
    for (const auto &pf : input.prefilled_palettes()) {
        if (pf.palette_index() < initial_palette_colors.size()) {
            /*
             * TODO: this is broken: e.g. initial_palette_colors has size 6 due to num_tiles_in_primary being used to
             * initialize input.num_palettes(), but user supplied a pal 12 override in their Porytiles component
             */
            initial_palette_colors[pf.palette_index()] = pf.fixed_colors();
        }
    }

    // Sort tiles ascending, recursive code will work off tail end of vector, placing tiles with more colors first
    std::vector<PackableTile> sorted_tiles = input.tiles();
    std::vector<PackableTile> sorted_hints = input.hints();
    std::ranges::stable_sort(
        sorted_tiles, [](const PackableTile &a, const PackableTile &b) { return a.color_count() < b.color_count(); });
    std::ranges::stable_sort(
        sorted_hints, [](const PackableTile &a, const PackableTile &b) { return a.color_count() < b.color_count(); });

    // Set up initial state
    DfsState initial_state{initial_palette_colors, sorted_hints.size(), sorted_tiles.size()};

    std::size_t explored_nodes = 0;
    std::vector<ColorSet> solution;

    AssignResult result = assign_depth_first(
        initial_state,
        sorted_tiles,
        sorted_hints,
        input.prefilled_palettes(),
        input.palette_capacity(),
        node_cutoff_,
        explored_nodes,
        solution);

    if (result == AssignResult::cutoff_reached) {
        return FormattableError{
            "Classic DFS: exploration cutoff reached after " + std::to_string(explored_nodes) + " nodes"};
    }

    if (result == AssignResult::no_solution) {
        return FormattableError{"Classic DFS: no valid palette assignment exists"};
    }

    // Build the final result
    return build_packing_result(
        solution, sorted_tiles, sorted_hints, input.prefilled_palettes(), input.palette_capacity());
}

} // namespace porytiles2
