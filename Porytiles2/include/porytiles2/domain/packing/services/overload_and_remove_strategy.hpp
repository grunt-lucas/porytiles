#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

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
    /**
     * @brief Constructs a OverloadAndRemoveStrategy with the specified dependencies.
     *
     * @param format TextFormatter for building diagnostic output
     * @param diag UserDiagnostics for warnings and errors
     */
    explicit OverloadAndRemoveStrategy(
        gsl::not_null<const TextFormatter *> format, gsl::not_null<const UserDiagnostics *> diag)
        : format_{format}, diag_{diag}
    {
    }

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

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
