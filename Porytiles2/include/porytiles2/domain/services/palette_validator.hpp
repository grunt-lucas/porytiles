#pragma once

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief A collection of palette validation functions for compilation operations.
 *
 * @details
 * PaletteValidator validates Porytiles palette overrides, original Porymap palettes,
 * and palette hints before they are used in tileset compilation. This ensures that
 * palettes meet the requirements of the GBA hardware and Porytiles compilation process.
 *
 * Validations include:
 * - Override and Porymap palettes: non-slot-0 positions cannot contain extrinsic transparency
 * - Override and Porymap palettes: slot 0 should match extrinsic transparency (warning)
 * - Palette hints cannot contain duplicate colors or extrinsic transparency
 */
class PaletteValidator {
  public:
    explicit PaletteValidator(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const PalettePrinter *> pal_printer,
        gsl::not_null<const DomainConfig *> config,
        std::string tileset_scope)
        : format_{format}, diag_{diag}, pal_printer_{pal_printer}, config_{config},
          tileset_scope_{std::move(tileset_scope)}
    {
    }

    /**
     * @brief Validates all palettes for a primary tileset compilation.
     *
     * @details
     * Runs all palette validations in sequence and collates any errors:
     * - Porymap palette validation (extrinsic transparency checks)
     * - Porytiles override palette validation (extrinsic transparency checks)
     * - Palette hint validation (duplicate colors, extrinsic transparency)
     *
     * @param porymap_pals The array of original Porymap palettes
     * @param overrides The array of optional palette overrides from PorytilesTilesetComponent
     * @param hints The vector of palette hints
     * @return ChainableResult<void> indicating success or validation failure with all errors
     */
    [[nodiscard]] ChainableResult<void> validate_primary(
        const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &porymap_pals,
        const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &overrides,
        const std::vector<PaletteHint> &hints) const;

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const PalettePrinter *pal_printer_;
    const DomainConfig *config_;
    std::string tileset_scope_;
};

} // namespace porytiles2
