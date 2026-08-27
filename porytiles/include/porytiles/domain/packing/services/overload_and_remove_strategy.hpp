#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/shuffle_strategy.hpp"
#include "porytiles/domain/packing/services/packing_strategy.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Packing strategy that assigns tiles to palettes using overload-and-remove with multi-start.
///
/// @details
/// Tiles are greedily assigned to the palette with the best color overlap. When a palette becomes overloaded (exceeds
/// its color capacity), the worst-fitting tile is removed and re-queued with that palette marked as forbidden,
/// guaranteeing termination. A multi-start loop retries with different tile orderings controlled by
/// @c ShuffleStrategy to escape local optima that defeat a single FFD pass.
///
/// The strategy supports two modes:
///
/// - **Preset matrix mode** (default): Iterates through 17 known-good configurations (varying shuffle strategies,
///   attempt counts, and seeds) until a solution is found. Configurations escalate from cheapest (single FFD) to most
///   expensive (75 random attempts). Use this when the input characteristics are unknown.
///
/// - **Single-config mode**: Runs a single multi-start search with caller-specified parameters. Use this when you know
///   which settings work for your input, or for targeted testing and tuning.
///
/// This algorithm is based on insights and samples from:
///
/// Grange, A., Kacem, I., & Martin, S. (2018). Algorithms for the bin packing problem with overlapping items. Computers
/// & Industrial Engineering, 115, 331–341. https://doi.org/10.1016/j.cie.2017.10.015 https://arxiv.org/pdf/1605.00558
class OverloadAndRemoveStrategy final : public PackingStrategy {
  public:
    OverloadAndRemoveStrategy() = default;

    /// @brief Constructs in preset matrix mode with diagnostics support.
    ///
    /// @details
    /// Uses the full 17-configuration preset matrix (default behavior) and emits a remark via the provided diagnostics
    /// when a successful configuration is found.
    ///
    /// @param diag Diagnostics interface for emitting remarks about successful search parameters.
    explicit OverloadAndRemoveStrategy(gsl::not_null<const UserDiagnostics *> diag) : diag_{diag} {}

    /// @brief Constructs with explicit parameters for single-configuration mode.
    ///
    /// @details
    /// When constructed with this constructor, the strategy runs a single multi-start search with the provided
    /// parameters instead of iterating through the preset configuration matrix.
    ///
    /// @param max_attempts Maximum number of packing attempts (first uses FFD, subsequent use shuffled orderings).
    /// @param seed Base seed for the PRNG used to generate shuffle seeds.
    /// @param shuffle_strategy Strategy for generating tile orderings in retry attempts.
    /// @param diag Optional diagnostics interface for emitting remarks about successful search parameters.
    explicit OverloadAndRemoveStrategy(
        std::size_t max_attempts,
        std::uint64_t seed,
        ShuffleStrategy shuffle_strategy,
        const UserDiagnostics *diag = nullptr)
        : use_preset_matrix_{false}, max_attempts_{max_attempts}, seed_{seed}, shuffle_strategy_{shuffle_strategy},
          diag_{diag}
    {
    }

    /// @brief Packs tiles into palettes using the Overload-And-Remove algorithm with multi-start.
    ///
    /// @details
    /// In preset matrix mode, tries 17 search configurations (varying shuffle strategies, attempt counts, and seeds)
    /// until a valid assignment is found. In single-config mode, runs one multi-start search with the configured
    /// parameters. Returns the first successful result, or an error if all configurations fail.
    ///
    /// @param input The packing input containing tiles, hints, fixed slots, and constraints.
    /// @return A PackingOutput on success, or an error if packing fails.
    [[nodiscard]] ChainableResult<PackingOutput> pack(const PackingInput &input) const override;

    /// @brief Returns the name of this strategy.
    ///
    /// @return "Overload-And-Remove"
    [[nodiscard]] std::string name() const override
    {
        return "Overload-And-Remove";
    }

  private:
    bool use_preset_matrix_ = true;
    std::size_t max_attempts_ = 20;
    std::uint64_t seed_ = 42;
    ShuffleStrategy shuffle_strategy_ = ShuffleStrategy::noisy_ffd;
    const UserDiagnostics *diag_ = nullptr;

    /// @brief Runs the multi-start O&R loop with the given parameters.
    ///
    /// @details
    /// Attempts packing with FFD ordering first. If that fails and max_attempts > 1, retries with shuffled tile
    /// orderings using a seeded PRNG. Returns the first successful result, or the first attempt's error if all fail.
    ///
    /// @param input The packing input.
    /// @param shuffle_strategy Strategy for generating tile orderings in retry attempts.
    /// @param max_attempts Maximum number of packing attempts.
    /// @param seed Base seed for the PRNG used to generate shuffle seeds.
    /// @return A PackingOutput on success, or an error if all attempts fail.
    [[nodiscard]] ChainableResult<PackingOutput> run_multi_start(
        const PackingInput &input,
        ShuffleStrategy shuffle_strategy,
        std::size_t max_attempts,
        std::uint64_t seed) const;

    /// @brief Performs a single packing attempt with the given tile ordering.
    ///
    /// @param input The packing input.
    /// @param shuffle_strategy Strategy for generating tile orderings (controls sort behavior after shuffle).
    /// @param shuffle_seed If nullopt, uses FFD ordering; otherwise shuffles tiles with the given seed.
    /// @return A PackingOutput on success, or an error if packing fails.
    [[nodiscard]] ChainableResult<PackingOutput> try_pack(
        const PackingInput &input, ShuffleStrategy shuffle_strategy, std::optional<std::uint64_t> shuffle_seed) const;
};

} // namespace porytiles
