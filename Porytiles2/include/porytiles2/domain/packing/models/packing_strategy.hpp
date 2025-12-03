#pragma once

#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for palette packing algorithms.
 *
 * @details
 * PackingStrategy defines the interface for algorithms that solve the "Pagination Problem" (Bin Packing with
 * Overlapping Items). Different strategies may use different approaches such as greedy algorithms (Best Fusion) or
 * non-greedy algorithms with backtracking (Overload-and-Remove, Classic DFS, Classic BFS).
 *
 * Implementations should be stateless - all state needed for packing is provided via the PackingInput parameter.
 */
class PackingStrategy {
  public:
    virtual ~PackingStrategy() = default;

    /**
     * @brief Packs tile colors into palettes according to the strategy's algorithm.
     *
     * @details
     * Attempts to assign all tiles from the input to palettes such that each tile's colors fit within at least one
     * palette's capacity.
     *
     * @param input The packing input containing tiles, hints, fixed slots, and constraints
     * @return A PackingResult on success, or an error if packing is not possible
     */
    // TODO: this should return our result type and take our input type
    [[nodiscard]] virtual ChainableResult<void> pack(const std::string &input) const = 0;

    /**
     * @brief Returns the human-readable name of this strategy.
     *
     * @return The strategy name (e.g., "Best Fusion", "Overload-and-Remove")
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

} // namespace porytiles2
