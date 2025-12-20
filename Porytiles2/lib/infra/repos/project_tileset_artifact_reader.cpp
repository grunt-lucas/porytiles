#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <filesystem>
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

/**
 * @brief Helper template function to extract 8x8 tiles from an image in row-major order.
 *
 * @tparam PixelType The pixel type (Rgba32 or IndexPixel)
 * @param img The source image
 * @return Vector of extracted tiles
 * @pre Image dimensions must be multiples of 8
 */
template <typename PixelType>
std::vector<PixelTile<PixelType>> extract_tiles_from_image(const Image<PixelType> &img)
{
    if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
        panic(
            fmt::format(
                "Animation frame dimensions must be multiples of {}, got {}x{}",
                tile::side_length_pix,
                img.width(),
                img.height()));
    }

    const std::size_t tiles_per_row = img.width() / tile::side_length_pix;
    const std::size_t tiles_per_col = img.height() / tile::side_length_pix;

    std::vector<PixelTile<PixelType>> tiles;
    tiles.reserve(tiles_per_row * tiles_per_col);

    for (std::size_t tile_row = 0; tile_row < tiles_per_col; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < tiles_per_row; ++tile_col) {
            PixelTile<PixelType> pixel_tile;

            const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
            const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

            for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
                for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                    const std::size_t src_row = pixel_row_offset + pixel_row;
                    const std::size_t src_col = pixel_col_offset + pixel_col;
                    pixel_tile.set(pixel_row, pixel_col, img.at(src_row, src_col));
                }
            }

            tiles.push_back(std::move(pixel_tile));
        }
    }

    return tiles;
}

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

/*
 * TODO: the import_x_anim_{key}_frame functions are almost identical. All three cases can be moved to a template method
 * using duck typing, and then we can simply call the correct template from the main loading implementation function.
 */
ChainableResult<void> import_porytiles_anim_key_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, const PngRgbaImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "{}: failed to load Porytiles animation key frame", FormatParam{src_key.key(), Style::bold}},
            image_result};
    }

    auto tiles = extract_tiles_from_image(*image_result.value());
    constexpr auto frame_name = "key.png";

    AnimationFrame frame{frame_name, std::move(tiles)};

    // Get or create the animation in the Porymap component
    auto &porytiles_comp = dest.porytiles_component();
    if (!porytiles_comp.has_anim(anim_name)) {
        Animation<Rgba32> anim{anim_name};
        porytiles_comp.add_anim(std::move(anim));
    }

    // Add the frame to the animation
    // Note: frames may be loaded out of order, so we use a simple approach:
    // ensure the frames vector is large enough and set the frame at the correct index
    auto &anim = porytiles_comp.anims().at(anim_name);
    anim.key_frame(frame);

    return {};
}

ChainableResult<void> import_porymap_anim_frame(
    Tileset &dest,
    const ArtifactKey &src_key,
    const std::string &anim_name,
    std::size_t frame_index,
    const PngIndexedImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to load Porymap animation frame", FormatParam{src_key.key(), Style::bold}},
            image_result};
    }

    auto tiles = extract_tiles_from_image(*image_result.value());
    const std::string frame_name = std::to_string(frame_index);

    AnimationFrame frame{frame_name, std::move(tiles)};

    // Get or create the animation in the Porymap component
    auto &porymap_comp = dest.porymap_component();
    if (!porymap_comp.has_anim(anim_name)) {
        Animation<IndexPixel> anim{anim_name};
        porymap_comp.add_anim(std::move(anim));
    }

    // Add the frame to the animation
    // Note: frames may be loaded out of order, so we use a simple approach:
    // ensure the frames vector is large enough and set the frame at the correct index
    auto &anim = porymap_comp.anims().at(anim_name);
    while (anim.frame_count() <= frame_index) {
        anim.add_frame(AnimationFrame<IndexPixel>{});
    }
    anim.frames()[frame_index] = std::move(frame);

    return {};
}

