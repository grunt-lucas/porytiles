#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles/app/config/app_config.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/layout_metadata_provider.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/domain/services/tileset_creator.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Use case for creating a new secondary Tileset from scratch.
///
/// @details
/// This use case creates a brand new Porytiles-managed secondary tileset. Unlike import, which converts existing
/// vanilla pokeemerald tilesets, this creates a tileset from nothing with empty assets and a paired primary.
///
/// The workflow:
/// 1. Check if tileset already exists (error if so)
/// 2. Resolve the partner primary tileset via configured pairing mode
/// 3. Create blank PorytilesTilesetComponent via TilesetCreator (secondary variant)
/// 4. Create blank PorymapTilesetComponent and wrap in Tileset
/// 5. Compile (generates minimal valid Porymap assets, paired with primary)
/// 6. Persist managed state with is_secondary=true (creates headers.h entry + manifest)
/// 7. Save via TilesetRepo
/// 8. Wire animation code if needed
class CreateSecondaryTileset {
  public:
    /// @brief Constructs a CreateSecondaryTileset use case with the given dependencies.
    ///
    /// @param creator Service for creating blank secondary PorytilesTilesetComponent
    /// @param compiler Service for compiling PorytilesTilesetComponent to PorymapTilesetComponent
    /// @param tileset_repo Repository for persisting Tileset aggregates
    /// @param metadata_provider Provider for checking if tilesets exist
    /// @param layout_metadata_provider Provider for scanning project layouts (used in automatic primary pairing)
    /// @param tileset_manager Service for persisting managed state and headers.h entries
    /// @param domain_config Configuration for domain layer operations
    /// @param app_config Configuration for app layer operations
    /// @param diag User diagnostics for warnings and errors
    CreateSecondaryTileset(
        gsl::not_null<const TilesetCreator *> creator,
        gsl::not_null<const TilesetCompiler *> compiler,
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const LayoutMetadataProvider *> layout_metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const UserDiagnostics *> diag)
        : creator_{creator}, compiler_{compiler}, tileset_repo_{tileset_repo}, metadata_provider_{metadata_provider},
          layout_metadata_provider_{layout_metadata_provider}, tileset_manager_{tileset_manager},
          domain_config_{domain_config}, app_config_{app_config}, diag_{diag}
    {
    }

    /// @brief Creates a new secondary Tileset with the given name.
    ///
    /// @details
    /// Creates a brand-new secondary tileset from scratch. The tileset will have:
    /// - A new entry in src/data/tilesets/headers.h with Porytiles-managed field values and is_secondary=TRUE
    /// - Minimal valid Porymap assets (metatiles.bin, metatile_attributes.bin, tiles.png, palettes)
    /// - A tileset-manifest.json marking it as Porytiles-managed (imported=false)
    ///
    /// @param tileset_name The name for the new tileset (e.g., "gTileset_MyNewSecondary")
    /// @pre name must not correspond to an existing tileset in headers.h
    /// @return Success or error result with details
    [[nodiscard]] ChainableResult<void> create(const std::string &tileset_name) const;

  private:
    const TilesetCreator *creator_;
    const TilesetCompiler *compiler_;
    const TilesetRepo *tileset_repo_;
    const TilesetMetadataProvider *metadata_provider_;
    const LayoutMetadataProvider *layout_metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
