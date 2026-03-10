#pragma once

#include <string>

#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Packing strategy that assigns tiles to palettes using BFS/DFS backtracking with a fallback matrix.
 *
 * @details
 * This strategy systematically explores the solution space for valid palette assignments using depth-first and
 * breadth-first search with backtracking. A fallback matrix of 48 configurations (varying cutoff limits, branch limits,
 * and search algorithms) is tried in sequence until a solution is found.
 *
 * BFS uses visited-state deduplication and a dual-queue heuristic (high-overlap assignments prioritized) to efficiently
 * explore the state space. DFS uses in-place mutation with undo for memory efficiency.
 *
 * This strategy is designed to solve instances that defeat greedy heuristics like Overload-and-Remove, such as
 * gTileset_General from pokefirered.
 *
 * Based on Porytiles1's palette_assignment.cpp, with two key improvements:
 *
 * 1. **Authoritative DFS subset shortcut.** When a tile's colors are already a subset of some palette, Porytiles1 had
 *    no general subset shortcut for primary tilesets — it processed the tile as a normal candidate. Our implementation
 *    detects the subset case and immediately recurses without modification. Critically, the result is returned directly
 *    (no fallthrough to the candidate loop). A non-authoritative version that fell through caused exponential blowup:
 *    the covering palette appeared as the first candidate (union is a no-op), re-exploring the same subtree. At K
 *    levels with subset matches, total work became O(2^K) times the necessary amount.
 *
 * 2. **DFS in-place mutation with undo.** Porytiles1 copied the entire palette vector for each DFS branch. We save and
 *    restore only the single modified palette, reducing per-node allocation overhead.
 */
class BacktrackingStrategy final : public PackingStrategy {
  public:
    BacktrackingStrategy() = default;

    /**
     * @brief Packs tiles into palettes using BFS/DFS backtracking search.
     *
     * @details
     * Tries 48 search configurations (DFS and BFS with varying cutoffs and branch limits) until a valid assignment is
     * found. The search state is a compact vector of ColorSets (one per palette), and tile-to-palette mappings are
     * reconstructed after a solution is found.
     *
     * @param input The packing input containing tiles, hints, fixed slots, and constraints
     * @return A PackingOutput on success, or an error if all configurations fail
     */
    [[nodiscard]] ChainableResult<PackingOutput> pack(const PackingInput &input) const override;

    /**
     * @brief Returns the name of this strategy.
     *
     * @return "Backtracking"
     */
    [[nodiscard]] std::string name() const override
    {
        return "Backtracking";
    }
};

} // namespace porytiles2
