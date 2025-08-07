#pragma once

#include "gsl/pointers"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/artifact_metadata_provider.hpp"
#include "porytiles2/infra/project/project_paths.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald` project as the
 * backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
  public:
    explicit ProjectTilesetRepo(std::unique_ptr<ArtifactMetadataProvider> metadata_provider,
                                std::unique_ptr<TilesetArtifactKeyProvider> key_provider,
                                std::unique_ptr<TilesetArtifactReader> reader,
                                std::unique_ptr<TilesetArtifactWriter> writer,
                                const gsl::not_null<ProjectPaths *> paths)
        : TilesetRepo{std::move(metadata_provider), std::move(key_provider), std::move(reader), std::move(writer)},
          paths_{paths} {}

    [[nodiscard]] bool exists(const std::string &name) const override;

  private:
    const ProjectPaths *paths_;
};

} // namespace porytiles2
