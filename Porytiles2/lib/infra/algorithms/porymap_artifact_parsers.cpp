#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"

#include <format>
#include <fstream>
#include <iterator>

namespace porytiles2 {

ChainableResult<std::vector<TilemapEntry>> parse_metatiles_bin(const std::filesystem::path &path)
{
    std::ifstream metatiles_bin{path, std::ios::binary};
    if (!metatiles_bin) {
        return FormattableError{"Failed to open metatiles.bin: {}", FormatParam{path.string(), Style::bold}};
    }

    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatiles_bin), {}};

    if (data_buf.size() % 2 != 0) {
        return FormattableError{"Size of metatiles.bin is not a multiple of 2 bytes, possibly corrupted."};
    }

    std::vector<TilemapEntry> entries;
    entries.reserve(data_buf.size() / 2);

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

        entries.push_back(std::move(entry));
    }

    return entries;
}

ChainableResult<std::vector<MetatileAttribute>> parse_emerald_metatile_attributes(const std::filesystem::path &path)
{
    std::ifstream metatile_attr_bin{path, std::ios::binary};
    if (!metatile_attr_bin) {
        return FormattableError{"Failed to open metatile_attributes.bin: {}", FormatParam{path.string(), Style::bold}};
    }

    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % attr::bytes_per_attr_emerald != 0) {
        return FormattableError{
            "Size of metatile_attributes.bin is not a multiple of {} bytes, possibly corrupted.",
            FormatParam{attr::bytes_per_attr_emerald, Style::bold}};
    }

    std::vector<MetatileAttribute> attributes;
    const std::size_t metatile_count = data_buf.size() / attr::bytes_per_attr_emerald;
    attributes.reserve(metatile_count);

    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        std::uint16_t byte0 = data_buf.at((metatile_index * attr::bytes_per_attr_emerald));
        std::uint16_t byte1 = data_buf.at((metatile_index * attr::bytes_per_attr_emerald) + 1);
        std::uint16_t attribute = (byte1 << 8) | byte0;

        auto layer_type_result = layer_type_from_int(attribute >> 12 & 0x000F);
        if (!layer_type_result.has_value()) {
            return ChainableResult<std::vector<MetatileAttribute>>{
                FormattableError{"Invalid layer type for metatile '{}'.", FormatParam{metatile_index, Style::bold}},
                layer_type_result};
        }
        MetatileAttribute metatile_attribute{layer_type_result.value(), static_cast<std::uint16_t>(attribute & 0x00FF)};
        attributes.push_back(std::move(metatile_attribute));
    }

    return attributes;
}

ChainableResult<std::vector<MetatileAttribute>> parse_firered_metatile_attributes(const std::filesystem::path &path)
{
    std::ifstream metatile_attr_bin{path, std::ios::binary};
    if (!metatile_attr_bin) {
        return FormattableError{"Failed to open metatile_attributes.bin: {}", FormatParam{path.string(), Style::bold}};
    }

    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attr_bin), {}};

    if (data_buf.size() % attr::bytes_per_attr_firered != 0) {
        return FormattableError{
            "Size of metatile_attributes.bin is not a multiple of {} bytes, possibly corrupted.",
            FormatParam{attr::bytes_per_attr_firered, Style::bold}};
    }

    std::vector<MetatileAttribute> attributes;
    const std::size_t metatile_count = data_buf.size() / attr::bytes_per_attr_firered;
    attributes.reserve(metatile_count);

    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        const std::size_t base = metatile_index * attr::bytes_per_attr_firered;
        const std::uint32_t byte0 = data_buf.at(base);
        const std::uint32_t byte1 = data_buf.at(base + 1);
        const std::uint32_t byte2 = data_buf.at(base + 2);
        const std::uint32_t byte3 = data_buf.at(base + 3);
        const std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;

        // FireRed attribute bit layout (from fieldmap.c):
        //   Bits  0-8:  behavior       (0x000001FF)
        //   Bits  9-13: terrain        (0x00003E00)
        //   Bits 14-17: attribute_2    (0x0003C000)
        //   Bits 18-23: attribute_3    (0x00FC0000)
        //   Bits 24-26: encounter_type (0x07000000)
        //   Bits 27-28: attribute_5    (0x18000000)
        //   Bits 29-30: layer_type     (0x60000000)
        //   Bit  31:    attribute_7    (0x80000000)
        const auto behavior = static_cast<std::uint16_t>(attribute & 0x000001FF);
        const auto terrain = static_cast<std::uint8_t>((attribute >> 9) & 0x1F);
        const auto attribute_2 = static_cast<std::uint8_t>((attribute >> 14) & 0x0F);
        const auto attribute_3 = static_cast<std::uint8_t>((attribute >> 18) & 0x3F);
        const auto encounter_type = static_cast<std::uint8_t>((attribute >> 24) & 0x07);
        const auto attribute_5 = static_cast<std::uint8_t>((attribute >> 27) & 0x03);
        const auto layer_type_raw = static_cast<unsigned int>((attribute >> 29) & 0x03);
        const bool attribute_7 = (attribute >> 31) & 0x01;

        auto layer_type_result = layer_type_from_int(layer_type_raw);
        if (!layer_type_result.has_value()) {
            return ChainableResult<std::vector<MetatileAttribute>>{
                FormattableError{"Invalid layer type for metatile '{}'.", FormatParam{metatile_index, Style::bold}},
                layer_type_result};
        }

        MetatileAttribute metatile_attribute{
            layer_type_result.value(),
            behavior,
            terrain,
            encounter_type,
            attribute_2,
            attribute_3,
            attribute_5,
            attribute_7};
        attributes.push_back(std::move(metatile_attribute));
    }

    return attributes;
}

ChainableResult<std::unique_ptr<Image<IndexPixel>>>
load_indexed_png(const std::filesystem::path &path, const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(path);
    if (!image_result.has_value()) {
        return ChainableResult<std::unique_ptr<Image<IndexPixel>>>{
            FormattableError{"Failed to load indexed PNG: {}", FormatParam{path.string(), Style::bold}}, image_result};
    }
    return std::move(image_result.value());
}

ChainableResult<Palette<Rgba32, pal::max_size>>
load_porymap_palette(const std::filesystem::path &path, const FilePalLoader &loader)
{
    auto pal_result = loader.load(path);
    if (!pal_result.has_value()) {
        return ChainableResult<Palette<Rgba32, pal::max_size>>{
            FormattableError{"Failed to load palette file: {}", FormatParam{path.string(), Style::bold}}, pal_result};
    }
    return pal_result.value();
}

} // namespace porytiles2
