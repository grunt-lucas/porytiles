#pragma once

#include <string>

#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief TODO
 *
 * @details
 * TODO
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
     * @brief Packs tiles into palettes using the Overload-And-Remove algorithm.
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
};

} // namespace porytiles2
