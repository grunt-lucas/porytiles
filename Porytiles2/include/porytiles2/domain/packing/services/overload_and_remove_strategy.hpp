#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/domain/packing/services/shuffle_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Packing strategy that assigns tiles to palettes using overload-and-remove with multi-start.
 *
 * @details
 * Tiles are greedily assigned to the palette with the best color overlap. When a palette becomes overloaded (exceeds
 * its color capacity), the worst-fitting tile is removed and re-queued with that palette marked as forbidden,
 * guaranteeing termination. A multi-start loop retries with different tile orderings controlled by
 * @c ShuffleStrategy to escape local optima that defeat a single FFD pass.
 *
 * This algorithm is based on insights and samples from:
 *
 * Grange, A., Kacem, I., & Martin, S. (2018). Algorithms for the bin packing problem with overlapping items. Computers
 * & Industrial Engineering, 115, 331–341. https://doi.org/10.1016/j.cie.2017.10.015 https://arxiv.org/pdf/1605.00558
 */
class OverloadAndRemoveStrategy final : public PackingStrategy {
  public:
    OverloadAndRemoveStrategy() = default;

    /**
     * @brief Constructs with multi-start parameters.
     *
     * @param max_attempts Maximum number of packing attempts (first uses FFD, subsequent use shuffled orderings)
     * @param seed Base seed for the PRNG used to generate shuffle seeds
     * @param shuffle_strategy Strategy for generating tile orderings in retry attempts
     */
    explicit OverloadAndRemoveStrategy(std::size_t max_attempts, std::uint64_t seed, ShuffleStrategy shuffle_strategy)
        : max_attempts_{max_attempts}, seed_{seed}, shuffle_strategy_{shuffle_strategy}
    {
    }

    /**
     * @brief Packs tiles into palettes using the Overload-And-Remove algorithm with multi-start.
     *
     * @details
     * Attempts packing with FFD ordering first. If that fails and max_attempts > 1, retries with shuffled tile
     * orderings using a seeded PRNG. Returns the first successful result, or the first attempt's error if all fail.
     *
     * @param input The packing input containing tiles, hints, fixed slots, and constraints
     * @return A PackingOutput on success, or an error if packing fails
     */
    [[nodiscard]] ChainableResult<PackingOutput> pack(const PackingInput &input) const override;

    /**
     * @brief Returns the name of this strategy.
     *
     * @return "Overload-And-Remove"
     */
    [[nodiscard]] std::string name() const override
    {
        return "Overload-And-Remove";
    }

  private:
    std::size_t max_attempts_ = 20;
    std::uint64_t seed_ = 42;
    ShuffleStrategy shuffle_strategy_ = ShuffleStrategy::noisy_ffd;

    /**
     * @brief Performs a single packing attempt with the given tile ordering.
     *
     * @param input The packing input
     * @param shuffle_seed If nullopt, uses FFD ordering; otherwise shuffles tiles with the given seed
     * @return A PackingOutput on success, or an error if packing fails
     */
    [[nodiscard]] ChainableResult<PackingOutput>
    try_pack(const PackingInput &input, std::optional<std::uint64_t> shuffle_seed) const;
};

} // namespace porytiles2
