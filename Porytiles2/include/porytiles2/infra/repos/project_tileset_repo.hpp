#pragma once

#include <memory>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"

namespace porytiles2 {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald` project as the
 * backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
  public:
    explicit ProjectTilesetRepo(
        gsl::not_null<ArtifactChecksumProvider *> checksum_provider,
        gsl::not_null<TilesetArtifactKeyProvider *> key_provider,
        gsl::not_null<TilesetArtifactReader *> reader,
        gsl::not_null<TilesetArtifactWriter *> writer)
        : TilesetRepo{checksum_provider, key_provider, reader, writer} {}

    [[nodiscard]] bool exists(const std::string &name) const override;
};

} // namespace porytiles2
