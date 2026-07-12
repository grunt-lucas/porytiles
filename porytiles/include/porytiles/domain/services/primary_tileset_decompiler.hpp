#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

class PrimaryTilesetDecompiler {
  public:
    explicit PrimaryTilesetDecompiler(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> palette_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, palette_printer_{palette_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> decompile(const Tileset &tileset) const;

  private:
    const DomainConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *palette_printer_;
};

} // namespace porytiles
