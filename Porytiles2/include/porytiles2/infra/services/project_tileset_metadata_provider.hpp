#pragma once

#include <filesystem>
#include <map>

#include "gsl/pointers"

#include "porytiles2/domain/services/tileset_metadata_provider.hpp"
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Filesystem-based implementation of TilesetMetadataProvider.
 *
 * @details
 * ProjectTilesetMetadataProvider parses C source files from a pokeemerald-style project to discover
 * tileset metadata and artifact paths. It parses:
 *
 * - `src/data/tilesets/headers.h` - For tileset struct definitions (isSecondary, variable references)
 * - `src/data/tilesets/graphics.h` - For INCBIN declarations (tiles, palettes)
 * - `src/data/tilesets/metatiles.h` - For INCBIN declarations (metatiles, attributes)
 *
 * The provider uses lazy loading with caching - files are parsed on first access and results
 * are cached for subsequent queries.
 *
 * Example:
 * @code
 * ProjectTilesetMetadataProvider provider{project_root, &formatter, &diagnostics};
 *
 * auto name = TilesetName::from("gTileset_General").value();
 * auto metadata = provider.metadata_for(name);
 * if (metadata.has_value()) {
 *     bool is_secondary = metadata.value().is_secondary();  // false
 *     std::string tiles_var = metadata.value().tiles_var(); // "gTilesetTiles_General"
 * }
 * @endcode
 *
 * @see TilesetMetadataProvider for the interface definition
 */
class ProjectTilesetMetadataProvider final : public TilesetMetadataProvider {
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
     * Parses headers.h on first call and caches results. Returns the parsed struct fields
     * for the specified tileset.
     *
     * @param tileset_name The tileset to look up
     * @return TilesetMetadata on success, or an error if the tileset is not found
     */
    [[nodiscard]] ChainableResult<TilesetMetadata> metadata_for(const TilesetName &tileset_name) const override;

    /**
     * @brief Retrieves resolved artifact paths for a specific tileset.
     *
     * @details
     * Parses graphics.h and metatiles.h on first call and caches results. Uses the variable
     * names from metadata to look up INCBIN paths.
     *
     * @param tileset_name The tileset to look up
     * @return TilesetArtifactPaths on success, or an error if paths cannot be resolved
     */
    [[nodiscard]] ChainableResult<TilesetArtifactPaths>
    artifact_paths_for(const TilesetName &tileset_name) const override;

    /**
     * @brief Checks if a tileset exists in the project.
     *
     * @details
     * Checks if the tileset is declared in headers.h.
     *
     * @param tileset_name The tileset to check
     * @return true if the tileset exists, false otherwise
     */
    [[nodiscard]] ChainableResult<bool> tileset_exists(const TilesetName &tileset_name) const override;

    /**
     * @brief Retrieves animation frame paths for a specific tileset.
     *
     * @details
     * Discovers animation frame INCBIN paths from C source files. Priority:
     * 1. If `<tileset_root>/include/generated_anim_code.h` exists, parse it
     * 2. Otherwise, parse `src/tileset_anims.c`
     *
     * @param tileset_name The tileset to look up
     * @return AnimationFramePaths on success (may be empty if no animations)
     */
    [[nodiscard]] ChainableResult<AnimationFramePaths>
    animation_frame_paths_for(const TilesetName &tileset_name) const override;

  private:
    [[nodiscard]] ChainableResult<void> ensure_headers_parsed() const;
    [[nodiscard]] ChainableResult<void> ensure_incbins_parsed() const;

    [[nodiscard]] ChainableResult<std::string> lookup_incbin_path(const std::string &variable_name) const;

    [[nodiscard]] ChainableResult<AnimationFramePaths> parse_anim_incbins_from_file(
        const std::filesystem::path &c_file, const std::string &tileset_shorthand, bool porytiles_managed) const;
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
