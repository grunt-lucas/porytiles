#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <fstream>
#include <functional>
#include <iterator>
#include <ostream>

#include "porytiles2/domain/model/tilemap_entry.hpp"
#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/templates/panic.hpp"

namespace {

using namespace porytiles2;

/*
 * TODO: remove these hardcoded constants. fieldmap.c and global.fieldmap.h contain definitions for attribute shifts and
 * masks that could be used to infer these values
 */
constexpr std::size_t bytes_per_attr_emerald = 2;
constexpr std::size_t bytes_per_attr_firered = 4;

Result<void> import_layer_png(
    Tileset &dest,
    const ArtifactKey &src_key,
    const PngRgbaImageLoader &loader,
    const std::function<void(PorytilesTilesetComponent &, const Image<Rgba32> &)> &layer_img_setter)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        switch (image_result.error().type) {
        case ImageLoadError::Type::file_not_found:
            layer_img_setter(dest.porytiles_component(), Image<Rgba32>{});
            return {};
        case ImageLoadError::Type::unsupported_channel_count:
        case ImageLoadError::Type::other_load_error:
            // TODO: need a more descriptive ProjectTilesetArtifactReader::read result type
            return std::unexpected{fmt::format("failed to load bottom.png: {}", image_result.error().metadata)};
        default:
            panic("unhandled ImageLoadError type");
        }
    }
    layer_img_setter(dest.porytiles_component(), *image_result.value());
    return {};
}

Result<void> import_metatiles_bin(Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatiles_bin(src_key.key(), std::ios::binary);
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatiles_bin), {}};

    if (data_buf.size() % 2 != 0) {
        return std::unexpected{"metatiles.bin size is not a multiple of 2 bytes, probably corrupted"};
    }

    for (unsigned int byte_index = 0; byte_index < data_buf.size(); byte_index += 2) {
        TilemapEntry entry{};
        const std::uint16_t lower_byte = data_buf.at(byte_index);
        const std::uint16_t upper_byte = data_buf.at(byte_index + 1);
        const std::uint16_t entry_bits = (upper_byte << 8) | lower_byte;

        // -------- Metatile BIN Structure --------
        // The metatiles.bin file contains a sequence of tilemap entries, which are each two bytes with the following
        // structure:
        //
        // 0000 00XX XXXX XXXX
        // least significant 10 bits are the tile index
        //
        // 0000 0X00 0000 0000
        // 11th bit is the hflip bit
        //
        // 0000 X000 0000 0000
        // 12th bit is the vflip bit
        //
        // XXXX 0000 0000 0000
        // top 4 bits are pal index

        entry.tile_index(entry_bits & 0x03FF);
        entry.hflip((entry_bits >> 10) & 0x0001);
        entry.vflip((entry_bits >> 11) & 0x0001);
        entry.pal_index((entry_bits >> 12) & 0x000F);

        dest.porymap_component().push_back_tilemap_entry(entry);
    }

    return {};
}

Result<void> import_emerald_metatile_attributes(Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatile_attr_bin(src_key.key(), std::ios::binary);
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % bytes_per_attr_emerald != 0) {
        return std::unexpected{fmt::format(
            "metatile_attributes.bin size is not a multiple of {} bytes, probably corrupted", bytes_per_attr_emerald)};
    }

    std::size_t metatile_count = data_buf.size() / bytes_per_attr_emerald;
    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        std::uint16_t byte0 = data_buf.at((metatile_index * bytes_per_attr_emerald));
        std::uint16_t byte1 = data_buf.at((metatile_index * bytes_per_attr_emerald) + 1);
        std::uint16_t attribute = (byte1 << 8) | byte0;
        // attributes.metatileBehavior = attribute & 0x00FF;
        // attributes.layerType = layerTypeFromInt((attribute >> 12) & 0x000F);
        // TODO: init an attr here and insert into 'dest'
    }

    return {};
}

Result<void> import_firered_metatile_attributes(Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatile_attr_bin(src_key.key(), std::ios::binary);
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % bytes_per_attr_firered != 0) {
        return std::unexpected{fmt::format(
            "metatile_attributes.bin size is not a multiple of {} bytes, probably corrupted", bytes_per_attr_firered)};
    }

    std::size_t metatile_count = data_buf.size() / bytes_per_attr_emerald;
    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        std::uint32_t byte0 = data_buf.at((metatile_count * bytes_per_attr_firered));
        std::uint32_t byte1 = data_buf.at((metatile_count * bytes_per_attr_firered) + 1);
        std::uint32_t byte2 = data_buf.at((metatile_count * bytes_per_attr_firered) + 2);
        std::uint32_t byte3 = data_buf.at((metatile_count * bytes_per_attr_firered) + 3);
        std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
        // attributes.metatileBehavior = attribute & 0x000001FF;
        // attributes.terrainType = terrainTypeFromInt((attribute >> 9) & 0x0000001F);
        // attributes.encounterType = encounterTypeFromInt((attribute >> 24) & 0x00000007);
        // attributes.layerType = layerTypeFromInt((attribute >> 29) & 0x00000003);
        // TODO: init an attr here and insert into 'dest'
    }

    return {};
}

Result<void> import_tiles_png(Tileset &dest, const ArtifactKey &src_key, const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return std::unexpected{fmt::format("failed to load tiles.png: {}", image_result.error())};
    }
    dest.porymap_component().tiles_png(*image_result.value());
    return {};
}

Result<void> import_palette(Tileset &dest, const ArtifactKey &src_key, int index)
{
    // TODO: implement palette import
    return {};
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
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.bottom(img);
            });
    case TilesetArtifact::Type::middle_png:
        return import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.middle(img);
            });
    case TilesetArtifact::Type::top_png:
        return import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.top(img);
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
        // TODO: branch here based on target base game?
        return import_emerald_metatile_attributes(dest, src_key);
    case TilesetArtifact::Type::tiles_png:
        return import_tiles_png(dest, src_key, *png_indexed_loader_);
    case TilesetArtifact::Type::porymap_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_n:
        return import_palette(dest, src_key, artifact.index().value());

    // Default case
    default:
        panic("unhandled TilesetArtifact::Type");
    }
}

} // namespace porytiles2
