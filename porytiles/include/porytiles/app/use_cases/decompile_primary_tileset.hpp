#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles/app/config/app_config.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Use case for decompiling a primary Tileset.
class DecompilePrimaryTileset {
  public:
    DecompilePrimaryTileset(
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const PrimaryTilesetDecompiler *> decompiler,
        gsl::not_null<const TilesetCompiler *> compiler,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const UserDiagnostics *> diag)
        : tileset_repo_{tileset_repo}, decompiler_{decompiler}, compiler_{compiler},
          metadata_provider_{metadata_provider}, tileset_manager_{tileset_manager}, domain_config_{domain_config},
          app_config_{app_config}, diag_{diag}
    {
    }

    /// @brief Decompiles the primary Tileset with the given tileset name.
    ///
    /// @param tileset_name The name of the primary Tileset to decompile
    /// @return An empty ChainableResult on success, otherwise an error chain
    [[nodiscard]] ChainableResult<void> decompile(const std::string &tileset_name) const;

  private:
    const TilesetRepo *tileset_repo_;
    const PrimaryTilesetDecompiler *decompiler_;
    const TilesetCompiler *compiler_;
    const TilesetMetadataProvider *metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
