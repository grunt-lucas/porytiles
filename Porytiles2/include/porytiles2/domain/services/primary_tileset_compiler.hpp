#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

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
        gsl::not_null<TilePrinter *> tile_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile(const Tileset &tileset);

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile_patch(const Tileset &tileset);

  private:
    DomainConfig *config_;
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
