#pragma once

#include <cstddef>
#include <limits>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/search_algorithm.hpp"
#include "porytiles/domain/packing/services/packing_strategy.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Packing strategy that assigns tiles to palettes using BFS/DFS backtracking with a fallback matrix.
///
/// @details
/// This strategy systematically explores the solution space for valid palette assignments using depth-first and
/// breadth-first search with backtracking. A fallback matrix of 48 configurations (varying cutoff limits, branch
/// limits, and search algorithms) is tried in sequence until a solution is found.
///
/// BFS uses visited-state deduplication and a dual-queue heuristic (high-overlap assignments prioritized) to
/// efficiently explore the state space. DFS uses in-place mutation with undo for memory efficiency.
///
/// This strategy is designed to solve instances that defeat greedy heuristics like Overload-and-Remove, such as
/// gTileset_General from pokefirered.
///
/// Based on Porytiles1's palette_assignment.cpp, with two key improvements:
///
/// 1. **Authoritative DFS subset shortcut.** When a tile's colors are already a subset of some palette, Porytiles1 had
///    no general subset shortcut for primary tilesets. It processed the tile as a normal candidate. Our implementation
///    detects the subset case and immediately recurses without modification. Critically, the result is returned
///    directly (no fallthrough to the candidate loop). A non-authoritative version that fell through caused exponential
///    blowup: the covering palette appeared as the first candidate (union is a no-op), re-exploring the same subtree.
///    At K levels with subset matches, total work became O(2^K) times the necessary amount.
///
/// 2. **DFS in-place mutation with undo.** Porytiles1 copied the entire palette vector for each DFS branch. We save and
///    restore only the single modified palette, reducing per-node allocation overhead.
///
/// The strategy supports two modes:
///
/// - **Preset matrix mode** (default): Iterates through 48 known-good configurations (varying cutoffs, branch limits,
///   and algorithms) until a solution is found. Use this when the input characteristics are unknown.
///
/// - **Single-config mode**: Runs a single search with caller-specified parameters. Use this when you know which
///   settings work for your input, or for targeted testing and tuning.
class BacktrackingStrategy final : public PackingStrategy {
  public:
    BacktrackingStrategy() = default;

    /// @brief Constructs in preset matrix mode with diagnostics support.
    ///
    /// @details
    /// Uses the full 48-configuration preset matrix (default behavior) and emits a remark via the provided diagnostics
    /// when a successful configuration is found.
    ///
    /// @param diag Diagnostics interface for emitting remarks about successful search parameters.
    explicit BacktrackingStrategy(gsl::not_null<const UserDiagnostics *> diag) : diag_{diag} {}

    /// @brief Constructs with explicit search parameters for single-configuration mode.
    ///
    /// @details
    /// When constructed with this constructor, the strategy runs a single search with the provided parameters instead
    /// of iterating through the preset configuration matrix.
    ///
    /// @param algorithm The search algorithm to use (DFS or BFS).
    /// @param node_cutoff Maximum number of nodes to explore before giving up.
    /// @param best_branches Maximum number of candidate branches per node (@c SIZE_MAX for unlimited).
    /// @param smart_prune Whether to cap candidates after the first zero-intersection palette.
    /// @param diag Optional diagnostics interface for emitting remarks about successful search parameters.
    /// @pre @p node_cutoff must be greater than zero.
    explicit BacktrackingStrategy(
        SearchAlgorithm algorithm,
        std::size_t node_cutoff,
        std::size_t best_branches,
        bool smart_prune,
        const UserDiagnostics *diag = nullptr)
        : use_preset_matrix_{false}, algorithm_{algorithm}, node_cutoff_{node_cutoff}, best_branches_{best_branches},
          smart_prune_{smart_prune}, diag_{diag}
    {
    }

    /// @brief Packs tiles into palettes using BFS/DFS backtracking search.
    ///
    /// @details
    /// In preset matrix mode, tries 48 search configurations (DFS and BFS with varying cutoffs and branch limits) until
    /// a valid assignment is found. In single-config mode, runs one search with the configured parameters. The search
    /// state is a compact vector of ColorSets (one per palette), and tile-to-palette mappings are reconstructed after a
    /// solution is found.
    ///
    /// @param input The packing input containing tiles, hints, fixed slots, and constraints
    /// @return A PackingOutput on success, or an error if all configurations fail
    [[nodiscard]] ChainableResult<PackingOutput> pack(const PackingInput &input) const override;

    /// @brief Returns the name of this strategy.
    ///
    /// @return "Backtracking"
    [[nodiscard]] std::string name() const override
    {
        return "Backtracking";
    }

  private:
    bool use_preset_matrix_ = true;
    SearchAlgorithm algorithm_ = SearchAlgorithm::dfs;
    std::size_t node_cutoff_ = 1'000'000;
    std::size_t best_branches_ = std::numeric_limits<std::size_t>::max();
    bool smart_prune_ = true;
    const UserDiagnostics *diag_ = nullptr;
};

} // namespace porytiles
