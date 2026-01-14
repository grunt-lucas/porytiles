#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/app/config/app_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Use case for decompiling a primary Tileset.
 */
class DecompilePrimaryTileset {
  public:
    DecompilePrimaryTileset(
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const PrimaryTilesetDecompiler *> decompiler,
        gsl::not_null<const PrimaryTilesetCompiler *> compiler,
        gsl::not_null<const TilesetMetadataProvider *> tileset_metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> porytiles_tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : tileset_repo_{tileset_repo}, decompiler_{decompiler}, compiler_{compiler},
          tileset_metadata_provider_{tileset_metadata_provider}, porytiles_tileset_manager_{porytiles_tileset_manager},
          domain_config_{domain_config}, app_config_{app_config}, format_{format}, diag_{diag}
    {
    }

    /**
     * @brief Decompiles the primary Tileset with the given tileset name.
     *
     * @param name The name of the primary Tileset to decompile
     * @return An empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] ChainableResult<void> decompile(const std::string &name) const;

  private:
    const TilesetRepo *tileset_repo_;
    const PrimaryTilesetDecompiler *decompiler_;
    const PrimaryTilesetCompiler *compiler_;
    const TilesetMetadataProvider *tileset_metadata_provider_;
    const PorytilesTilesetManager *porytiles_tileset_manager_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
