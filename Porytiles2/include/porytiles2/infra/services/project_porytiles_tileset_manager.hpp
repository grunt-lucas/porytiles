#pragma once

#include <filesystem>

#include "porytiles2/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles2/infra/models/original_artifacts.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Manages Porytiles-owned tilesets via OriginalArtifacts JSON files.
 *
 * @details
 * This class serializes and deserializes `original_artifacts.json` files in the `porytiles/tilesets/{tileset_name}/`
 * directory. The presence of this file indicates that a tileset is Porytiles-managed.
 *
 * The JSON format varies based on whether the tileset was imported from vanilla pokeemerald:
 * - **Imported tilesets**: All original field values are stored for restoration support
 * - **Created tilesets**: Only version and imported flag are stored
 *
 * @see OriginalArtifacts for the model class
 * @see PorytilesTilesetManager for the abstract interface
 */
class ProjectPorytilesTilesetManager : public PorytilesTilesetManager {
  public:
    /**
     * @brief Constructs a ProjectPorytilesTilesetManager with required dependencies.
     *
     * @param project_root Path to the pokeemerald project root directory
     * @param metadata_writer Provider for reading headers.h fields
     * @param metadata_writer Writer for updating headers.h fields
     */
    ProjectPorytilesTilesetManager(
        std::filesystem::path project_root,
        const ProjectTilesetMetadataProvider *metadata_provider,
        const ProjectTilesetMetadataWriter *metadata_writer)
        : project_root_{std::move(project_root)}, metadata_provider_{metadata_provider},
          metadata_writer_{metadata_writer}
    {
    }

    /**
     * @brief Reads an OriginalArtifacts object from the porytiles utility directory.
     *
     * @details
     * Looks for `original_artifacts.json` at `{project_root}/porytiles/tilesets/{tileset_name}/`. Returns an error if
     * the file doesn't exist or contains invalid JSON.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return The deserialized OriginalArtifacts, or an error if the file doesn't exist or is invalid
     */
    [[nodiscard]] ChainableResult<OriginalArtifacts> read(const std::string &tileset_name) const;

    /**
     * @brief Writes an OriginalArtifacts object to the porytiles utility directory.
     *
     * @details
     * Creates the directory structure `porytiles/tilesets/{tileset_name}/` if it doesn't exist, then writes
     * `original_artifacts.json` with 2-space indented JSON formatting.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @param artifacts The OriginalArtifacts data to serialize
     * @post `original_artifacts.json` exists at `{project_root}/porytiles/tilesets/{tileset_name}/`
     */
    void write(const std::string &tileset_name, const OriginalArtifacts &artifacts) const;

    /**
     * @brief Checks whether a tileset has an original_artifacts.json file.
     *
     * @details
     * This is the canonical way to check if a tileset is Porytiles-managed.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return true if the tileset has an original_artifacts.json file
     */
    [[nodiscard]] bool is_porytiles_managed(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<void> persist_managed_state(const std::string &tileset_name) const override;

  private:
    std::filesystem::path project_root_;
    const ProjectTilesetMetadataProvider *metadata_provider_;
    const ProjectTilesetMetadataWriter *metadata_writer_;
};

} // namespace porytiles2
