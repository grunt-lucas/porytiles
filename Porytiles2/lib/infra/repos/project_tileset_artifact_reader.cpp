#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/string_utils.hpp"

#include <iostream>

namespace {

using namespace porytiles2;

// TODO: don't hardcode, this is duplicated in ProjectTilesetArtifactKeyProvider
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

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
            std::format(
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
            const auto error_msg = std::format("failed to load layer image: {}", src_key.key());
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
        return FormattableError{std::format(
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
        return FormattableError{std::format(
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
        panic(std::format("invalid pal index {}: out of range", index));
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
        panic(std::format("invalid pal index {}: out of range", index));
    }

    const auto pal_result = loader.load_with_wildcards(src_key.key());
    if (!pal_result.has_value()) {
        return ChainableResult<void>{FormattableError{"failed to load palette file"}, pal_result};
    }
    dest.porytiles_component().set_pal(index, pal_result.value());

    return {};
}

/**
 * @brief Template helper for importing animation frames from PNG files.
 *
 * @details
 * This function unifies the logic for importing animation frames from both Porymap
 * (IndexPixel) and Porytiles (Rgba32) components. It handles both key frames and
 * regular frames through the frame_name parameter.
 *
 * @tparam PixelType The pixel type (Rgba32 or IndexPixel)
 * @tparam LoaderType The PNG loader type (must have load_from_file method)
 * @tparam ComponentGetter Callable returning a reference to the tileset component
 * @param dest The destination tileset
 * @param src_key The artifact key for the source PNG file
 * @param anim_name The name of the animation
 * @param frame_name The frame name ("key" for key frames, otherwise arbitrary string like "0", "1", "left", etc.)
 * @param loader The PNG image loader
 * @param component_getter Lambda to get the appropriate component from tileset
 * @param error_context Description for error messages (e.g., "Porymap animation frame")
 * @return ChainableResult<void> indicating success or failure
 */
template <SupportsTransparency PixelType, typename LoaderType, typename ComponentGetter>
ChainableResult<void> import_anim_frame_impl(
    Tileset &dest,
    const ArtifactKey &src_key,
    const std::string &anim_name,
    const std::string &frame_name,
    const LoaderType &loader,
    ComponentGetter component_getter,
    std::string_view error_context)
{
    auto image_result = loader.load_from_file(src_key.key());
    if (!image_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "{}: failed to load {}",
                FormatParam{src_key.key(), Style::bold},
                FormatParam{std::string{error_context}}},
            image_result};
    }

    // Capture dimensions before extracting tiles
    const auto &img = *image_result.value();
    const std::size_t width_tiles = img.width() / tile::side_length_pix;
    const std::size_t height_tiles = img.height() / tile::side_length_pix;

    auto tiles = extract_tiles_from_image(img);

    AnimationFrame<PixelType> frame{frame_name, std::move(tiles)};
    if (img.palette().has_value()) {
        frame.palette(Palette<Rgba32>{img.palette().value()});
    }

    // Get or create the animation in the component
    auto &component = component_getter(dest);
    if (!component.has_anim(anim_name)) {
        Animation<PixelType> anim{anim_name};
        component.add_anim(std::move(anim));
    }

    auto &anim = component.anims().at(anim_name);

    if (frame_name == "key") {
        // Key frame
        anim.key_frame(std::move(frame));
    }
    else {
        // Regular frame - use string-based map storage
        anim.put_frame(frame_name, std::move(frame));
    }

    // Update dimensions in params if not already set
    auto params = anim.params();
    if (params.width_tiles() == 0 && params.height_tiles() == 0) {
        params.width_tiles(width_tiles);
        params.height_tiles(height_tiles);
        anim.params(std::move(params));
    }

    return {};
}

/**
 * @brief Template helper for reading config files.
 *
 * @details
 * This function unifies the logic for reading config and local_config files.
 *
 * @tparam ConfigSetter Callable that sets the config on the tileset
 * @param dest The destination tileset
 * @param src_key The artifact key for the config file
 * @param config_setter Lambda to set the config lines on the component
 * @return ChainableResult<void> indicating success or failure
 */
template <typename ConfigSetter>
ChainableResult<void> read_config_impl(Tileset &dest, const ArtifactKey &src_key, ConfigSetter config_setter)
{
    std::ifstream config_file{src_key.key()};
    if (!config_file.is_open()) {
        // Config file is optional - if not found, just leave config empty
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(config_file, line)) {
        lines.push_back(std::move(line));
    }

    config_setter(dest, lines);
    return {};
}

/**
 * @brief Extracts the PascalCase tileset name from an animation callback function name.
 *
 * @param callback_func The callback function name (e.g., "InitTilesetAnim_General" or
 * "InitTilesetAnim_PorytilesManaged_General")
 * @param porytiles_managed Whether this is a Porytiles-managed callback
 * @return The tileset name in PascalCase (e.g., "General")
 */
std::string extract_tileset_from_callback(const std::string &callback_func, bool porytiles_managed)
{
    constexpr std::string_view prefix = "InitTilesetAnim_";
    constexpr std::string_view managed_prefix = "InitTilesetAnim_PorytilesManaged_";

    if (porytiles_managed) {
        return callback_func.substr(managed_prefix.size());
    }
    return callback_func.substr(prefix.size());
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

[[nodiscard]] ChainableResult<void> ProjectTilesetArtifactReader::read_porymap_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const
{
    // Load each frame using the unified template helper
    // The first call creates the animation in the component, subsequent calls add frames to it
    for (const auto &[frame_name, frame_key] : frame_keys) {
        PT_TRY_CALL_PASS_ERR(
            (import_anim_frame_impl<IndexPixel>(
                dest,
                frame_key,
                anim_name,
                frame_name,
                *png_indexed_loader_,
                [](Tileset &t) -> PorymapTilesetComponent & { return t.porymap_component(); },
                "Porymap animation frame")),
            void);
    }

    // Get metadata for this tileset to extract callback info
    auto metadata_result = metadata_provider_->metadata_for(dest.name());
    if (!metadata_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to get metadata for tileset '{}'", FormatParam{dest.name(), Style::bold}},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    // Skip param loading if no animations configured in tileset metadata
    if (!metadata.has_animations()) {
        return {};
    }

    // Extract callback info from metadata
    const auto &callback_func = metadata.callback_func().value();
    bool porytiles_managed = callback_func.starts_with("InitTilesetAnim_PorytilesManaged_");
    std::string tileset_shorthand = dest.name().substr(std::size("gTileset_") - 1);

    std::filesystem::path c_path;
    if (porytiles_managed) {
        c_path = params_key.key();
    }
    else {
        c_path = project_root_ / tileset_anims_c_rel_path;
    }

    // Parse C code for animation params
    auto params_result =
        anim_code_parser_->parse_from_callback(c_path, callback_func, tileset_shorthand, porytiles_managed);

    if (!params_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to parse animation code", FormatParam{params_key.key(), Style::bold}},
            params_result};
    }

    // Apply params to the specific animation if found
    if (params_result.value().contains(anim_name)) {
        auto &anim = dest.porymap_component().anims().at(anim_name);
        auto existing_params = anim.params();
        auto new_params = params_result.value().at(anim_name);

        // Preserve dimensions from frame import (C code doesn't have this info)
        new_params.width_tiles(existing_params.width_tiles());
        new_params.height_tiles(existing_params.height_tiles());

        anim.params(std::move(new_params));
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

[[nodiscard]] ChainableResult<void> ProjectTilesetArtifactReader::read_porytiles_anim(
    Tileset &dest,
    const std::string &anim_name,
    const ArtifactKey &params_key,
    const ArtifactKey &key_frame_key,
    const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const
{
    // Parse anim.yaml to get params for this animation
    auto params_result = anim_yaml_parser_->parse(params_key.key());
    if (!params_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"{}: failed to parse anim.yaml", FormatParam{params_key.key(), Style::bold}},
            params_result};
    }

    // Find params for this specific animation
    auto it = params_result.value().find(anim_name);
    if (it == params_result.value().end()) {
        return ChainableResult<void>{FormattableError{
            "{}: animation '{}' not found in anim.yaml",
            FormatParam{params_key.key(), Style::bold},
            FormatParam{anim_name, Style::bold}}};
    }

    const auto &params = it->second;

    // Create the animation with params FIRST, before loading frames
    // This ensures YAML-specified width_tiles/height_tiles take precedence over auto-detected dimensions
    Animation<Rgba32> anim{anim_name, params};
    dest.porytiles_component().add_anim(std::move(anim));

    // Load key frame using the unified template helper
    PT_TRY_CALL_PASS_ERR(
        (import_anim_frame_impl<Rgba32>(
            dest,
            key_frame_key,
            anim_name,
            "key",
            *png_rgba_loader_,
            [](Tileset &t) -> PorytilesTilesetComponent & { return t.porytiles_component(); },
            "Porytiles animation frame")),
        void);

    // Load remaining frames using the unified template helper
    for (const auto &[frame_name, frame_key] : frame_keys) {
        PT_TRY_CALL_PASS_ERR(
            (import_anim_frame_impl<Rgba32>(
                dest,
                frame_key,
                anim_name,
                frame_name,
                *png_rgba_loader_,
                [](Tileset &t) -> PorytilesTilesetComponent & { return t.porytiles_component(); },
                "Porytiles animation frame")),
            void);
    }

    return {};
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactReader::read_config(Tileset &dest, const ArtifactKey &src_key) const
{
    return read_config_impl(dest, src_key, [](Tileset &t, const std::vector<std::string> &lines) {
        t.porytiles_component().config(lines);
    });
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactReader::read_local_config(Tileset &dest, const ArtifactKey &src_key) const
{
    return read_config_impl(dest, src_key, [](Tileset &t, const std::vector<std::string> &lines) {
        t.porytiles_component().local_config(lines);
    });
}

} // namespace porytiles2
