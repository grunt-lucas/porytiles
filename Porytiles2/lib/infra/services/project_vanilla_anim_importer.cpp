#include "porytiles2/infra/services/project_vanilla_anim_importer.hpp"

#include <vector>

#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/infra/services/anim_code_parser.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Extracts the Pascal-case tileset short name from the full name.
 *
 * @details
 * Converts "gTileset_General" to "General".
 *
 * @param tileset_name The full tileset name (e.g., "gTileset_General")
 * @return The short name (e.g., "General")
 */
[[nodiscard]] std::string extract_tileset_shorthand(const std::string &tileset_name)
{
    constexpr std::string_view prefix = "gTileset_";
    if (tileset_name.starts_with(prefix)) {
        return tileset_name.substr(prefix.size());
    }
    return tileset_name;
}

/**
 * @brief Converts a .4bpp INCBIN path to a .png path.
 *
 * @details
 * Changes the extension from .4bpp to .png.
 *
 * @param path_4bpp The .4bpp path from INCBIN
 * @return The corresponding .png path
 */
[[nodiscard]] std::filesystem::path fourBpp_to_png_path(const std::string &path_4bpp)
{
    std::filesystem::path p{path_4bpp};
    p.replace_extension(".png");
    return p;
}

} // namespace

namespace porytiles2 {

ChainableResult<std::map<std::string, Animation<IndexPixel>>>
ProjectVanillaAnimImporter::import_animations(const std::string &tileset_name) const
{
    std::map<std::string, Animation<IndexPixel>> result;

    // Step 1: Get tileset metadata
    ProjectTilesetMetadataProvider metadata_provider{project_root_, format_, diag_};

    if (!metadata_provider.exists(tileset_name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}};
    }

    auto metadata_result = metadata_provider.metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<std::map<std::string, Animation<IndexPixel>>>{
            FormattableError{"failed to get tileset metadata"}, metadata_result};
    }
    auto metadata = std::move(metadata_result).value();

    if (!metadata.has_animations()) {
        // No animations - return empty map
        return result;
    }

    const std::string callback_func = metadata.callback_func().value();
    const std::string pascal_tileset = extract_tileset_shorthand(tileset_name);

    // TODO: provide some way for user to configure this?
    // Step 2: Parse animation parameters from tileset_anims.c
    const auto tileset_anims_path = project_root_ / "src" / "tileset_anims.c";

    if (!std::filesystem::exists(tileset_anims_path)) {
        return FormattableError{"{}: tileset_anims.c not found", FormatParam{tileset_anims_path.string(), Style::bold}};
    }

    AnimCodeParser anim_parser{format_, diag_};
    auto anim_params_result =
        anim_parser.parse_from_callback(tileset_anims_path, callback_func, pascal_tileset, /*porytiles_managed=*/false);
    if (!anim_params_result.has_value()) {
        return ChainableResult<std::map<std::string, Animation<IndexPixel>>>{
            FormattableError{"failed to parse animation params"}, anim_params_result};
    }
    auto anim_params_map = std::move(anim_params_result).value();

    if (anim_params_map.empty()) {
        // No animations found in callback chain
        return result;
    }

    // Step 3: Parse INCBIN declarations for frame paths
    CParserFacade c_parser{tileset_anims_path, format_};
    const std::string incbin_prefix = "gTilesetAnims_" + pascal_tileset + "_";
    auto incbin_decls_result = c_parser.parse_incbin_arrays(incbin_prefix);
    if (!incbin_decls_result.has_value()) {
        return ChainableResult<std::map<std::string, Animation<IndexPixel>>>{
            FormattableError{"failed to parse INCBIN declarations"}, incbin_decls_result};
    }
    auto incbin_decls = std::move(incbin_decls_result).value();

    // Build map: frame variable name -> .png file path
    std::map<std::string, std::filesystem::path> frame_paths;
    for (const auto &decl : incbin_decls) {
        if (decl.variable_name().find("_Frame") != std::string::npos && !decl.paths().empty()) {
            frame_paths[decl.variable_name()] = project_root_ / fourBpp_to_png_path(decl.paths().front());
        }
    }

    // Step 4: For each animation, construct Animation<IndexPixel> with frame data
    PngIndexedImageLoader png_loader;

    for (const auto &[anim_name, params] : anim_params_map) {
        Animation<IndexPixel> anim{anim_name};
        anim.params(params);

        // Load and extract tiles from each frame PNG
        const std::string pascal_anim_name = to_pascal_case(anim_name);

        for (const auto &frame_name : params.frame_names()) {
            const std::string frame_var =
                "gTilesetAnims_" + pascal_tileset + "_" + pascal_anim_name + "_Frame" + frame_name;

            auto it = frame_paths.find(frame_var);
            if (it == frame_paths.end()) {
                return FormattableError{
                    "frame variable '{}' not found in INCBIN declarations for frame '{}'",
                    FormatParam{frame_var, Style::bold},
                    FormatParam{frame_name, Style::bold}};
            }

            const auto &frame_png_path = it->second;
            if (!std::filesystem::exists(frame_png_path)) {
                return FormattableError{
                    "frame PNG '{}' not found for frame '{}'",
                    FormatParam{frame_png_path.string(), Style::bold},
                    FormatParam{frame_name, Style::bold}};
            }

            auto frame_png_result = png_loader.load_from_file(frame_png_path);
            if (!frame_png_result.has_value()) {
                return ChainableResult<std::map<std::string, Animation<IndexPixel>>>{
                    FormattableError{
                        "failed to load frame PNG '{}' for frame '{}'",
                        FormatParam{frame_png_path.string(), Style::bold},
                        FormatParam{frame_name, Style::bold}},
                    frame_png_result};
            }

            const Image<IndexPixel> &frame_png = *frame_png_result.value();
            std::vector<PixelTile<IndexPixel>> tiles = extract_tiles_from_image(frame_png);

            AnimationFrame<IndexPixel> frame{frame_name, std::move(tiles)};
            anim.put_frame(frame_name, std::move(frame));
        }

        result[anim_name] = std::move(anim);
    }

    return result;
}

} // namespace porytiles2
