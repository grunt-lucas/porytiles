#pragma once

#include "gsl/pointers"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/domain/repos/TilesetRepo.hpp"
#include "porytiles2/domain/services/ArtifactMetadataProvider.hpp"
#include "porytiles2/infra/project/ProjectPaths.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald` project as the
 * backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
public:
  explicit ProjectTilesetRepo(std::unique_ptr<ArtifactMetadataProvider> metadata_service,
                              const gsl::not_null<ProjectPaths *> paths)
      : TilesetRepo{std::move(metadata_service)}, paths_{paths} {}

  [[nodiscard]] Result<std::unique_ptr<Tileset>> load(const std::string &name) const override;

  [[nodiscard]] bool exists(const std::string &name) const override;

protected:
  [[nodiscard]] Result<void> save_tileset(const Tileset &tileset) override;

private:
  const ProjectPaths *paths_;
};

} // namespace porytiles
