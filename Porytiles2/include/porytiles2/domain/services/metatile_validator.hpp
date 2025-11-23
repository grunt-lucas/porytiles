#pragma once

#include <string>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief A collection of tile validation functions for compilation operations.
 */
class MetatileValidator {
    friend class MetatileValidatorTestAccess;

  public:
    explicit MetatileValidator(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer,
        gsl::not_null<const DomainConfig *> config,
        std::string tileset_scope)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}, config_{config},
          tileset_scope_{std::move(tileset_scope)}
    {
    }

    [[nodiscard]] ChainableResult<void> validate_primary(const std::vector<Metatile<Rgba32>> &metatiles) const;

  private:
    [[nodiscard]] ChainableResult<void> validate_alpha_channels(const std::vector<Metatile<Rgba32>> &metatiles) const;

    [[nodiscard]] ChainableResult<void> validate_tile_color_count(const std::vector<Metatile<Rgba32>> &metatiles) const;

    [[nodiscard]] ChainableResult<void>
    validate_global_color_count(const std::vector<Metatile<Rgba32>> &metatiles, std::size_t count_limit) const;

    [[nodiscard]] ChainableResult<void>
    validate_layer_mode(const std::vector<Metatile<Rgba32>> &metatiles, LayerMode mode) const;

    [[nodiscard]] ChainableResult<void>
    generate_precision_loss_warnings(const std::vector<Metatile<Rgba32>> &metatiles) const;

    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
    const DomainConfig *config_;
    std::string tileset_scope_;
};

} // namespace porytiles2
