#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/base_game.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/infra/algorithms/animation_frame_loader.hpp"
#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/string_utils.hpp"

#include <iostream>

namespace {

using namespace porytiles2;

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
 * @brief Template helper for importing animation frames into a tileset component.
 *
 * @details
 * This function handles the tileset-specific logic for importing animation frames. It uses the shared
 * `load_animation_frame_from_png` helper for PNG loading and tile extraction, then manages the tileset component
 * integration including animation creation, key frame vs regular frame handling, and dimension tracking.
 *
 * @tparam PixelType The pixel type (Rgba32 or IndexPixel)
 * @tparam LoaderType The PNG loader type (must have load_from_file method)
 * @tparam ComponentGetter Callable returning a reference to the tileset component
 * @param dest The destination tileset
 * @param src_key The artifact key for the source PNG file
 * @param project_root The project root path (keys are relative to this)
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
    // Use shared helper to load PNG and extract tiles
    const auto png_path = project_root / src_key.key();
    auto frame_load_result = load_animation_frame_from_png<PixelType>(png_path, frame_name, loader);
    if (!frame_load_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "{}: failed to load {}",
                FormatParam{src_key.key(), Style::bold},
                FormatParam{std::string{error_context}}},
            frame_load_result};
    }

    auto &load_result = frame_load_result.value();

    // Get or create the animation in the component
    auto &component = component_getter(dest);
    if (!component.has_anim(anim_name)) {
        Animation<PixelType> anim{anim_name};
        component.add_anim(std::move(anim));
    }

    auto &anim = component.anims().at(anim_name);

    if (frame_name == "key") {
        // Key frame
        anim.key_frame(std::move(load_result.frame));
    }
    else {
        // Regular frame - use string-based map storage
        anim.put_frame(frame_name, std::move(load_result.frame));
    }

    // Update dimensions in params if not already set
    auto params = anim.params();
    if (params.width_tiles() == 0 && params.height_tiles() == 0) {
        params.width_tiles(load_result.width_tiles);
        params.height_tiles(load_result.height_tiles);
        anim.params(std::move(params));
    }

    return {};
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
    // Keys are relative to project_root_, so prepend for file I/O
    const auto path = project_root_ / src_key.key();
    ChainableResult<std::vector<MetatileAttribute>> attributes_result = base_game_ == BaseGame::pokefirered
                                                                            ? parse_firered_metatile_attributes(path)
                                                                            : parse_emerald_metatile_attributes(path);
    if (!attributes_result.has_value()) {
        return ChainableResult<void>{FormattableError{"failed to read metatile_attributes.bin"}, attributes_result};
    }
    for (auto &attr : attributes_result.value()) {
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

    // Skip param loading if there is no generated header file present
    std::filesystem::path params_path = project_root_ / params_key.key();
    if (!std::filesystem::exists(params_path)) {
        return {};
    }

    // Setup callback info, use Porytiles-managed callback regardless of metadata
    const std::string tileset_shorthand = extract_tileset_shorthand(dest.name());
    const std::string callback_func = "InitTilesetAnim_PorytilesManaged_" + tileset_shorthand;

    // Parse C code for animation params
    auto params_result = anim_code_parser_->parse_from_callback(params_path, callback_func, tileset_shorthand, true);

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
