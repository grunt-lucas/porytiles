#include "porytiles2/domain/packing/services/backtracking_strategy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <format>
#include <limits>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"
#include "porytiles2/domain/packing/models/palette_pool.hpp"
#include "porytiles2/domain/packing/models/shape_group_metadata.hpp"
#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"

namespace porytiles2 {

namespace {

// ============================================================================
// Internal types
// ============================================================================

enum class AssignResult { success, no_solution, cutoff_reached };

struct SearchParams {
    SearchAlgorithm algorithm;
    std::size_t node_cutoff;
    std::size_t best_branches; // SIZE_MAX = unlimited
    bool smart_prune;
};

struct SearchContext {
    std::vector<PackableTile> sorted_tiles;
    std::vector<ColorSet> initial_palette_colors;
    std::vector<std::size_t> palette_capacities;
    std::vector<std::size_t> hardware_indices;

    /**
     * @brief Optional shape group metadata for sharing-aware candidate sorting.
     *
     * @details
     * When non-null, the DFS and BFS algorithms deprioritize candidate palettes that already contain a sibling from the
     * same shape group. Sibling presence is inferred by checking whether any sibling tile's color set is a subset of
     * the candidate palette's current color set.
     */
    const ShapeGroupMetadata *shape_group_metadata = nullptr;

