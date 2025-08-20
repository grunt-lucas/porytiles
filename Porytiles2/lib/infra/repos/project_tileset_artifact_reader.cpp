#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

Result<void>
ProjectTilesetArtifactReader::read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const {
    panic("TODO: implement");
}

} // namespace porytiles2
