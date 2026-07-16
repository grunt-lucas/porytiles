#include "porytiles/infra/algorithms/porymap_artifact_parsers.hpp"

#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>

namespace {

using namespace porytiles;

/// @brief Packs one attribute's schema fields, including the layer type, into a single word.
///
/// @details
/// Every mask and offset comes from the schema; this function carries no layout literals. A value field
/// absent from the attribute packs its schema default, the same effective-value rule the attributes CSV
/// writer applies, so the binary and CSV renderings of one attribute always agree. Field values are masked
/// after shifting, so a stored value wider than its field cannot bleed into neighboring bits. The
/// layer_type-role field packs the attribute's layer type, whose value Porytiles manages rather than the
/// fields map; a schema with no role field packs no layer bits at all.
[[nodiscard]] std::uint32_t pack_metatile_attribute(const MetatileAttribute &attribute, const Schema &schema)
{
    std::uint32_t raw = 0;
    for (const Field &field : schema.fields()) {
        const std::uint32_t value = field.packs_layer_type() ? static_cast<std::uint32_t>(attribute.layer_type())
                                    : attribute.fields().contains(field.name()) ? attribute.field(field.name())
                                                                                : field.default_value();
        raw |= (value << field.offset()) & field.mask();
    }
    return raw;
}

/// @brief Unpacks a single attribute word into schema field values and the layer type.
///
/// @details
/// The inverse of pack_metatile_attribute. Every value field is set explicitly (a zero bit pattern stores
/// an explicit 0), matching what the binary genuinely encodes. The layer_type-role field decodes into the
/// attribute's layer type, never into the fields map, and its bits must decode to a known LayerType; an
/// out-of-range value is a parse error. A schema with no role field reads no layer bits, so the attribute
/// keeps the default LayerType::normal.
[[nodiscard]] ChainableResult<MetatileAttribute> unpack_metatile_attribute(std::uint32_t raw, const Schema &schema)
{
    MetatileAttribute attribute{};
    for (const Field &field : schema.value_fields()) {
        attribute.field(field.name(), (raw & field.mask()) >> field.offset());
    }

    if (const Field *layer_field = schema.layer_type_field(); layer_field != nullptr) {
        PT_TRY_ASSIGN_PASS_ERR(
            layer_type, layer_type_from_int((raw & layer_field->mask()) >> layer_field->offset()), MetatileAttribute);
        attribute.layer_type(layer_type);
    }

    return attribute;
}

} // namespace

namespace porytiles {

ChainableResult<std::vector<TilemapEntry>> parse_metatiles_bin(const std::filesystem::path &path)
{
    std::ifstream metatiles_bin{path, std::ios::binary};
    if (!metatiles_bin) {
        return FormattableError{"Failed to open metatiles.bin: '{}'.", FormatParam{path.string(), Style::bold}};
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

        // Metatile BIN Structure
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
        // top 4 bits are palette index

        entry.tile_index(entry_bits & 0x03FF);
        entry.h_flip((entry_bits >> 10) & 0x0001);
        entry.v_flip((entry_bits >> 11) & 0x0001);
        entry.palette_index((entry_bits >> 12) & 0x000F);

        entries.push_back(std::move(entry));
    }

    return entries;
}

ChainableResult<std::vector<MetatileAttribute>>
parse_metatile_attributes(const std::filesystem::path &path, const Schema &schema)
{
    std::ifstream metatile_attribute_bin{path, std::ios::binary};
    if (!metatile_attribute_bin) {
        return FormattableError{
            "Failed to open metatile_attributes.bin: '{}'.", FormatParam{path.string(), Style::bold}};
    }

    const std::vector<unsigned char> data_buf{std::istreambuf_iterator(metatile_attribute_bin), {}};

    const std::size_t attribute_bytes = schema.attribute_bytes();
    if (data_buf.size() % attribute_bytes != 0) {
        return FormattableError{
            "Size of metatile_attributes.bin is not a multiple of {} bytes, possibly corrupted.",
            FormatParam{attribute_bytes, Style::bold}};
    }

    std::vector<MetatileAttribute> attributes;
    const std::size_t metatile_count = data_buf.size() / attribute_bytes;
    attributes.reserve(metatile_count);

    for (std::size_t metatile_index = 0; metatile_index < metatile_count; metatile_index++) {
        const std::size_t base = metatile_index * attribute_bytes;
        std::uint32_t raw = 0;
        for (std::size_t byte_index = 0; byte_index < attribute_bytes; byte_index++) {
            raw |= static_cast<std::uint32_t>(data_buf.at(base + byte_index)) << (byte_index * 8);
        }

        auto attribute_result = unpack_metatile_attribute(raw, schema);
        if (!attribute_result.has_value()) {
            return ChainableResult<std::vector<MetatileAttribute>>{
                FormattableError{"Invalid layer type for metatile '{}'.", FormatParam{metatile_index, Style::bold}},
                attribute_result};
        }
        attributes.push_back(std::move(attribute_result).value());
    }

    return attributes;
}

ChainableResult<void> save_metatile_attributes_bin(
    const std::vector<MetatileAttribute> &attributes, const std::filesystem::path &path, const Schema &schema)
{
    std::ofstream out{path, std::ios::binary};
    if (!out) {
        return FormattableError{
            "Failed to open metatile_attributes.bin for writing: '{}'.", FormatParam{path.string(), Style::bold}};
    }

    const std::size_t attribute_bytes = schema.attribute_bytes();
    for (const auto &attribute : attributes) {
        const std::uint32_t raw = pack_metatile_attribute(attribute, schema);
        for (std::size_t byte_index = 0; byte_index < attribute_bytes; byte_index++) {
            out << static_cast<std::uint8_t>(raw >> (byte_index * 8));
        }
    }
    out.flush();
    return {};
}

ChainableResult<std::unique_ptr<Image<IndexPixel>>>
load_indexed_png(const std::filesystem::path &path, const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(path);
    if (!image_result.has_value()) {
        return ChainableResult<std::unique_ptr<Image<IndexPixel>>>{
            FormattableError{"Failed to load indexed PNG: '{}'.", FormatParam{path.string(), Style::bold}},
            image_result};
    }
    return std::move(image_result.value());
}

ChainableResult<Palette<Rgba32, palette::max_size>>
load_porymap_palette(const std::filesystem::path &path, const FilePaletteLoader &loader)
{
    auto palette_result = loader.load(path);
    if (!palette_result.has_value()) {
        return ChainableResult<Palette<Rgba32, palette::max_size>>{
            FormattableError{"Failed to load palette file: '{}'.", FormatParam{path.string(), Style::bold}},
            palette_result};
    }
    return palette_result.value();
}

} // namespace porytiles
