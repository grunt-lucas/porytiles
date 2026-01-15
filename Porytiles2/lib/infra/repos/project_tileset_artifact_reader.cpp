#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>

#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/string_utils.hpp"

#include <iostream>

namespace {

using namespace porytiles2;

// TODO: don't hardcode, this is duplicated in ProjectTilesetArtifactKeyProvider
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

ChainableResult<void> import_layer_png(
    Tileset &dest,
    const ArtifactKey &src_key,
    const std::filesystem::path &project_root,
    const PngRgbaImageLoader &loader,
    const std::function<void(PorytilesTilesetComponent &, const Image<Rgba32> &)> &layer_img_setter)
{
    // Keys are relative to project_root, so prepend for file I/O
    auto image_result = loader.load_from_file((project_root / src_key.key()).string());
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

// NOTE: FireRed metatile attributes parsing is not yet implemented.
// The format is different from Emerald (4 bytes vs 2 bytes) and requires additional work.

ChainableResult<void> import_porytiles_palette(
    Tileset &dest,
    const ArtifactKey &src_key,
    std::size_t index,
    const std::filesystem::path &project_root,
    const FilePalLoader &loader)
{
    if (index >= pal::num_pals) {
        panic(std::format("invalid pal index {}: out of range", index));
    }

    // Keys are relative to project_root, so prepend for file I/O
    const auto pal_result = loader.load_with_wildcards((project_root / src_key.key()).string());
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
    const std::filesystem::path &project_root,
    const std::string &anim_name,
    const std::string &frame_name,
    const LoaderType &loader,
    ComponentGetter component_getter,
    std::string_view error_context)
{
    // Keys are relative to project_root, so prepend for file I/O
    auto image_result = loader.load_from_file((project_root / src_key.key()).string());
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
    // Keys are relative to project_root_, so prepend for file I/O
    PT_TRY_ASSIGN_PASS_ERR(entries, parse_metatiles_bin(project_root_ / src_key.key()), void);
    for (auto &entry : entries) {
        dest.porymap_component().push_back_tilemap_entry(std::move(entry));
    }
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const
{
    // TODO: branch here based on target base game?
    // Keys are relative to project_root_, so prepend for file I/O
    PT_TRY_ASSIGN_PASS_ERR(attributes, parse_emerald_metatile_attributes(project_root_ / src_key.key()), void);
    for (auto &attr : attributes) {
        dest.porymap_component().push_back_attribute(std::move(attr));
    }
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const
{
    // Keys are relative to project_root_, so prepend for file I/O
    PT_TRY_ASSIGN_PASS_ERR(image, load_indexed_png(project_root_ / src_key.key(), *png_indexed_loader_), void);
    dest.porymap_component().tiles_png(*image);
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    if (index >= pal::num_pals) {
        panic(std::format("invalid pal index {}: out of range", index));
    }
    // Keys are relative to project_root_, so prepend for file I/O
    PT_TRY_ASSIGN_PASS_ERR(palette, load_porymap_palette(project_root_ / src_key.key(), *pal_loader_), void);
    dest.porymap_component().set_pal(index, std::move(palette));
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
                project_root_,
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
        // Keys are relative to project_root_, so prepend for file I/O
        c_path = project_root_ / params_key.key();
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
            project_root_,
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
            project_root_,
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
            project_root_,
            *png_rgba_loader_,
            [](PorytilesTilesetComponent &comp, const Image<Rgba32> &img) { comp.top(img); }),
        void);
    return {};
}

ChainableResult<void> ProjectTilesetArtifactReader::read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const
{
    // Keys are relative to project_root_, so prepend for file I/O
    PT_TRY_ASSIGN_PASS_ERR(attributes, attributes_csv_loader_->load((project_root_ / src_key.key()).string()), void);
    for (const auto &[metatile_id, attribute] : attributes) {
        dest.porytiles_component().insert_attribute(metatile_id, attribute);
    }
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactReader::read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const
{
    PT_TRY_CALL_PASS_ERR(import_porytiles_palette(dest, src_key, index, project_root_, *pal_loader_), void);
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
    // Keys are relative to project_root_, so prepend for file I/O
    auto params_result = anim_yaml_parser_->parse((project_root_ / params_key.key()).string());
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

    /*
     * TODO: I think this logic technically double loads the key frame. We can probably remove the:
     *   const ArtifactKey &key_frame_key,
     * param, since it's the caller's responsibility to make sure the key frame actually exists.
     *
     * At some point, if we refactor key frame handling to be a user-selected frame, we'll have to rethink all this
     * anyway.
     */

    // Load key frame using the unified template helper
    PT_TRY_CALL_PASS_ERR(
        (import_anim_frame_impl<Rgba32>(
            dest,
            key_frame_key,
            project_root_,
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
                project_root_,
                anim_name,
                frame_name,
                *png_rgba_loader_,
                [](Tileset &t) -> PorytilesTilesetComponent & { return t.porytiles_component(); },
                "Porytiles animation frame")),
            void);
    }

    return {};
}

} // namespace porytiles2
