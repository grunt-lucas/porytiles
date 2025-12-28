#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "animation_callback_info.hpp"
#include "porytiles2/domain/models/tileset_metadata.hpp"
#include "porytiles2/infra/repos//tileset_artifact_paths.hpp"
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/*
 * TODO: ANIM: move this class's functionality entirely into ProjectTilesetArtifactKeyProvider. It doesn't need to be
 * its own class. It doesn't even have a parent interface. This functionality is all related to artifact key discovery
 * via pokeemerald project tilesets/headers.h and related files, which is already the unique responsibility of
 * ProjectTilesetArtifactKeyProvider.
 */

/**
 * @brief Provides tileset metadata and artifact paths by parsing project source files.
 *
 * @details
 * ProjectTilesetMetadataProvider parses C source files from a pokeemerald-style project to discover tileset metadata
 * and artifact paths. It enables dynamic artifact path discovery instead of hardcoded path assumptions, supporting
 * projects with non-standard tileset organizations.
 *
 * It parses:
 * - `src/data/tilesets/headers.h` - For tileset struct definitions (isSecondary, variable references, callbacks)
 * - `src/data/tilesets/graphics.h` - For INCBIN declarations (tiles, palettes)
 * - `src/data/tilesets/metatiles.h` - For INCBIN declarations (metatiles, attributes)
 *
 * The provider uses lazy loading with caching - files are parsed on first access and results are cached for subsequent
 * queries.
 *
 * Example usage:
 * @code
 * ProjectTilesetMetadataProvider provider{project_root, &formatter, &diagnostics};
 *
 * auto name = TilesetName::from("gTileset_General").value();
 * auto metadata = provider.metadata_for(name);
 * if (metadata.has_value()) {
 *     if (metadata.value().is_secondary()) {
 *         // Handle secondary tileset...
 *     }
 * }
 *
 * auto paths = provider.artifact_paths_for(name);
 * if (paths.has_value()) {
 *     auto tiles_file = paths.value().tiles_path();
 *     // Use resolved path...
 * }
 * @endcode
 */
class ProjectTilesetMetadataProvider final {
  public:
    /**
     * @brief Constructs a provider for the specified project.
     *
     * @param project_root The root directory of the pokeemerald-style project
     * @param format Text formatter for styled output (non-owning, must outlive provider)
     * @param diag Diagnostics handler for warnings (non-owning, must outlive provider)
     */
    explicit ProjectTilesetMetadataProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    /**
     * @brief Retrieves metadata for a specific tileset.
     *
     * @details
     * Parses headers.h on first call and caches results. Returns the parsed struct fields for the specified tileset,
     * including:
     * - Whether it's primary or secondary (is_secondary)
     * - Variable references for tiles, palettes, metatiles, attributes
     * - Animation callback function name (if present)
     *
     * @param tileset_name The tileset to look up
     * @return TilesetMetadata on success, or an error if the tileset is not found or parsing fails
     */
    [[nodiscard]] ChainableResult<TilesetMetadata> metadata_for(const std::string &tileset_name) const;

    /**
     * @brief Retrieves resolved artifact paths for a specific tileset.
     *
     * @details
     * Returns the actual filesystem paths for tileset artifacts by looking up INCBIN declarations in graphics.h and
     * metatiles.h. The paths are relative to the project root. Parses these files on first call and caches results.
     * Uses the variable names from metadata to look up INCBIN paths.
     *
     * @param tileset_name The tileset to look up
     * @return TilesetArtifactPaths on success, or an error if paths cannot be resolved
     */
    [[nodiscard]] ChainableResult<TilesetArtifactPaths> artifact_paths_for(const std::string &tileset_name) const;

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
    [[nodiscard]] ChainableResult<bool> tileset_exists(const std::string &tileset_name) const;

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
     * @return AnimationFramePaths on success (may be empty if no animations), or an error if parsing fails
     */
    [[nodiscard]] ChainableResult<AnimationFramePaths> animation_frame_paths_for(const std::string &tileset_name) const;

    /**
     * @brief Retrieves animation callback information for a specific tileset.
     *
     * @details
     * Parses the tileset's callback function name from TilesetMetadata to extract:
     * - The callback function name (e.g., "InitTilesetAnim_General")
     * - The tileset shorthand name (e.g., "General")
     * - Whether it's Porytiles-managed
     * - The path to the C file containing animation code
     *
     * Returns nullopt if the tileset has no animations (callback is NULL or missing).
     *
     * This information can be passed to AnimCodeParser::parse_from_callback() to extract animation parameters.
     *
     * @param tileset_name The tileset to look up
     * @return AnimationCallbackInfo if the tileset has animations, nullopt if not, or an error if parsing fails
     */
    [[nodiscard]] ChainableResult<std::optional<AnimationCallbackInfo>>
    animation_callback_info_for(const std::string &tileset_name) const;

  private:
    [[nodiscard]] ChainableResult<void> ensure_headers_parsed() const;
    [[nodiscard]] ChainableResult<void> ensure_incbins_parsed() const;

    [[nodiscard]] ChainableResult<std::string> lookup_incbin_path(const std::string &variable_name) const;
    [[nodiscard]] ChainableResult<std::vector<std::string>> lookup_incbin_paths(const std::string &variable_name) const;

    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;

    // Lazy-loaded caches (mutable for const methods)
    mutable bool headers_parsed_{false};
    mutable bool incbins_parsed_{false};
    mutable std::map<std::string, StructInitializerDeclaration> tileset_structs_; // variable_name -> struct
    mutable std::map<std::string, IncbinDeclaration> incbin_vars_;                // variable_name -> incbin
};

} // namespace porytiles2
