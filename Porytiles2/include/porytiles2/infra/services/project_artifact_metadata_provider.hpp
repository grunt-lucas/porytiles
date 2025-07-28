#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/artifact_metadata_provider.hpp"
#include "porytiles2/infra/project/project_paths.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of the ArtifactMetadataProvider service that uses an in-filesystem
 * `pokeemerald` project as the source for artifact metadata.
 */
class ProjectArtifactMetadataProvider final : public ArtifactMetadataProvider {
  public:
    explicit ProjectArtifactMetadataProvider(const gsl::not_null<ProjectPaths *> paths,
                                             const gsl::not_null<TilesetRepo *> tileset_repo)
        : paths_{paths}, tileset_repo_{tileset_repo} {}

    [[nodiscard]] std::unordered_map<std::string, std::string>
    compute_porymap_checksums(const Tileset &tileset) const override;

    [[nodiscard]] std::unordered_map<std::string, std::string>
    load_stored_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] Result<void>
    store_checksums(const std::string &tileset_name,
                    const std::unordered_map<std::string, std::string> &checksums) const override;

    [[nodiscard]] std::unordered_map<std::string, Timestamp>
    get_porymap_timestamps(const Tileset &tileset) const override;

    [[nodiscard]] std::unordered_map<std::string, Timestamp>
    get_porytiles_timestamps(const Tileset &tileset) const override;

    [[nodiscard]] bool are_porytiles_assets_newer(const std::string &tileset_name) const override;

    [[nodiscard]] bool are_porymap_assets_newer(const std::string &tileset_name) const override;

  private:
    const ProjectPaths *paths_;
    TilesetRepo *tileset_repo_;
};

} // namespace porytiles2
