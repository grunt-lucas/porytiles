#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <fstream>
#include <functional>
#include <iterator>
#include <ostream>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/templates/panic.hpp"

namespace {

porytiles2::Result<void> import_layer_png(
    const porytiles2::Tileset &dest,
    const porytiles2::ArtifactKey &src_key,
    const porytiles2::PngRgbaImageLoader &loader,
    const std::function<
        void(porytiles2::PorytilesTilesetComponent *, std::unique_ptr<porytiles2::Image<porytiles2::Rgba32>>)> &setter)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        switch (image_result.error().type) {
        case porytiles2::ImageLoadError::Type::file_not_found:
            setter(dest.porytiles_component(), std::make_unique<porytiles2::Image<porytiles2::Rgba32>>());
            return {};
        case porytiles2::ImageLoadError::Type::unsupported_channel_count:
        case porytiles2::ImageLoadError::Type::other_load_error:
            // TODO: need a more descriptive ProjectTilesetArtifactReader::read result type
            return std::unexpected{fmt::format("failed to load bottom.png: {}", image_result.error().metadata)};
        default:
            porytiles2::panic("unhandled error type");
        }
    }
    setter(dest.porytiles_component(), std::move(image_result).value());
    return {};
}

porytiles2::Result<void> import_metatiles_bin(porytiles2::Tileset &dest, const porytiles2::ArtifactKey &src_key)
{
    std::ifstream metatiles_bin(src_key.key(), std::ios::binary);
    std::vector<unsigned char> metatileDataBuf{std::istreambuf_iterator(metatiles_bin), {}};
    porytiles2::panic("TODO: implement");
}

} // namespace

namespace porytiles2 {

Result<void>
ProjectTilesetArtifactReader::read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const
{
    switch (artifact.type()) {
    // Porytiles artifacts
    case TilesetArtifact::Type::bottom_png:
        return import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent *comp, std::unique_ptr<Image<Rgba32>> img) {
                comp->bottom(std::move(img));
            });
    case TilesetArtifact::Type::middle_png:
        return import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent *comp, std::unique_ptr<Image<Rgba32>> img) {
                comp->middle(std::move(img));
            });
    case TilesetArtifact::Type::top_png:
        return import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent *comp, std::unique_ptr<Image<Rgba32>> img) {
                comp->top(std::move(img));
            });
    case TilesetArtifact::Type::attributes_csv:
        panic("TODO: implement");
    case TilesetArtifact::Type::porytiles_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_override_n:
        panic("TODO: implement");

    // Porymap artifacts
    case TilesetArtifact::Type::metatiles_bin:
        return import_metatiles_bin(dest, src_key);
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
