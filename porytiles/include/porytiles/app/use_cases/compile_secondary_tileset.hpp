#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles/app/config/app_config.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/layout_metadata_provider.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/**
 * @brief Use case for compiling a secondary Tileset.
 *
 * @details
 * Orchestrates secondary tileset compilation by resolving the paired primary tileset (via automatic layout scanning,
 * manual configuration, or standalone mode), loading the necessary data, invoking the compiler, and persisting the
 * result. The pairing mode is controlled by the @c primary_pairing_mode configuration value.
 */
class CompileSecondaryTileset {
  public:
    /**
     * @brief Constructs a CompileSecondaryTileset use case with the given repositories and services.
     *
     * @param tileset_repo A pointer to the TilesetRepo for this use case.
     * @param compiler A pointer to the TilesetCompiler for this use case.
     * @param metadata_provider A pointer to the TilesetMetadataProvider for this use case.
     * @param layout_metadata_provider A pointer to the LayoutMetadataProvider for automatic primary resolution.
     * @param tileset_manager A pointer to the PorytilesTilesetManager for this use case.
     * @param domain_config A pointer to the DomainConfig for this use case.
     * @param app_config A pointer to the AppConfig for this use case.
     * @param diag A pointer to the UserDiagnostics for this use case.
     */
    CompileSecondaryTileset(
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetCompiler *> compiler,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const LayoutMetadataProvider *> layout_metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const UserDiagnostics *> diag)
        : tileset_repo_{tileset_repo}, compiler_{compiler}, metadata_provider_{metadata_provider},
          layout_metadata_provider_{layout_metadata_provider}, tileset_manager_{tileset_manager},
          domain_config_{domain_config}, app_config_{app_config}, diag_{diag}
    {
    }

    /**
     * @brief Compiles the secondary Tileset with the given tileset name.
     *
     * @details
     * Given a secondary tileset by name, resolves the paired primary tileset according to the configured pairing mode,
     * then compiles the PorytilesTileset assets into PorymapTileset assets. Uses the use case's configured repos to
     * load and save the tileset assets.
     *
     * @param tileset_name The name of the secondary Tileset to compile.
     * @pre @p tileset_name must refer to an existing tileset in the project metadata.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] ChainableResult<void> compile(const std::string &tileset_name) const;

  private:
    const TilesetRepo *tileset_repo_;
    const TilesetCompiler *compiler_;
    const TilesetMetadataProvider *metadata_provider_;
    const LayoutMetadataProvider *layout_metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
