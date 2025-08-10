#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/artifact_metadata_provider.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"

namespace porytiles2 {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald` project as the
 * backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
  public:
    explicit ProjectTilesetRepo(
        std::unique_ptr<ArtifactMetadataProvider> metadata_provider,
        std::unique_ptr<TilesetArtifactKeyProvider> key_provider,
        std::unique_ptr<TilesetArtifactReader> reader,
        std::unique_ptr<TilesetArtifactWriter> writer)
        : TilesetRepo{std::move(metadata_provider), std::move(key_provider), std::move(reader), std::move(writer)} {}

    [[nodiscard]] bool exists(const std::string &name) const override;
};

} // namespace porytiles2
