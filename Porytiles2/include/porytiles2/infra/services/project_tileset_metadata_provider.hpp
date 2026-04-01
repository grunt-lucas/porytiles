#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "gsl/pointers"

#include "porytiles2/domain/services/tileset_metadata_provider.hpp"
#include "porytiles2/infra/models/project_tileset_metadata.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_paths.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Provides a pokeemerald project filesystem-based implementation for TilesetMetadataProvider.
 *
 * @details
 * This class parses tileset struct declarations from headers.h to extract metadata such as whether a tileset is
 * primary/secondary, what animation callback it uses, and the variable names for tiles, palettes, metatiles, and
 * attributes.
 *
 * Tileset struct metadata is lazy-loaded and cached for efficiency.
 */
class ProjectTilesetMetadataProvider : public TilesetMetadataProvider {
  public:
    ProjectTilesetMetadataProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] bool exists(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<bool> is_secondary(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<bool> has_animations(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>> tilesets() const override;

    /**
     * @brief Retrieves metadata for a specific tileset from the struct cache.
     *
     * @details
     * This method parses tileset struct declarations from headers.h to extract raw field values. The result
     * contains only the fields directly from the struct: name, is_secondary, variable names, and callback.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @pre tileset_name must refer to an existing tileset on disk
     * @return The parsed metadata for the tileset
     */
    [[nodiscard]] ChainableResult<ProjectTilesetMetadata> metadata_for(const std::string &tileset_name) const;

    /**
     * @brief Resolves artifact paths for a tileset by parsing INCBIN declarations.
     *
     * @details
     * This method retrieves tileset metadata (which contains INCBIN variable names), then looks up the
     * actual filesystem paths for those variables by parsing graphics.h, metatiles.h, and src/graphics.c.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @pre tileset_name must refer to an existing tileset on disk
     * @return The resolved artifact paths for the tileset
     */
    [[nodiscard]] ChainableResult<ProjectTilesetArtifactPaths>
    artifact_paths_for(const std::string &tileset_name) const;

    /**
     * @brief Invalidates the lazy-loaded metadata and artifact paths caches, forcing a re-parse on next access.
     *
     * @details
     * This is needed when the underlying files (e.g. headers.h) have been modified on disk since the cache was
     * populated. For example, after creating a new tileset struct, the cache must be invalidated so subsequent
     * metadata lookups see the newly written struct.
     */
    void invalidate_metadata_cache() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;

    // Lazy-loaded cache for parsed tileset metadata (mutable for const methods)
    mutable bool metadata_parsed_{false};
    mutable std::map<std::string, ProjectTilesetMetadata> tileset_metadata_;

    // Lazy-loaded cache for resolved tileset artifact paths (mutable for const methods)
    mutable bool artifact_paths_parsed_{false};
    mutable std::map<std::string, ProjectTilesetArtifactPaths> tileset_artifact_paths_;
};

} // namespace porytiles2