ChainableResult<void> import_porytiles_anim_frame(
    Tileset &dest,
    const ArtifactKey &src_key,
    const std::string &anim_name,
    std::size_t frame_index,
    const PngRgbaImageLoader &loader)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to load Porytiles animation frame: {}", FormatParam{src_key.key(), Style::bold}},
            image_result};
    }

    auto tiles = extract_tiles_from_image(*image_result.value());
    const std::string frame_name = std::to_string(frame_index);

    AnimationFrame frame{frame_name, std::move(tiles)};

    // Get or create the animation in the Porytiles component
    auto &porytiles_comp = dest.porytiles_component();
    if (!porytiles_comp.has_anim(anim_name)) {
        Animation<Rgba32> anim{anim_name};
        porytiles_comp.add_anim(std::move(anim));
    }

    // Add the frame to the animation
    // Note: frames may be loaded out of order, so we ensure the vector is large enough
    auto &anim = porytiles_comp.anims().at(anim_name);
    while (anim.frame_count() <= frame_index) {
        anim.add_frame(AnimationFrame<Rgba32>{});
    }
    anim.frames()[frame_index] = std::move(frame);

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
    PT_TRY_CALL_PASS_ERR(import_porymap_anim_frame(dest, src_key, anim_name, frame_index, *png_indexed_loader_), void);
    return {};
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactReader::read_generated_anim_code(Tileset &dest, const ArtifactKey &src_key) const
{
    auto params_result = anim_code_parser_->parse_generated_header(src_key.key());
    if (!params_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to parse generated anim code", FormatParam{src_key.key(), Style::bold}},
            params_result};
    }

    // Update animation params in the Porymap component
    for (const auto &[anim_name, params] : params_result.value()) {
        if (dest.porymap_component().has_anim(anim_name)) {
            dest.porymap_component().anims().at(anim_name).params(params);
        }
        else {
            // Create a new animation with just the params (frames will be loaded separately)
            Animation<IndexPixel> anim{anim_name, params};
            dest.porymap_component().add_anim(std::move(anim));
        }
    }

    return {};
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactReader::read_vanilla_anim_code(Tileset &dest, const ArtifactKey &src_key) const
{
    auto params_result = anim_code_parser_->parse_vanilla_anims(src_key.key(), dest.name());
    if (!params_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to parse vanilla anim code", FormatParam{src_key.key(), Style::bold}},
            params_result};
    }

    // Update animation params in the Porymap component
    for (const auto &[anim_name, params] : params_result.value()) {
        if (dest.porymap_component().has_anim(anim_name)) {
            dest.porymap_component().anims().at(anim_name).params(params);
        }
        else {
            // Create a new animation with just the params (frames will be loaded separately)
            Animation<IndexPixel> anim{anim_name, params};
            dest.porymap_component().add_anim(std::move(anim));
        }
    }

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
    PT_TRY_ASSIGN_PASS_ERR(attributes, attributes_csv_loader_->load(src_key.key()), void);
    for (const auto &[metatile_id, attribute] : attributes) {
        dest.porytiles_component().insert_attribute(metatile_id, attribute);
    }
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
    PT_TRY_CALL_PASS_ERR(import_porytiles_anim_frame(dest, src_key, anim_name, frame_index, *png_rgba_loader_), void);
    return {};
}

[[nodiscard]] ChainableResult<void> ProjectTilesetArtifactReader::read_porytiles_anim_key_frame(
    Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name) const
{
    PT_TRY_CALL_PASS_ERR(import_porytiles_anim_key_frame(dest, src_key, anim_name, *png_rgba_loader_), void);
    return {};
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactReader::read_anim_yaml(Tileset &dest, const ArtifactKey &src_key) const
{
    auto params_result = anim_yaml_parser_->parse(src_key.key());
    if (!params_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to parse anim.yaml", FormatParam{src_key.key(), Style::bold}}, params_result};
    }

    // Update animation params in the Porytiles component
    for (const auto &[anim_name, params] : params_result.value()) {
        if (dest.porytiles_component().has_anim(anim_name)) {
            dest.porytiles_component().anims().at(anim_name).params(params);
        }
        else {
            // Create a new animation with just the params (frames will be loaded separately)
            Animation<Rgba32> anim{anim_name, params};
            dest.porytiles_component().add_anim(std::move(anim));
        }
    }

    return {};
}

} // namespace porytiles2
