#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <fstream>
#include <functional>
#include <iterator>

#include "fmt/format.h"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace {

using namespace porytiles2;

ChainableResult<void> import_layer_png(
    Tileset &dest,
    const ArtifactKey &src_key,
    const PngRgbaImageLoader &loader,
    const std::function<void(PorytilesTilesetComponent &, const Image<Rgba32> &)> &layer_img_setter)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        switch (image_result.error().type()) {
            // TODO: this shouldn't load a blank image, it should just error. To support the "import" case, we're going
            // to create a special tileset operation called "import" which is distinct from "load", and which assumes a
            // Porytiles component is not present.
        case ImageLoadError::Type::file_not_found:
            layer_img_setter(dest.porytiles_component(), Image<Rgba32>{});
            return {};
        case ImageLoadError::Type::unsupported_channel_count:
        case ImageLoadError::Type::other_load_error: {
            const auto error_msg = fmt::format("failed to load layer image: {}", src_key.key());
            return ChainableResult<void>{FormattableError{error_msg}, image_result};
        }
        default:
            panic("unhandled ImageLoadError type");
        }
    }
    layer_img_setter(dest.porytiles_component(), *image_result.value());
    return {};
}

ChainableResult<void> import_metatiles_bin(Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatiles_bin{src_key.key(), std::ios::binary};
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatiles_bin), {}};

    if (data_buf.size() % 2 != 0) {
        return FormattableError{"metatiles.bin size is not a multiple of 2 bytes, probably corrupted"};
    }

    for (std::size_t byte_index = 0; byte_index < data_buf.size(); byte_index += 2) {
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
        entry.h_flip((entry_bits >> 10) & 0x0001);
        entry.v_flip((entry_bits >> 11) & 0x0001);
        entry.pal_index((entry_bits >> 12) & 0x000F);

        dest.porymap_component().push_back_tilemap_entry(entry);
    }

    return {};
}

ChainableResult<void> import_emerald_metatile_attributes(Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatile_attr_bin{src_key.key(), std::ios::binary};
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % attr::bytes_per_attr_emerald != 0) {
        return FormattableError{fmt::format(
            "metatile_attributes.bin size is not a multiple of {} bytes, probably corrupted",
            attr::bytes_per_attr_emerald)};
    }

    std::size_t metatile_count = data_buf.size() / attr::bytes_per_attr_emerald;
    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        std::uint16_t byte0 = data_buf.at((metatile_index * attr::bytes_per_attr_emerald));
        std::uint16_t byte1 = data_buf.at((metatile_index * attr::bytes_per_attr_emerald) + 1);
        std::uint16_t attribute = (byte1 << 8) | byte0;

        auto layer_type_result = layer_type_from_int(attribute >> 12 & 0x000F);
        if (!layer_type_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{"invalid layer type for metatile '{}'", FormatParam{metatile_index, Style::bold}},
                layer_type_result};
        }
        MetatileAttribute metatile_attribute{layer_type_result.value(), static_cast<std::uint16_t>(attribute & 0x00FF)};
        dest.porymap_component().push_back_attribute(metatile_attribute);
    }

    return {};
}

[[maybe_unused]] ChainableResult<void>
import_firered_metatile_attributes([[maybe_unused]] Tileset &dest, const ArtifactKey &src_key)
{
    std::ifstream metatile_attr_bin{src_key.key(), std::ios::binary};
    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % attr::bytes_per_attr_firered != 0) {
        return FormattableError{fmt::format(
            "metatile_attributes.bin size is not a multiple of {} bytes, probably corrupted",
            attr::bytes_per_attr_firered)};
    }

    std::size_t metatile_count = data_buf.size() / attr::bytes_per_attr_emerald;
    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        std::uint32_t byte0 = data_buf.at((metatile_count * attr::bytes_per_attr_firered));
        std::uint32_t byte1 = data_buf.at((metatile_count * attr::bytes_per_attr_firered) + 1);
        std::uint32_t byte2 = data_buf.at((metatile_count * attr::bytes_per_attr_firered) + 2);
        std::uint32_t byte3 = data_buf.at((metatile_count * attr::bytes_per_attr_firered) + 3);
        std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
        // attributes.metatileBehavior = attribute & 0x000001FF;
        // attributes.terrainType = terrainTypeFromInt((attribute >> 9) & 0x0000001F);
        // attributes.encounterType = encounterTypeFromInt((attribute >> 24) & 0x00000007);
        // attributes.layerType = layerTypeFromInt((attribute >> 29) & 0x00000003);
        // TODO: finish impl: init an attr here and insert into 'dest'
    }

    return {};
}

ChainableResult<void> import_tiles_png(Tileset &dest, const ArtifactKey &src_key, const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return ChainableResult<void>{FormattableError{"failed to load tiles.png"}, image_result};
    }
    dest.porymap_component().tiles_png(*image_result.value());
    return {};
}

ChainableResult<void>
import_porymap_palette(Tileset &dest, const ArtifactKey &src_key, std::size_t index, const FilePalLoader &loader)
{
    if (index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", index));
    }

    const auto pal_result = loader.load(src_key.key());
    if (!pal_result.has_value()) {
        return ChainableResult<void>{FormattableError{"failed to load palette"}, pal_result};
    }
    dest.porymap_component().set_pal(index, pal_result.value());

    return {};
}

ChainableResult<void>
import_porytiles_palette(Tileset &dest, const ArtifactKey &src_key, std::size_t index, const FilePalLoader &loader)
{
    if (index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", index));
    }

    const auto pal_result = loader.load_with_wildcards(src_key.key());
    if (!pal_result.has_value()) {
        return ChainableResult<void>{FormattableError{"failed to load palette"}, pal_result};
    }
    dest.porytiles_component().set_pal(index, pal_result.value());

    return {};
}

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<void> ProjectTilesetArtifactReader::read_metatiles_bin(Tileset &dest, const ArtifactKey &src_key) const
{
    const auto result = import_metatiles_bin(dest, src_key);
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("could not import metatiles.bin")}, result};
    }
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const
{
    // TODO: branch here based on target base game?
    const auto result = import_emerald_metatile_attributes(dest, src_key);
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("could not import metatile_attributes.bin")}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const
{
    const auto result = import_tiles_png(dest, src_key, *png_indexed_loader_);
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("could not import tiles.png")}, result};
    }
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    const auto result = import_porymap_palette(dest, src_key, index, *pal_loader_);
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("could not import pal {}", index)}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_porymap_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const
{
    panic("TODO: implement read_porymap_anim_frame");
}

/*
 * Porytiles artifacts
 */
ChainableResult<void> ProjectTilesetArtifactReader::read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const
{
    const auto result = import_layer_png(
        dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
            comp.bottom(img);
        });
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("failed to read bottom.png")}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_middle_png(Tileset &dest, const ArtifactKey &src_key) const
{
    const auto result = import_layer_png(
        dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
            comp.middle(img);
        });
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("failed to read middle.png")}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_top_png(Tileset &dest, const ArtifactKey &src_key) const
{
    const auto result = import_layer_png(
        dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
            comp.top(img);
        });
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("failed to read top.png")}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const
{
    panic("TODO: implement read_attributes_csv");
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    const auto result = import_porytiles_palette(dest, src_key, index, *pal_loader_);
    if (!result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("could not import pal {}", index)}, result};
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_porytiles_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const
{
    panic("TODO: implement read_porytiles_anim_frame");
}

} // namespace porytiles2
