/**
 * @file defunct_primary_tileset_importer.hpp
 * @deprecated This file contains the legacy import implementation.
 * A new import system is being developed that separates "import" (vanilla migration)
 * from "decompile" (Porymap → Porytiles transformation). See project_structure_refactoring_plan.md.
 */
#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Service that imports a primary Tileset.
 * @deprecated See defunct_primary_tileset_importer.hpp file comment.
 */
class DefunctPrimaryTilesetImporter {
  public:
    explicit DefunctPrimaryTilesetImporter(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> import(const Tileset &tileset) const;

  private:
    const DomainConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles2
