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
 * @brief Service that compiles a primary Tileset.
 */
class PrimaryTilesetCompiler {
  public:
    explicit PrimaryTilesetCompiler(
        gsl::not_null<DomainConfig *> config,
        gsl::not_null<TextFormatter *> format,
        gsl::not_null<UserDiagnostics *> diag,
        gsl::not_null<TilePrinter *> tile_printer,
        gsl::not_null<PalettePrinter *> pal_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile(const Tileset &tileset);

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>>
    compile_patch_tiles_fixed_pals_fixed(const Tileset &tileset);

  private:
    DomainConfig *config_;
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
    PalettePrinter *pal_printer_;
};

} // namespace porytiles2
