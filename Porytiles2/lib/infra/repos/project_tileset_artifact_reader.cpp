#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>

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
    case TilesetArtifact::Type::bottom_png: {
        auto image_result = png_rgba_loader_->load_from_file(src_key.key());
        if (!image_result.has_value()) {
            return std::unexpected{fmt::format("failed to load bottom.png: {}", image_result.error())};
        }
        dest.porytiles_component()->bottom(std::move(image_result).value());
        break;
    }
    case TilesetArtifact::Type::middle_png: {
        auto image_result = png_rgba_loader_->load_from_file(src_key.key());
        if (!image_result.has_value()) {
            return std::unexpected{fmt::format("failed to load middle.png: {}", image_result.error())};
        }
        dest.porytiles_component()->middle(std::move(image_result).value());
        break;
    }
    case TilesetArtifact::Type::top_png: {
        auto image_result = png_rgba_loader_->load_from_file(src_key.key());
        if (!image_result.has_value()) {
            return std::unexpected{fmt::format("failed to load top.png: {}", image_result.error())};
        }
        dest.porytiles_component()->top(std::move(image_result).value());
        break;
    }
    case TilesetArtifact::Type::attributes_csv:
        panic("TODO: implement");
    case TilesetArtifact::Type::porytiles_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_override_n:
        panic("TODO: implement");

    // Porymap artifacts
    case TilesetArtifact::Type::metatiles_bin:
        panic("TODO: implement");
    case TilesetArtifact::Type::metatile_attributes_bin:
        panic("TODO: implement");
    case TilesetArtifact::Type::tiles_png:
        panic("TODO: implement");
    case TilesetArtifact::Type::porymap_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_n:
        panic("TODO: implement");

    // Default case
    default:
        panic("unhandled TilesetArtifact::Type");
    }
    panic("TODO: implement");
}

} // namespace porytiles2
