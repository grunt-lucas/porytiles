#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/app/config/app_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles2/domain/services/primary_tileset_importer.hpp"
#include "porytiles2/domain/services/tileset_metadata_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Use case for importing a primary Tileset.
 */
class ImportPrimaryTileset {
  public:
    /**
     * @brief Constructs an ImportPrimaryTileset use case with the given repositories and services.
     */
    ImportPrimaryTileset(
        gsl::not_null<const PrimaryTilesetImporter *> importer,
        gsl::not_null<const PrimaryTilesetDecompiler *> decompiler,
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : importer_{importer}, decompiler_{decompiler}, tileset_repo_{tileset_repo},
          metadata_provider_{metadata_provider}, tileset_manager_{tileset_manager}, domain_config_{domain_config},
          app_config_{app_config}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<void> import(const std::string &tileset_name) const;

  private:
    const PrimaryTilesetImporter *importer_;
    const PrimaryTilesetDecompiler *decompiler_;
    const TilesetRepo *tileset_repo_;
    const TilesetMetadataProvider *metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