    /**
     * @brief Maps each sorted tile index to a list of sibling color sets from the same shape group.
     *
     * @details
     * Populated when shape_group_metadata is non-null. For tile at sorted_tiles[i], sibling_color_sets[i] contains the
     * color sets of all OTHER members of its shape group. Empty if the tile is not in a shape group or has no siblings.
     */
    std::vector<std::vector<ColorSet>> sibling_color_sets;
};

// ============================================================================
// BFS state types
// ============================================================================

struct BfsState {
    std::vector<ColorSet> palette_colors;
    std::size_t next_tile_index{};
    bool operator==(const BfsState &) const = default;
};

struct BfsStateHash {
    std::size_t operator()(const BfsState &s) const noexcept
    {
        std::size_t seed = std::hash<std::size_t>{}(s.next_tile_index);
        for (const auto &cs : s.palette_colors) {
            seed ^= std::hash<ColorSet>{}(cs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// ============================================================================
// Preset matrix (48 configurations)
// ============================================================================

[[nodiscard]] std::array<SearchParams, 48> build_preset_matrix()
{
    std::array<SearchParams, 48> matrix{};
    std::size_t idx = 0;

    /*
     * TODO: it turns out that after making the search space improvements to DFS/BFS, we often find a solution with
     * values as low as 50-100k. We should probably add more cutoffs here in the lower bounds. Eventually, it might be
     * nice to have quicker cutoff points if we implement the multi-threaded approach, since perhaps we can start
     * searching for "better" solutions and not just "a" solution.
     */
    constexpr std::array<std::size_t, 4> cutoffs = {1'000'000, 2'000'000, 4'000'000, 8'000'000};

    for (std::size_t cutoff : cutoffs) {
        for (auto algo : {SearchAlgorithm::dfs, SearchAlgorithm::bfs}) {
            // Configuration 1: unlimited branches with smart pruning
            matrix[idx++] = SearchParams{algo, cutoff, std::numeric_limits<std::size_t>::max(), true};

            // Configurations 2-6: limited branches without smart pruning
            for (std::size_t branches = 2; branches <= 6; ++branches) {
                matrix[idx++] = SearchParams{algo, cutoff, branches, false};
            }
        }
    }

    return matrix;
}

// ============================================================================
// Search context construction
// ============================================================================

[[nodiscard]] SearchContext build_search_context(const PackingInput &input)
{
    SearchContext ctx;

    // Combine hints and regular tiles, sorted descending by color count (FFD)
    ctx.sorted_tiles.reserve(input.hints_.size() + input.tiles_.size());
    for (const auto &hint : input.hints_) {
        ctx.sorted_tiles.push_back(hint);
    }
    for (const auto &tile : input.tiles_) {
        ctx.sorted_tiles.push_back(tile);
    }
    std::ranges::sort(ctx.sorted_tiles, [](const PackableTile &a, const PackableTile &b) {
        // Descending by color count, tiebreak by ID for determinism
        if (a.color_count() != b.color_count()) {
            return a.color_count() > b.color_count();
        }
        return a.id() < b.id();
    });

    // Initialize palettes from prefilled + available pool slots
    PalettePool pool = input.pal_pool_;
    auto prefilled_pals = initialize_packed_palettes(input.prefilled_pals_, pool, input.pal_capacity_);

    // Build palette arrays: first prefilled, then empty slots from pool
    for (const auto &pal : prefilled_pals) {
        ctx.initial_palette_colors.push_back(pal.color_set());
        ctx.palette_capacities.push_back(input.pal_capacity_);
        ctx.hardware_indices.push_back(pal.hardware_index());
    }

    // Compute effective capacities for prefilled palettes
    for (std::size_t i = 0; i < prefilled_pals.size(); ++i) {
        // Account for wasted slots from duplicate colors in prefilled palettes
        for (const auto &prefilled_pal : input.prefilled_pals_) {
            if (prefilled_pal.hardware_index() == ctx.hardware_indices[i]) {
                const std::size_t unique_colors = color_set_count(prefilled_pal.fixed_colors());
                const std::size_t occupied = prefilled_pal.occupied_slots();
                const std::size_t wasted = occupied - unique_colors;
                ctx.palette_capacities[i] = input.pal_capacity_ - wasted;
                break;
            }
        }
    }

    while (pool.has_available_pal()) {
        std::size_t hw_idx = pool.checkout();
        ctx.initial_palette_colors.emplace_back();
        ctx.palette_capacities.push_back(input.pal_capacity_);
        ctx.hardware_indices.push_back(hw_idx);
    }

    // Populate sharing metadata if available
    if (input.shape_group_metadata_.has_value()) {
        ctx.shape_group_metadata = &input.shape_group_metadata_.value();

        // Build sibling color sets for each sorted tile
        ctx.sibling_color_sets.resize(ctx.sorted_tiles.size());
        for (std::size_t i = 0; i < ctx.sorted_tiles.size(); ++i) {
            const auto &tile_id = ctx.sorted_tiles[i].id();
            auto group_it = ctx.shape_group_metadata->tile_id_to_group.find(tile_id);
            if (group_it == ctx.shape_group_metadata->tile_id_to_group.end()) {
                continue;
            }
            std::size_t group_idx = group_it->second;
            const auto &members = ctx.shape_group_metadata->group_members[group_idx];

            // Find sibling color sets from the sorted_tiles vector
            for (const auto &sibling_id : members) {
                if (sibling_id == tile_id) {
                    continue;
                }
                // Look up sibling in sorted_tiles to get its color set
                for (const auto &st : ctx.sorted_tiles) {
                    if (st.id() == sibling_id) {
                        ctx.sibling_color_sets[i].push_back(st.color_set());
                        break;
                    }
                }
            }
        }
    }

    return ctx;
}

// ============================================================================
// DFS algorithm
// ============================================================================

/*
 * DFS with in-place mutation and undo. Porytiles1 copied the entire palette vector for each branch;
 * we save and restore only the single modified ColorSet, reducing per-node allocation overhead.
 *
 * Candidate palettes are sorted by intersection_size (descending), then color_set_count (ascending).
 * This "best-fit" heuristic tries palettes with the most color overlap first, preferring emptier
 * palettes as a tiebreaker. smart_prune caps candidates after the first zero-intersection palette,
 * and best_branches limits total branching factor.
 */
AssignResult assign_depth_first(
    std::vector<ColorSet> &palette_colors,
    const SearchContext &ctx,
    const SearchParams &params,
    std::size_t next_tile_index,
    std::size_t &explored_nodes)
{
    ++explored_nodes;
    if (explored_nodes > params.node_cutoff) {
        return AssignResult::cutoff_reached;
    }

    // Base case: all tiles assigned
    if (next_tile_index >= ctx.sorted_tiles.size()) {
        return AssignResult::success;
    }

    const auto &tile = ctx.sorted_tiles[next_tile_index];
    const auto &tile_colors = tile.color_set();

    /*
     * Authoritative subset shortcut (improvement over Porytiles1).
     *
     * If the tile's colors are already a subset of some palette, the tile is satisfied without adding
     * any new colors. We recurse immediately and return the result directly — no fallthrough to the
     * candidate loop.
     *
     * Why authoritative? If skipping the tile fails (remaining tiles can't be packed), trying explicit
     * candidate assignments can only make things worse:
     *   - Assigning to the covering palette is a no-op (union doesn't change its ColorSet), so we'd
     *     re-explore the exact same subtree that already failed.
     *   - Assigning to a different palette ADDS colors to it, strictly reducing its remaining capacity.
     *
     * A non-authoritative version that fell through to candidates caused exponential blowup: at each
     * of K levels with a subset match, the same subtree was explored twice (once via shortcut, once
     * via the covering palette as first candidate), yielding O(2^K) redundant work. For tilesets like
     * gTileset_General with many shared colors, K is large enough to make the search hang indefinitely.
     */
    for (std::size_t i = 0; i < palette_colors.size(); ++i) {
        if (is_subset(tile_colors, palette_colors[i])) {
            return assign_depth_first(palette_colors, ctx, params, next_tile_index + 1, explored_nodes);
        }
    }

    // Build candidate list: (palette_index, intersection_size, color_set_count, has_sibling)
    struct Candidate {
        std::size_t pal_index;
        std::size_t isect_size;
        std::size_t cs_count;
        bool has_sibling;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(palette_colors.size());

    for (std::size_t i = 0; i < palette_colors.size(); ++i) {
        std::size_t u_size = union_size(tile_colors, palette_colors[i]);
        if (u_size <= ctx.palette_capacities[i]) {
            std::size_t i_size = intersection_size(tile_colors, palette_colors[i]);
            std::size_t c_count = color_set_count(palette_colors[i]);

            /*
             * Heuristic: check if this palette likely contains a sibling by testing whether any sibling's color set
             * is a subset of the palette's accumulated colors. This is an approximation — false positives are possible
             * when unrelated tiles contribute the same colors. False positives only cause suboptimal candidate ordering
             * (deprioritizing a palette unnecessarily), not incorrect packing.
             */
            bool sibling = false;
            if (!ctx.sibling_color_sets.empty() && next_tile_index < ctx.sibling_color_sets.size()) {
                for (const auto &sibling_cs : ctx.sibling_color_sets[next_tile_index]) {
                    if (is_subset(sibling_cs, palette_colors[i])) {
                        sibling = true;
                        break;
                    }
                }
            }

            candidates.push_back(Candidate{i, i_size, c_count, sibling});
        }
    }

    // Sort: no_sibling < has_sibling, then descending by intersection_size, then ascending by color_set_count
    std::ranges::sort(candidates, [](const Candidate &a, const Candidate &b) {
        if (a.has_sibling != b.has_sibling) {
            return !a.has_sibling;
        }
        if (a.isect_size != b.isect_size) {
            return a.isect_size > b.isect_size;
        }
        return a.cs_count < b.cs_count;
    });

    // Smart prune: cap candidates after first zero-intersection palette
    if (params.smart_prune) {
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (candidates[i].isect_size == 0) {
                // Keep this one but remove the rest after it
                candidates.resize(i + 1);
                break;
            }
        }
    }

    // Apply best_branches limit
    if (candidates.size() > params.best_branches) {
        candidates.resize(params.best_branches);
    }

    for (const auto &cand : candidates) {
        // Save/restore single palette (Porytiles1 copied the entire vector per branch)
        ColorSet saved = palette_colors[cand.pal_index];
        palette_colors[cand.pal_index] = color_set_union(palette_colors[cand.pal_index], tile_colors);

        auto result = assign_depth_first(palette_colors, ctx, params, next_tile_index + 1, explored_nodes);
        if (result != AssignResult::no_solution) {
            return result;
        }

        // Restore state (backtrack)
        palette_colors[cand.pal_index] = saved;
    }

    return AssignResult::no_solution;
}

// ============================================================================
// BFS algorithm
// ============================================================================

/*
 * BFS with dual-queue heuristic and visited-state deduplication (matching Porytiles1's approach).
 *
 * Two queues partition the frontier: high_queue for states reached via overlapping assignments
 * (intersection > 0), and low_queue for states reached via zero-overlap assignments. The high_queue
 * is always drained first, focusing exploration on promising branches before resorting to "waste"
 * assignments that consume fresh palette capacity.
 *
 * Improvement over Porytiles1: when ALL candidates for a tile have zero intersection (no palette has
 * any color overlap), Porytiles1 routed them to the high queue via a `sawAssignmentWithIntersection`
 * flag that stayed false. We preserve this behavior — zero-intersection candidates only go to the low
 * queue after we've seen at least one candidate with overlap. This prevents starvation when a tile
 * has entirely unique colors (common for early tiles assigned to empty palettes).
 */
AssignResult assign_breadth_first(
    const std::vector<ColorSet> &initial_colors,
    const SearchContext &ctx,
    const SearchParams &params,
    std::size_t &explored_nodes,
    std::vector<ColorSet> &solution)
{
    std::deque<BfsState> high_queue;
    std::deque<BfsState> low_queue;
    std::unordered_set<BfsState, BfsStateHash> visited;

    BfsState initial{initial_colors, 0};
    visited.insert(initial);
    high_queue.push_back(std::move(initial));

    while (!high_queue.empty() || !low_queue.empty()) {
        ++explored_nodes;
        if (explored_nodes > params.node_cutoff) {
            return AssignResult::cutoff_reached;
        }

        // Dequeue: prefer high_queue (overlap assignments)
        BfsState current = [&]() {
            if (!high_queue.empty()) {
                BfsState s = std::move(high_queue.front());
                high_queue.pop_front();
                return s;
            }
            BfsState s = std::move(low_queue.front());
            low_queue.pop_front();
            return s;
        }();

        // Skip tiles whose colors are already subsets (advance next_tile_index)
        std::size_t tile_idx = current.next_tile_index;
        while (tile_idx < ctx.sorted_tiles.size()) {
            const auto &tc = ctx.sorted_tiles[tile_idx].color_set();
            bool already_covered = false;
            for (const auto &pc : current.palette_colors) {
                if (is_subset(tc, pc)) {
                    already_covered = true;
                    break;
                }
            }
            if (!already_covered) {
                break;
            }
            ++tile_idx;
        }

        // Base case: all tiles assigned
        if (tile_idx >= ctx.sorted_tiles.size()) {
            solution = std::move(current.palette_colors);
            return AssignResult::success;
        }

        const auto &tile_colors = ctx.sorted_tiles[tile_idx].color_set();

        // Build candidates
        struct Candidate {
            std::size_t pal_index;
            std::size_t isect_size;
            std::size_t cs_count;
            bool has_sibling;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(current.palette_colors.size());

        for (std::size_t i = 0; i < current.palette_colors.size(); ++i) {
            std::size_t u_size = union_size(tile_colors, current.palette_colors[i]);
            if (u_size <= ctx.palette_capacities[i]) {
                std::size_t i_size = intersection_size(tile_colors, current.palette_colors[i]);
                std::size_t c_count = color_set_count(current.palette_colors[i]);

                // Check if this palette already contains a sibling
                bool sibling = false;
                if (!ctx.sibling_color_sets.empty() && tile_idx < ctx.sibling_color_sets.size()) {
                    for (const auto &sibling_cs : ctx.sibling_color_sets[tile_idx]) {
                        if (is_subset(sibling_cs, current.palette_colors[i])) {
                            sibling = true;
                            break;
                        }
                    }
                }

                candidates.push_back(Candidate{i, i_size, c_count, sibling});
            }
        }

        // Sort: no_sibling < has_sibling, then descending by intersection_size, ascending by color_set_count
        std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
            if (a.has_sibling != b.has_sibling) {
                return !a.has_sibling;
            }
            if (a.isect_size != b.isect_size) {
                return a.isect_size > b.isect_size;
            }
            return a.cs_count < b.cs_count;
        });

        // Smart prune
        if (params.smart_prune) {
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                if (candidates[i].isect_size == 0) {
                    candidates.resize(i + 1);
                    break;
                }
            }
        }

        // Apply best_branches limit
        if (candidates.size() > params.best_branches) {
            candidates.resize(params.best_branches);
        }

        // Track whether we've seen any candidate with color overlap, matching Porytiles1's
        // dual-queue heuristic: when NO candidate has intersection, all go to high_queue
        // (they're the only options). Only after seeing an intersection candidate do
        // zero-intersection candidates go to low_queue.
        bool saw_intersection = false;

        for (const auto &cand : candidates) {
            BfsState next_state;
            next_state.palette_colors = current.palette_colors;
            next_state.palette_colors[cand.pal_index] =
                color_set_union(next_state.palette_colors[cand.pal_index], tile_colors);
            next_state.next_tile_index = tile_idx + 1;

            if (cand.isect_size > 0) {
                saw_intersection = true;
            }

            if (!visited.contains(next_state)) {
                visited.insert(next_state);
                if (saw_intersection && cand.isect_size == 0) {
                    low_queue.push_back(std::move(next_state));
                }
                else {
                    high_queue.push_back(std::move(next_state));
                }
            }
        }
    }

    return AssignResult::no_solution;
}

// ============================================================================
// Output reconstruction
// ============================================================================

[[nodiscard]] PackingOutput
build_packing_output(const std::vector<ColorSet> &solution_colors, const SearchContext &ctx, const PackingInput &input)
{
    PackingOutput output;

    // Create PackedPalettes
    for (std::size_t i = 0; i < ctx.hardware_indices.size(); ++i) {
        PackedPalette pal{ctx.hardware_indices[i], ctx.palette_capacities[i]};

        // Add system tile for prefilled palettes
        for (const auto &prefilled : input.prefilled_pals_) {
            if (prefilled.hardware_index() == ctx.hardware_indices[i] &&
                color_set_count(prefilled.fixed_colors()) > 0) {
                PackableTile system_tile{
                    PackableTile::PrefilledPaletteId{prefilled.hardware_index()}, prefilled.fixed_colors()};
                pal.add_tile(system_tile);
                break;
            }
        }

        output.pals_.push_back(std::move(pal));
    }

    // Assign each tile to the first palette whose solution colors are a superset
    for (const auto &tile : ctx.sorted_tiles) {
        // Skip prefilled palette system tiles — they were already added above
        if (tile.is_prefilled_palette()) {
            continue;
        }

        for (std::size_t i = 0; i < solution_colors.size(); ++i) {
            if (is_subset(tile.color_set(), solution_colors[i])) {
                output.pals_[i].add_tile(tile);
                output.tile_to_pal_[tile.id()] = ctx.hardware_indices[i];
                break;
            }
        }
    }

    return output;
}

[[nodiscard]] PackingOutput build_empty_output(const SearchContext &ctx, const PackingInput &input)
{
    PackingOutput output;
    for (std::size_t i = 0; i < ctx.hardware_indices.size(); ++i) {
        PackedPalette pal{ctx.hardware_indices[i], ctx.palette_capacities[i]};

        for (const auto &prefilled : input.prefilled_pals_) {
            if (prefilled.hardware_index() == ctx.hardware_indices[i] &&
                color_set_count(prefilled.fixed_colors()) > 0) {
                PackableTile system_tile{
                    PackableTile::PrefilledPaletteId{prefilled.hardware_index()}, prefilled.fixed_colors()};
                pal.add_tile(system_tile);
                break;
            }
        }

        output.pals_.push_back(std::move(pal));
    }
    return output;
}

[[nodiscard]] std::string format_search_params_line(const SearchParams &params)
{
    std::string branches_str = params.best_branches == std::numeric_limits<std::size_t>::max()
                                   ? "unlimited"
                                   : std::to_string(params.best_branches);
    return std::format(
        "algorithm={}, node_cutoff={}, best_branches={}, smart_prune={}.",
        to_string(params.algorithm),
        params.node_cutoff,
        branches_str,
        params.smart_prune ? "true" : "false");
}

void emit_success_remark(const UserDiagnostics &diag, const SearchParams &params, bool is_preset)
{
    std::vector<std::string> lines;
    if (is_preset) {
        lines.emplace_back("Backtracking search succeeded with preset config:");
    }
    else {
        lines.emplace_back("Backtracking search succeeded:");
    }
    lines.emplace_back(format_search_params_line(params));
    diag.remark("backtracking-search", lines);
}

} // namespace

// ============================================================================
// BacktrackingStrategy::pack
// ============================================================================

ChainableResult<PackingOutput> BacktrackingStrategy::pack(const PackingInput &input) const
{
    auto ctx = build_search_context(input);

    if (ctx.sorted_tiles.empty()) {
        return build_empty_output(ctx, input);
    }

    if (use_preset_matrix_) {
        auto matrix = build_preset_matrix();

        for (const auto &params : matrix) {
            std::size_t explored = 0;

            if (params.algorithm == SearchAlgorithm::dfs) {
                auto colors = ctx.initial_palette_colors;
                if (assign_depth_first(colors, ctx, params, 0, explored) == AssignResult::success) {
                    if (diag_ != nullptr) {
                        emit_success_remark(*diag_, params, true);
                    }
                    return build_packing_output(colors, ctx, input);
                }
            }
            else {
                std::vector<ColorSet> solution;
                if (assign_breadth_first(ctx.initial_palette_colors, ctx, params, explored, solution) ==
                    AssignResult::success) {
                    if (diag_ != nullptr) {
                        emit_success_remark(*diag_, params, true);
                    }
                    return build_packing_output(solution, ctx, input);
                }
            }
        }

        return FormattableError{
            "Backtracking strategy failed to find a valid palette assignment after all preset configurations."};
    }

    // Single-config mode: run one search with the configured parameters
    SearchParams params{algorithm_, node_cutoff_, best_branches_, smart_prune_};
    std::size_t explored = 0;

    if (algorithm_ == SearchAlgorithm::dfs) {
        auto colors = ctx.initial_palette_colors;
        if (assign_depth_first(colors, ctx, params, 0, explored) == AssignResult::success) {
            if (diag_ != nullptr) {
                emit_success_remark(*diag_, params, false);
            }
            return build_packing_output(colors, ctx, input);
        }
    }
    else {
        std::vector<ColorSet> solution;
        if (assign_breadth_first(ctx.initial_palette_colors, ctx, params, explored, solution) ==
            AssignResult::success) {
            if (diag_ != nullptr) {
                emit_success_remark(*diag_, params, false);
            }
            return build_packing_output(solution, ctx, input);
        }
    }

    return FormattableError{
        "Backtracking strategy failed to find a valid palette assignment with the configured parameters."};
}

} // namespace porytiles2
