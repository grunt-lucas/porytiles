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

/**
 * @brief Service that compiles a Tileset (primary or secondary).
 *
 * @details
 * Supports three compilation modes via the @p is_secondary and @p paired_primary parameters:
 * - Primary: @p is_secondary is @c false (the default). @p paired_primary is ignored.
 * - Secondary with paired primary: @p is_secondary is @c true, @p paired_primary points to a compiled primary Tileset.
 *   The compiler uses the paired primary's tiles and palettes to produce correct global indices.
 * - Standalone secondary: @p is_secondary is @c true, @p paired_primary is @c nullptr. The compiler produces secondary
 *   compilation output without referencing any primary tileset data.
 */
class TilesetCompiler {
  public:
    explicit TilesetCompiler(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    /**
     * @brief Compiles the given Tileset, producing a new Tileset with compiled Porymap assets.
     *
     * @param tileset The Tileset to compile.
     * @param is_secondary Whether this is a secondary tileset compilation.
     * @param paired_primary The compiled paired primary Tileset for secondary compilation, or @c nullptr.
     * @return A new compiled Tileset on success, or an error chain on failure.
     */
    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>>
    compile(const Tileset &tileset, bool is_secondary = false, const Tileset *paired_primary = nullptr) const;

  private:
    const DomainConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles
