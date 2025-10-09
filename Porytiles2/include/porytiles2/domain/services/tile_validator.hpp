#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/models/rgba_tile.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief A collection of tile validation functions for compilation operations.
 */
class TileValidator {
  public:
    explicit TileValidator(gsl::not_null<TextFormatter *> format, gsl::not_null<UserDiagnostics *> diag)
        : format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<void> validate_alpha_channels(const std::vector<RgbaTile> &tiles) const;

    [[nodiscard]] ChainableResult<void> validate_unique_color_count(const std::vector<RgbaTile> &tiles) const;

    [[nodiscard]] ChainableResult<void> generate_precision_loss_warnings(const std::vector<RgbaTile> &tiles) const;

  private:
    TextFormatter *format_;
    UserDiagnostics *diag_;
};

} // namespace porytiles2
