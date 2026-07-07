#pragma once

#include <filesystem>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/infra/config/infra_config.hpp"
#include "porytiles/infra/models/tileset_manifest.hpp"
#include "porytiles/infra/services/incbin_declaration_appender.hpp"
#include "porytiles/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/**
 * @brief Manages Porytiles-owned tilesets via TilesetManifest JSON files.
 *
 * @details
 * This class serializes and deserializes `tileset-manifest.json` files in the `porytiles/tilesets/{tileset_name}/`
 * directory. The presence of this file indicates that a tileset is Porytiles-managed.
 *
 * The JSON format varies based on whether the tileset was imported from vanilla pokeemerald:
 * - **Imported tilesets**: All original field values are stored for restoration support
 * - **Created tilesets**: Only version and imported flag are stored
 *
 * @see TilesetManifest for the model class
 * @see PorytilesTilesetManager for the abstract interface
 */
class ProjectPorytilesTilesetManager : public PorytilesTilesetManager {
  public:
    /**
     * @brief Constructs a ProjectPorytilesTilesetManager with required dependencies.
     *
     * @param project_root Path to the pokeemerald project root directory
     * @param metadata_provider Provider for reading headers.h fields
     * @param metadata_writer Writer for updating headers.h fields
     * @param infra_config Configuration provider for tileset paths and animation settings
     * @param schema The resolved metatile attribute schema; its attr_bytes() selects the generated INCBIN
     * declaration type, so pass the schema resolver's product, whose width always covers the resolved masks
     * @param diag Diagnostics interface for warnings/notes
     * @param incbin_appender Service for appending INCBIN declarations to header files
     * @param tileset_anims_modifier Service for modifying tileset_anims.c includes
     */
    ProjectPorytilesTilesetManager(
        std::filesystem::path project_root,
        const ProjectTilesetMetadataProvider *metadata_provider,
        const ProjectTilesetMetadataWriter *metadata_writer,
        const InfraConfig *infra_config,
        gsl::not_null<const Schema *> schema,
        gsl::not_null<const UserDiagnostics *> diag,
        const IncbinDeclarationAppender *incbin_appender,
        const ProjectTilesetAnimsModifier *tileset_anims_modifier)
        : project_root_{std::move(project_root)}, metadata_provider_{metadata_provider},
          metadata_writer_{metadata_writer}, infra_config_{infra_config}, schema_{schema}, diag_{diag},
          incbin_appender_{incbin_appender}, tileset_anims_modifier_{tileset_anims_modifier}
    {
    }

    /**
     * @brief Reads an TilesetManifest object from the porytiles utility directory.
     *
     * @details
     * Looks for `tileset-manifest.json` at `{project_root}/porytiles/tilesets/{tileset_name}/`. Returns an error if
     * the file doesn't exist or contains invalid JSON.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return The deserialized TilesetManifest, or an error if the file doesn't exist or is invalid
     */
    [[nodiscard]] ChainableResult<TilesetManifest> read(const std::string &tileset_name) const;

    /**
     * @brief Writes an TilesetManifest object to the porytiles utility directory.
     *
     * @details
     * Creates the directory structure `porytiles/tilesets/{tileset_name}/` if it doesn't exist, then writes
     * `tileset-manifest.json` with 2-space indented JSON formatting.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @param artifacts The TilesetManifest data to serialize
     * @post `tileset-manifest.json` exists at `{project_root}/porytiles/tilesets/{tileset_name}/`
     */
    void write(const std::string &tileset_name, const TilesetManifest &artifacts) const;

    /**
     * @brief Checks whether a tileset has an tileset-manifest.json file.
     *
     * @details
     * This is the canonical way to check if a tileset is Porytiles-managed.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return true if the tileset has an tileset-manifest.json file
     */
    [[nodiscard]] bool is_porytiles_managed(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<void> persist_managed_existing(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<void>
    persist_managed_new(const std::string &tileset_name, bool is_secondary = false) const override;

    [[nodiscard]] ChainableResult<void>
    wire_anim_code(const std::string &tileset_name, bool is_secondary) const override;

    [[nodiscard]] ChainableResult<void>
    remove_wired_anim_code(const std::string &tileset_name, bool is_secondary) const override;

  private:
    std::filesystem::path project_root_;
    const ProjectTilesetMetadataProvider *metadata_provider_;
    const ProjectTilesetMetadataWriter *metadata_writer_;
    const InfraConfig *infra_config_;
    const Schema *schema_;
    const UserDiagnostics *diag_;
    const IncbinDeclarationAppender *incbin_appender_;
    const ProjectTilesetAnimsModifier *tileset_anims_modifier_;
};

} // namespace porytiles
