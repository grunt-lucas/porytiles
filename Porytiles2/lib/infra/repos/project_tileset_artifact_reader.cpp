#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <unordered_set>

#include "fmt/format.h"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/string_utils.hpp"

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
        return ChainableResult<void>{FormattableError{"failed to load palette file"}, pal_result};
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
        return ChainableResult<void>{FormattableError{"failed to load palette file"}, pal_result};
    }
    dest.porytiles_component().set_pal(index, pal_result.value());

    return {};
}

struct AttributesCsvRow {
    std::size_t metatile_id;
    std::string behavior;
};

ChainableResult<AttributesCsvRow>
parse_attributes_csv_row(const std::string &line, std::size_t line_number, const std::filesystem::path &file_path)
{
    auto columns = split(line, ",");

    if (columns.size() < 2) {
        return FormattableError{
            "{}:{}: expected at least 2 columns (id,behavior), found {}",
            FormatParam{file_path.string()},
            FormatParam{line_number},
            FormatParam{columns.size()}};
    }

    trim(columns[0]);
    trim(columns[1]);

    auto id_result = parse_int<int>(columns[0], 0);
    if (!id_result.has_value()) {
        return FormattableError{
            "{}:{}: invalid metatile id '{}': {}",
            FormatParam{file_path.string()},
            FormatParam{line_number},
            FormatParam{columns[0]},
            FormatParam{id_result.error()}};
    }

    if (id_result.value() < 0) {
        return FormattableError{
            "{}:{}: metatile id '{}' cannot be negative",
            FormatParam{file_path.string()},
            FormatParam{line_number},
            FormatParam{columns[0]}};
    }

    return AttributesCsvRow{static_cast<std::size_t>(id_result.value()), columns[1]};
}

ChainableResult<void>
import_attributes_csv(Tileset &dest, const std::filesystem::path &csv_path, const BehaviorMapProvider &behavior_map)
{
    if (!std::filesystem::exists(csv_path)) {
        return FormattableError{"attributes CSV file not found: {}", FormatParam{csv_path.string()}};
    }

    std::ifstream stream{csv_path};
    if (!stream.is_open()) {
        return FormattableError{"failed to open attributes CSV: {}", FormatParam{csv_path.string()}};
    }

    std::string line;
    std::size_t line_number = 0;
    std::unordered_set<std::size_t> seen_ids;

    if (!std::getline(stream, line)) {
        return FormattableError{"{}:1: file is empty, expected header 'id,behavior'", FormatParam{csv_path.string()}};
    }
    line_number = 1;
    trim_line_ending(line);

    auto header_columns = split(line, ",");
    if (header_columns.size() < 2) {
        return FormattableError{
            "{}:1: invalid header, expected 'id,behavior' but found '{}'",
            FormatParam{csv_path.string()},
            FormatParam{line}};
    }
    trim(header_columns[0]);
    trim(header_columns[1]);
    if (header_columns[0] != "id" || header_columns[1] != "behavior") {
        return FormattableError{
            "{}:1: invalid header, expected 'id,behavior' but found '{},{}'",
            FormatParam{csv_path.string()},
            FormatParam{header_columns[0]},
            FormatParam{header_columns[1]}};
    }

    while (std::getline(stream, line)) {
        line_number++;
        trim_line_ending(line);

        if (line.empty()) {
            continue;
        }

        auto row_result = parse_attributes_csv_row(line, line_number, csv_path);
        if (!row_result.has_value()) {
            return ChainableResult<void>{FormattableError{"failed to parse row"}, row_result};
        }

        const auto &row = row_result.value();

        if (seen_ids.contains(row.metatile_id)) {
            return FormattableError{
                "{}:{}: duplicate metatile id '{}' (previously defined)",
                FormatParam{csv_path.string()},
                FormatParam{line_number},
                FormatParam{row.metatile_id}};
        }
        seen_ids.insert(row.metatile_id);

        auto behavior_value = behavior_map.lookup(row.behavior);

        // TODO: use FileHighlightPrinter to display CSV lines that failed

        if (!behavior_value.has_value()) {
            return ChainableResult<void>{
                FormattableError{
                    "{}:{}: unknown metatile behavior '{}'",
                    FormatParam{csv_path.string()},
                    FormatParam{line_number},
                    FormatParam{row.behavior, Style::bold}},
                behavior_value};
        }

        MetatileAttribute attribute{LayerType::normal, behavior_value.value()};
        dest.porytiles_component().insert_attribute(row.metatile_id, attribute);
    }

    return {};
}

ChainableResult<void> import_porymap_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index)
{
    // TODO: implement
    return {};
}

ChainableResult<void> import_porytiles_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index)
{
    // TODO: implement
    return {};
}

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<void> ProjectTilesetArtifactReader::read_metatiles_bin(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(import_metatiles_bin(dest, src_key), void);
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const
{
    // TODO: branch here based on target base game?
    PT_TRY_CALL_PASS_ERR(import_emerald_metatile_attributes(dest, src_key), void)
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(import_tiles_png(dest, src_key, *png_indexed_loader_), void)
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    PT_TRY_CALL_PASS_ERR(import_porymap_palette(dest, src_key, index, *pal_loader_), void)
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_porymap_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_CALL_PASS_ERR(import_porymap_anim_frame(dest, src_key, anim_name, frame_index), void);
    return {};
}

/*
 * Porytiles artifacts
 */
ChainableResult<void> ProjectTilesetArtifactReader::read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(
        import_layer_png(
            dest,
            src_key,
            *png_rgba_loader_,
            [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) { comp.bottom(img); }),
        void);
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_middle_png(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(
        import_layer_png(
            dest,
            src_key,
            *png_rgba_loader_,
            [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) { comp.middle(img); }),
        void);
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_top_png(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(
        import_layer_png(
            dest,
            src_key,
            *png_rgba_loader_,
            [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) { comp.top(img); }),
        void);
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const
{
    PT_TRY_CALL_PASS_ERR(import_attributes_csv(dest, src_key.key(), *behavior_map_provider_), void);
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    PT_TRY_CALL_PASS_ERR(import_porytiles_palette(dest, src_key, index, *pal_loader_), void);
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_porytiles_anim_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_CALL_PASS_ERR(import_porytiles_anim_frame(dest, src_key, anim_name, frame_index), void);
    return {};
}

} // namespace porytiles2
