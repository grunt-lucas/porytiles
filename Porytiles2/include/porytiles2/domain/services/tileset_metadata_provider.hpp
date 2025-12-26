#pragma once

#include "porytiles2/domain/models/tileset_artifact_paths.hpp"
#include "porytiles2/domain/models/tileset_metadata.hpp"
#include "porytiles2/domain/models/tileset_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for retrieving tileset metadata and artifact paths.
 *
 * @details
 * TilesetMetadataProvider defines the contract for discovering tileset information from a project. Implementations
 * parse source files (like headers.h and graphics.h) to extract:
 * - Tileset metadata (is_secondary, variable references, callback functions)
 * - Artifact paths (resolved INCBIN file paths for tiles, palettes, metatiles)
 *
 * This interface enables dynamic artifact path discovery instead of hardcoded path assumptions, supporting projects
 * with non-standard tileset organizations.
 *
 * Example usage:
 * @code
 * auto metadata = provider.metadata_for(tileset_name);
 * if (metadata.has_value()) {
 *     if (metadata.value().is_secondary()) {
 *         // Handle secondary tileset...
 *     }
 * }
 *
 * auto paths = provider.artifact_paths_for(tileset_name);
 * if (paths.has_value()) {
 *     auto tiles_file = paths.value().tiles_path();
 *     // Use resolved path...
 * }
 * @endcode
 *
 * @see ProjectTilesetMetadataProvider for the filesystem-based implementation
 */
class TilesetMetadataProvider {
  public:
    virtual ~TilesetMetadataProvider() = default;

    /**
     * @brief Retrieves metadata for a specific tileset.
     *
     * @details
     * Returns the parsed struct fields for the specified tileset from headers.h, including:
     * - Whether it's primary or secondary (is_secondary)
     * - Variable references for tiles, palettes, metatiles, attributes
     * - Animation callback function name (if present)
     *
     * @param tileset_name The tileset to look up
     * @return TilesetMetadata on success, or an error if the tileset is not found or parsing fails
     */
    [[nodiscard]] virtual ChainableResult<TilesetMetadata> metadata_for(const TilesetName &tileset_name) const = 0;

    /**
     * @brief Retrieves resolved artifact paths for a specific tileset.
     *
     * @details
     * Returns the actual filesystem paths for tileset artifacts by looking up INCBIN declarations in graphics.h and
     * metatiles.h. The paths are relative to the project root.
     *
     * @param tileset_name The tileset to look up
     * @return TilesetArtifactPaths on success, or an error if paths cannot be resolved
     */
    [[nodiscard]] virtual ChainableResult<TilesetArtifactPaths>
    artifact_paths_for(const TilesetName &tileset_name) const = 0;

    /**
     * @brief Checks if a tileset exists in the project.
     *
     * @details
     * Verifies whether the specified tileset is declared in headers.h. This is a lightweight check that doesn't require
     * resolving artifact paths.
     *
     * @param tileset_name The tileset to check
     * @return true if the tileset exists, false otherwise, or an error on parse failure
     */
    [[nodiscard]] virtual ChainableResult<bool> tileset_exists(const TilesetName &tileset_name) const = 0;

    /**
     * @brief Retrieves animation frame paths for a specific tileset.
     *
     * @details
     * Discovers animation frame INCBIN paths from C source files. Uses the following priority:
     * 1. If `<tileset_root>/include/generated_anim_code.h` exists, parse it (Porytiles-managed)
     * 2. Otherwise, parse `src/tileset_anims.c` (vanilla pokeemerald)
     *
     * The tileset's callback function name (from TilesetMetadata) is used to filter animations belonging to this
     * specific tileset.
     *
     * @param tileset_name The tileset to look up
     * @return AnimationFramePaths on success (may be empty if no animations),
     *         or an error if parsing fails
     */
    [[nodiscard]] virtual ChainableResult<AnimationFramePaths>
    animation_frame_paths_for(const TilesetName &tileset_name) const = 0;
};

} // namespace porytiles2
