#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

Result<void>
ProjectTilesetArtifactReader::read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const {
    switch (artifact.type()) {
    // Porytiles artifacts
    case TilesetArtifact::Type::bottom_png:
        // TODO: implement
        break;
    case TilesetArtifact::Type::middle_png:
        // TODO: implement
        break;
    case TilesetArtifact::Type::top_png:
        // TODO: implement
        break;
    case TilesetArtifact::Type::attributes_csv:
        // TODO: implement
        break;
    case TilesetArtifact::Type::porytiles_anim_frame:
        // TODO: implement
        break;
    case TilesetArtifact::Type::pal_override_n:
        // TODO: implement
        break;

    // Porymap artifacts
    case TilesetArtifact::Type::metatiles_bin:
        // TODO: implement
        break;
    case TilesetArtifact::Type::metatile_attributes_bin:
        // TODO: implement
        break;
    case TilesetArtifact::Type::tiles_png:
        // TODO: implement
        break;
    case TilesetArtifact::Type::porymap_anim_frame:
        // TODO: implement
        break;
    case TilesetArtifact::Type::pal_n:
        // TODO: implement
        break;

    // Default case
    default:
        panic("unhandled TilesetArtifact::Type");
    }
    panic("TODO: implement");
}

} // namespace porytiles2
