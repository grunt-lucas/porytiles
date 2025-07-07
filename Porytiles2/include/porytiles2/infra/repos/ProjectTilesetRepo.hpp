#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/domain/repos/TilesetRepo.hpp"
#include "porytiles2/domain/services/ArtifactMetadataProvider.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald`
 * project as the backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
public:
  explicit ProjectTilesetRepo(std::unique_ptr<ArtifactMetadataProvider> metadata_service)
      : TilesetRepo{std::move(metadata_service)} {}

  [[nodiscard]] Result<std::unique_ptr<Tileset>> Load(const std::string &name) const override;

  [[nodiscard]] bool Exists(const std::string &name) const override;

protected:
  [[nodiscard]] Result<void> SaveTileset(const Tileset &tileset) override;
};

} // namespace porytiles
