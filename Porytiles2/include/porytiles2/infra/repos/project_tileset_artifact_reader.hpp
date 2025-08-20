#pragma once

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"

namespace porytiles2 {

/**
 * @brief Provides a filesystem-based implementation for TilesetArtifactReader
 *
 * @details
 * This class implements the TilesetArtifactReader interface to provide reading functionality for tileset artifacts. It
 * operates within the context of a Pokémon Gen III decompilation project on the local filesystem.
 */
class ProjectTilesetArtifactReader final : public TilesetArtifactReader {
  public:
    ProjectTilesetArtifactReader() = default;

    [[nodiscard]] Result<void>
    read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const override;
};

} // namespace porytiles2
