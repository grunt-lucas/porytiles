#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/app/config/app_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/primary_tileset_creator.hpp"
#include "porytiles2/domain/services/tileset_metadata_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Use case for creating a new primary Tileset from scratch.
 *
 * @details
 * This use case creates a brand new Porytiles-managed primary tileset. Unlike import, which converts existing
 * vanilla pokeemerald tilesets, this creates a tileset from nothing with empty assets.
 *
 * The workflow:
 * 1. Check if tileset already exists (error if so)
 * 2. Create blank PorytilesTilesetComponent via PrimaryTilesetCreator
 * 3. Create blank PorymapTilesetComponent and wrap in Tileset
 * 4. Compile (generates minimal valid Porymap assets)
 * 5. Save via TilesetRepo
 * 6. Persist managed state with brand_new=true (creates headers.h entry + manifest)
 */
class CreatePrimaryTileset {
  public:
    /**
     * @brief Constructs a CreatePrimaryTileset use case with the given dependencies.
     *
     * @param creator Service for creating blank PorytilesTilesetComponent
     * @param compiler Service for compiling PorytilesTilesetComponent to PorymapTilesetComponent
     * @param tileset_repo Repository for persisting Tileset aggregates
     * @param metadata_provider Provider for checking if tilesets exist
     * @param tileset_manager Service for persisting managed state and headers.h entries
     * @param domain_config Configuration for domain layer operations
     * @param app_config Configuration for app layer operations
     * @param diag User diagnostics for warnings and errors
     */
    CreatePrimaryTileset(
        gsl::not_null<const PrimaryTilesetCreator *> creator,
        gsl::not_null<const PrimaryTilesetCompiler *> compiler,
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const UserDiagnostics *> diag)
        : creator_{creator}, compiler_{compiler}, tileset_repo_{tileset_repo}, metadata_provider_{metadata_provider},
          tileset_manager_{tileset_manager}, domain_config_{domain_config}, app_config_{app_config}, diag_{diag}
    {
    }

    /**
     * @brief Creates a new primary Tileset with the given name.
     *
     * @details
     * Creates a brand-new primary tileset from scratch. The tileset will have:
     * - A new entry in src/data/tilesets/headers.h with Porytiles-managed field values
     * - Minimal valid Porymap assets (metatiles.bin, metatile_attributes.bin, tiles.png, palettes)
     * - A tileset-manifest.json marking it as Porytiles-managed (imported=false)
     *
     * @param tileset_name The name for the new tileset (e.g., "gTileset_MyNewTileset")
     * @pre name must not correspond to an existing tileset in headers.h
     * @return Success or error result with details
     */
    [[nodiscard]] ChainableResult<void> create(const std::string &tileset_name) const;

  private:
    const PrimaryTilesetCreator *creator_;
    const PrimaryTilesetCompiler *compiler_;
    const TilesetRepo *tileset_repo_;
    const TilesetMetadataProvider *metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
