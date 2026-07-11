#pragma once

#include <string>

#include "porytiles/domain/packing/services/packing_strategy.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Greedy palette packing algorithm using weighted cost optimization.
///
/// @details
/// BestFusionStrategy implements a greedy algorithm for palette packing based on the "Best Fusion" approach. For each
/// tile, it computes the "weighted cost" of placing the tile in each candidate palette and selects the one with minimal
/// cost.
///
/// The weighted cost for placing a tile in a palette is computed as:
///   sum(1 / multiplicity[color]) for each color in the tile
///
/// where multiplicity[color] is the number of tiles that use that color. This rewards placing tiles in palettes where
/// their colors are shared by many other tiles, maximizing color reuse and minimizing the number of palettes needed.
///
/// If the weighted cost is less than the tile's color count (indicating good color sharing), the tile is added to the
/// best palette. Otherwise, a new palette is created.
///
/// This algorithm is based on insights and samples from:
///
/// Grange, A., Kacem, I., & Martin, S. (2018). Algorithms for the bin packing problem with overlapping items. Computers
/// & Industrial Engineering, 115, 331–341. https://doi.org/10.1016/j.cie.2017.10.015 https://arxiv.org/pdf/1605.00558
class BestFusionStrategy final : public PackingStrategy {
  public:
    BestFusionStrategy() = default;

    /// @brief Packs tiles into palettes using the Best Fusion algorithm.
    ///
    /// @param input The packing input containing tiles, hints, fixed slots, and constraints
    /// @return A PackingOutput on success, or an error if packing fails
    [[nodiscard]] ChainableResult<PackingOutput> pack(const PackingInput &input) const override;

    /// @brief Returns the name of this strategy.
    ///
    /// @return "Best Fusion"
    [[nodiscard]] std::string name() const override
    {
        return "Best Fusion";
    }
};

} // namespace porytiles
