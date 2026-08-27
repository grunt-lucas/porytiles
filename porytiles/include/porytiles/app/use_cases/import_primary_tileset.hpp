#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles/app/config/app_config.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles/domain/services/primary_tileset_importer.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Use case for importing a primary Tileset.
class ImportPrimaryTileset {
  public:
    /// @brief Constructs an ImportPrimaryTileset use case with the given repositories and services.
    ImportPrimaryTileset(
        gsl::not_null<const PrimaryTilesetImporter *> importer,
        gsl::not_null<const PrimaryTilesetDecompiler *> decompiler,
        gsl::not_null<const TilesetCompiler *> compiler,
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const UserDiagnostics *> diag)
        : importer_{importer}, decompiler_{decompiler}, compiler_{compiler}, tileset_repo_{tileset_repo},
          metadata_provider_{metadata_provider}, tileset_manager_{tileset_manager}, domain_config_{domain_config},
          app_config_{app_config}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<void> import(const std::string &tileset_name) const;

  private:
    const PrimaryTilesetImporter *importer_;
    const PrimaryTilesetDecompiler *decompiler_;
    const TilesetCompiler *compiler_;
    const TilesetRepo *tileset_repo_;
    const TilesetMetadataProvider *metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
