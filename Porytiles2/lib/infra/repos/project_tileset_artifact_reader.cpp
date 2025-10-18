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
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

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
        /*
         * TODO: CRITICAL
         * The Problem:
         * - LayerType only defines 3 valid values (0, 1, 2)
         * - The extraction attribute >> 12 & 0x000F produces 4-bit values (0-15)
         * - 13 out of 16 possible values (3-15) are invalid!
         *
         * What happens with invalid values:
         * 1. The static_cast succeeds silently, creating an invalid LayerType
         * 2. When to_string(LayerType) is called (line 20-32), invalid values hit the default: case at line 30, which
         * panics the program
         * 3. Any code that assumes only valid enumerators exist will have undefined behavior
         *
         * This is a serious bug - corrupted or malformed binary data will crash the program instead of returning a
         * proper error.
         *
         * Recommended fix: Add validation after the extraction to check if the value is in range [0-2], and return a
         * ChainableResult error if not, similar to how the function already handles other validation errors (like file
         * size checks).
         */
        MetatileAttribute metatile_attribute{
            static_cast<attr::LayerType>(attribute >> 12 & 0x000F), static_cast<std::uint16_t>(attribute & 0x00FF)};
        dest.porymap_component().push_back_attribute(metatile_attribute);
    }

    return {};
}

ChainableResult<void> import_firered_metatile_attributes(Tileset &dest, const ArtifactKey &src_key)
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
        // TODO: init an attr here and insert into 'dest'
    }

    return {};
}

ChainableResult<void> import_tiles_png(Tileset &dest, const ArtifactKey &src_key, const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return FormattableError{fmt::format("failed to load tiles.png: {}", image_result.error())};
    }
    dest.porymap_component().tiles_png(*image_result.value());
    return {};
}

ChainableResult<void> import_palette(Tileset &dest, const ArtifactKey &src_key, int index, const FilePalLoader &loader)
{
    // TODO: don't hardcode 16 here
    if (index < 0 || index >= 16) {
        panic(fmt::format("invalid pal index {}: out of range", index));
    }

    const auto pal_result = loader.load(src_key.key());
    if (!pal_result.has_value()) {
        return FormattableError{fmt::format("failed to load: {}", pal_result.error())};
    }
    dest.porymap_component().set_pal(pal_result.value(), index);

    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<void>
ProjectTilesetArtifactReader::read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const
{
    switch (artifact.type()) {
    // Porytiles artifacts
    case TilesetArtifact::Type::bottom_png: {
        const auto result = import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.bottom(img);
            });
        if (!result.has_value()) {
            return ChainableResult<void>{FormattableError{fmt::format("failed to read bottom.png")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::middle_png: {
        const auto result = import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.middle(img);
            });
        if (!result.has_value()) {
            return ChainableResult<void>{FormattableError{fmt::format("failed to read middle.png")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::top_png: {
        const auto result = import_layer_png(
            dest, src_key, *png_rgba_loader_, [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) {
                comp.top(img);
            });
        if (!result.has_value()) {
            return ChainableResult<void>{FormattableError{fmt::format("failed to read top.png")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::attributes_csv:
        panic("TODO: implement");
    case TilesetArtifact::Type::porytiles_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_override_n:
        panic("TODO: implement");

    // Porymap artifacts
    case TilesetArtifact::Type::metatiles_bin: {
        const auto result = import_metatiles_bin(dest, src_key);
        if (!result.has_value()) {
            return ChainableResult<void>{FormattableError{fmt::format("could not import metatiles.bin")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::metatile_attributes_bin: {
        // TODO: branch here based on target base game?
        const auto result = import_emerald_metatile_attributes(dest, src_key);
        if (!result.has_value()) {
            return ChainableResult<void>{
                FormattableError{fmt::format("could not import metatile_attributes.bin")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::tiles_png: {
        // TODO: make this a ChainableResult
        const auto result = import_tiles_png(dest, src_key, *png_indexed_loader_);
        if (!result.has_value()) {
            return ChainableResult<void>{FormattableError{fmt::format("could not import tiles.png")}, result};
        }
        return {};
    }
    case TilesetArtifact::Type::porymap_anim_frame:
        panic("TODO: implement");
    case TilesetArtifact::Type::pal_n: {
        if (!artifact.index().has_value()) {
            panic("took TilesetArtifact::Type::pal_n branch but missing pal index");
        }
        const auto result = import_palette(dest, src_key, artifact.index().value(), *pal_loader_);
        if (!result.has_value()) {
            return ChainableResult<void>{
                FormattableError{fmt::format("could not import pal {}", artifact.index().value())}, result};
        }
        return {};
    }

    // Default case
    default:
        panic("unhandled TilesetArtifact::Type");
    }
}

} // namespace porytiles2
