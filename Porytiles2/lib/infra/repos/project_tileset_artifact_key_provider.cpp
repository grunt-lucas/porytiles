#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/infra/models/project_tileset_metadata.hpp"
#include "porytiles2/infra/repos/tileset_artifact_paths.hpp"
#include "porytiles2/infra/services/anim_yaml_parser.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

// Project source file paths for INCBIN parsing
// TODO: don't harcode
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";
const std::filesystem::path src_graphics_rel_path = std::filesystem::path{"src"} / "graphics.c";
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

// Prefix for parsing callback function names
constexpr std::string init_tileset_anim_prefix = "InitTilesetAnim_";

// Porytiles artifact paths
const std::filesystem::path anim_dir{"anim"};
const std::filesystem::path include_dir{"include"};
const std::filesystem::path generated_anim_code_header{"generated_anim_code.h"};
const std::filesystem::path porytiles_directory{"porytiles"};
const std::filesystem::path bottom_png{"bottom.png"};
const std::filesystem::path middle_png{"middle.png"};
const std::filesystem::path top_png{"top.png"};
const std::filesystem::path attributes_csv{"attributes.csv"};
const std::filesystem::path porytiles_pals{"palettes"};
const std::filesystem::path anim_yaml{"anim.yaml"};
const std::filesystem::path key_frame{"key.png"};
const std::filesystem::path config{"porytiles.yaml"};
const std::filesystem::path local_config{"porytiles.local.yaml"};

/**
 * @brief Extracts Porytiles managed status from a callback function name.
 *
 * @details
 * Parses callback function names to extract the Porytiles-managed status, indicated by presence of a
 * "PorytilesManaged_" prefix:
 * - "InitTilesetAnim_General" -> false
 * - "InitTilesetAnim_PorytilesManaged_General" -> true
 *
 * @param callback_func The callback function name from tileset metadata
 * @return If callback is Porytiles-managed
 */
[[nodiscard]] bool callback_points_to_porytiles_managed(const std::string &callback_func)
{
    const std::string prefix = init_tileset_anim_prefix + anim::porytiles_managed_prefix;
    if (callback_func.starts_with(prefix)) {
        return true;
    }

    return false;
}

/**
 * @brief Ensures INCBIN declarations have been parsed and cached.
 */
[[nodiscard]] ChainableResult<void> ensure_incbins_parsed(
    const std::filesystem::path &project_root,
    bool &incbins_parsed,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format)
{
    if (incbins_parsed) {
        return {};
    }

    // Parse graphics.h for tiles and palettes
    const auto graphics_path = project_root / graphics_rel_path;
    CParserFacade graphics_parser{graphics_path, format};

    auto graphics_result = graphics_parser.parse_incbin_arrays();
    if (!graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format->format(
                "{}: failed to parse graphics INCBINs", FormatParam{graphics_path.string(), Style::bold})},
            graphics_result};
    }

    for (auto &incbin : graphics_result.value()) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    // Parse metatiles.h for metatiles and attributes
    const auto metatiles_path = project_root / metatiles_rel_path;
    CParserFacade metatiles_parser{metatiles_path, format};

    auto metatiles_result = metatiles_parser.parse_incbin_arrays();
    if (!metatiles_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format->format(
                "{}: failed to parse metatiles INCBINs", FormatParam{metatiles_path.string(), Style::bold})},
            metatiles_result};
    }

    for (auto &incbin : metatiles_result.value()) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    // Parse src/graphics.c for General tileset (and potentially others with INCBINs there)
    const auto src_graphics_path = project_root / src_graphics_rel_path;
    CParserFacade src_graphics_parser{src_graphics_path, format};

    auto src_graphics_result = src_graphics_parser.parse_incbin_arrays();
    if (!src_graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format->format(
                "{}: failed to parse graphics INCBINs", FormatParam{src_graphics_path.string(), Style::bold})},
            src_graphics_result};
    }

    for (auto &incbin : src_graphics_result.value()) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    incbins_parsed = true;
    return {};
}

/**
 * @brief Looks up a single INCBIN path from the cache.
 */
[[nodiscard]] ChainableResult<std::string> lookup_incbin_path(
    const std::string &variable_name,
    const std::filesystem::path &project_root,
    bool &incbins_parsed,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format)
{
    if (const auto ensure_result = ensure_incbins_parsed(project_root, incbins_parsed, incbin_vars, format);
        !ensure_result.has_value()) {
        return ChainableResult<std::string>{
            FormattableError{
                format->format("failed to look up INCBIN path for '{}'", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    const auto it = incbin_vars.find(variable_name);
    if (it == incbin_vars.end()) {
        return FormattableError{
            format->format("INCBIN variable '{}' not found", FormatParam{variable_name, Style::bold})};
    }

    if (it->second.paths().empty()) {
        return FormattableError{
            format->format("INCBIN variable '{}' has no paths", FormatParam{variable_name, Style::bold})};
    }

    return it->second.paths().front();
}

/**
 * @brief Looks up multiple INCBIN paths from the cache (for palette arrays).
 */
[[nodiscard]] ChainableResult<std::vector<std::string>> lookup_incbin_paths(
    const std::string &variable_name,
    const std::filesystem::path &project_root,
    bool &incbins_parsed,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format)
{
    auto ensure_result = ensure_incbins_parsed(project_root, incbins_parsed, incbin_vars, format);
    if (!ensure_result.has_value()) {
        return ChainableResult<std::vector<std::string>>{
            FormattableError{
                format->format("failed to look up INCBIN paths for '{}'", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    auto it = incbin_vars.find(variable_name);
    if (it == incbin_vars.end()) {
        return FormattableError{
            format->format("INCBIN variable '{}' not found", FormatParam{variable_name, Style::bold})};
    }

    return it->second.paths();
}

/**
 * @brief Parses an INCBIN variable name to extract the animation name.
 *
 * @details
 * Variable naming patterns:
 * - Vanilla: "gTilesetAnims_General_Flower_Frame0" -> "Flower"
 * - Porytiles: "gTilesetAnims_PorytilesManaged_General_Water_Frame7" -> "Water"
 * - Porytiles: "gTilesetAnims_PorytilesManaged_PorytilesTest_FlowerYellow_FrameCenter" -> "FlowerYellow"
 *
 * Note: Frame names (numeric or arbitrary) are extracted from INCBIN paths, not from variable names.
 */
[[nodiscard]] std::optional<std::string>
parse_anim_frame_var(const std::string &var_name, const std::string &tileset_shorthand, bool porytiles_managed)
{
    std::string prefix = anim::g_tileset_anims_prefix;
    if (porytiles_managed) {
        prefix += anim::porytiles_managed_prefix;
    }
    prefix += tileset_shorthand + "_";

    if (!var_name.starts_with(prefix)) {
        return std::nullopt;
    }

    std::string remainder = var_name.substr(prefix.size());

    auto frame_pos = remainder.find("_Frame");
    if (frame_pos == std::string::npos) {
        return std::nullopt;
    }

    return remainder.substr(0, frame_pos);
}

/**
 * @brief Parses animation frame INCBINs from a C source file.
 */
[[nodiscard]] ChainableResult<AnimationFramePaths> parse_anim_incbins_from_file(
    const std::filesystem::path &c_file,
    const std::string &tileset_name,
    bool porytiles_managed,
    const TextFormatter *format)
{
    CParserFacade parser{c_file, format};
    const std::string tileset_shorthand = tileset_name.substr(std::size("gTileset_") - 1);

    std::string prefix = anim::g_tileset_anims_prefix;
    if (porytiles_managed) {
        prefix += anim::porytiles_managed_prefix;
    }
    prefix += tileset_shorthand + "_";

    auto incbins_result = parser.parse_incbin_arrays(prefix);
    if (!incbins_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{
                format->format("{}: failed to parse animation INCBINs", FormatParam{c_file.string(), Style::bold})},
            incbins_result};
    }

    AnimationFramePaths result;

    for (const auto &incbin : incbins_result.value()) {
        auto anim_name_opt = parse_anim_frame_var(incbin.variable_name(), tileset_shorthand, porytiles_managed);
        if (!anim_name_opt.has_value()) {
            continue;
        }

        std::string snake_anim_name = to_snake_case(anim_name_opt.value());

        if (!incbin.paths().empty()) {
            result[snake_anim_name].emplace_back(incbin.paths().front());
        }
    }

    return result;
}

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths_for(tileset_name),
        format_->format("failed to get metatiles.bin key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{paths.metatiles_path()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths_for(tileset_name),
        format_->format(
            "failed to get metatile_attributes.bin key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{paths.metatile_attributes_path()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const std::string &tileset_name) const
{
    /*
     * TODO: instead of harcoding "tiles.png" here, we should extract the filename from the INCBIN and replace .4bpp
     * extension with .png. This would be a cleaner way to handle things, and could handle a case where the user changed
     * the name of the tiles file.
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths_for(tileset_name),
        format_->format("failed to get tiles.png key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{paths.tiles_path().parent_path() / "tiles.png"};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_pal_n(const std::string &tileset_name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths_for(tileset_name),
        format_->format("failed to get Porymap pal key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{paths.palettes_dir() / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        porymap_animation_frame_paths_for(tileset_name),
        format_->format(
            "failed to get Porymap anim frame key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);

    // Find the animation
    const auto anim_it = frame_paths.find(anim_name);
    if (anim_it == frame_paths.end()) {
        return FormattableError{format_->format(
            "animation '{}' not found for tileset '{}'",
            FormatParam{anim_name, Style::bold},
            FormatParam{tileset_name, Style::bold})};
    }

    // Find the frame by matching stem (e.g., "0" matches "0.4bpp")
    for (const auto &path : anim_it->second) {
        if (path.stem().string() == frame_name) {
            auto png_path = path;
            png_path.replace_extension(".png");
            return ArtifactKey{png_path};
        }
    }

    return FormattableError{format_->format(
        "frame '{}' not found in animation '{}' for tileset '{}'",
        FormatParam{frame_name, Style::bold},
        FormatParam{anim_name, Style::bold},
        FormatParam{tileset_name, Style::bold})};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format(
            "failed to get generated anim code key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / include_dir / generated_anim_code_header};
}

/*
 * Porytiles artifacts
 */
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get bottom.png key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / bottom_png};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_middle_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get middle.png key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / middle_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_top_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get top.png key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / top_png};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get attributes.csv key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / attributes_csv};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_pal_n(const std::string &tileset_name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get Porytiles pal key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / porytiles_pals / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format(
            "failed to get Porytiles anim frame key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_name / (frame_name + std::string{".png"})};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_params(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get anim.yaml key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_yaml};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_config(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get config key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / config};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_local_config(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(tileset_name),
        format_->format("failed to get local config key for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / local_config};
}

bool ProjectTilesetArtifactKeyProvider::artifact_exists(const ArtifactKey &key) const
{
    // Keys are relative to project_root_, so prepend for filesystem operations
    const std::filesystem::path artifact = project_root_ / key.key();
    return std::filesystem::exists(artifact);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        porymap_animation_frame_paths_for(tileset_name),
        format_->format("failed to discover Porymap anims for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::set<std::string>);

    std::set<std::string> anim_names;
    for (const auto &anim_name : frame_paths | std::views::keys) {
        anim_names.insert(anim_name);
    }
    return anim_names;
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        porymap_animation_frame_paths_for(tileset_name),
        format_->format(
            "failed to discover Porymap anim frames for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::set<std::string>);

    std::set<std::string> frame_names;

    const auto it = frame_paths.find(anim_name);
    if (it == frame_paths.end()) {
        return frame_names;
    }

    // Extract basenames from INCBIN paths
    // Path format: "data/tilesets/primary/general/anim/flower/0.4bpp"
    // We want to return: "0"
    for (const auto &path : it->second) {
        // stem() extracts the filename without extension: "0.4bpp" -> "0"
        std::string basename = path.stem().string();
        frame_names.insert(basename);
    }

    return frame_names;
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) const
{
    // Get the anim.yaml path
    PT_TRY_ASSIGN_CHAIN_ERR(
        anim_yaml_key,
        key_for_porytiles_anim_params(tileset_name),
        format_->format("failed to discover Porytiles anims for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::set<std::string>);

    // If anim.yaml doesn't exist, no Porytiles animations
    if (!artifact_exists(anim_yaml_key)) {
        return std::set<std::string>{};
    }

    // Parse the anim.yaml file (snake_case validation is now handled by AnimYamlParser)
    // Keys are relative to project_root_, so prepend for file I/O
    AnimYamlParser parser{format_};
    auto parse_result = parser.parse((project_root_ / anim_yaml_key.key()).string());
    if (!parse_result.has_value()) {
        return ChainableResult<std::set<std::string>>{
            FormattableError{
                format_->format("failed to parse anim.yaml for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            parse_result};
    }

    // Extract animation names
    std::set<std::string> anim_names;
    for (const auto &anim_name : parse_result.value() | std::views::keys) {
        anim_names.insert(anim_name);
    }

    return anim_names;
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    // Get the anim.yaml path
    PT_TRY_ASSIGN_CHAIN_ERR(
        anim_yaml_key,
        key_for_porytiles_anim_params(tileset_name),
        format_->format(
            "failed to discover Porytiles anim frames for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::set<std::string>);

    // If anim.yaml doesn't exist, no frames to discover
    if (!artifact_exists(anim_yaml_key)) {
        return std::set<std::string>{};
    }

    // Parse the anim.yaml file
    // Keys are relative to project_root_, so prepend for file I/O
    AnimYamlParser parser{format_};
    auto parse_result = parser.parse((project_root_ / anim_yaml_key.key()).string());
    if (!parse_result.has_value()) {
        return ChainableResult<std::set<std::string>>{
            FormattableError{
                format_->format("failed to parse anim.yaml for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            parse_result};
    }

    // Find the animation in the parsed map
    const auto &animations = parse_result.value();
    auto it = animations.find(anim_name);
    if (it == animations.end()) {
        return ChainableResult<std::set<std::string>>{FormattableError{format_->format(
            "animation '{}' not found in anim.yaml for tileset '{}'",
            FormatParam{anim_name, Style::bold},
            FormatParam{tileset_name, Style::bold})}};
    }

    // Extract unique frame names from the frames array
    std::set<std::string> frame_names;
    for (const auto &frame_name : it->second.frames()) {
        frame_names.insert(frame_name);
    }

    // Porytiles animations always require a key frame
    // TODO: this is kinda a hack, see TODOLIST.md for ideas on better keyframe handling
    frame_names.insert("key");

    return frame_names;
}

[[nodiscard]] ChainableResult<std::filesystem::path>
ProjectTilesetArtifactKeyProvider::tileset_root(const std::string &tileset_name) const
{
    // Get metadata for tiles_var
    auto metadata_result = metadata_provider_.metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<std::filesystem::path>{
            FormattableError{
                format_->format("failed to get tileset root for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metadata_result};
    }

    // Lookup the tiles INCBIN path
    auto tiles_path_result = ::lookup_incbin_path(
        metadata_result.value().tiles_var(), project_root_, incbins_parsed_, incbin_vars_, format_);
    if (!tiles_path_result.has_value()) {
        return ChainableResult<std::filesystem::path>{
            FormattableError{format_->format(
                "failed to resolve tiles path for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            tiles_path_result};
    }

    // Compute tileset_root from tiles path (e.g., "data/tilesets/primary/general/tiles.4bpp" -> parent)
    // Returns relative path (relative to project_root_)
    std::filesystem::path tiles_path{tiles_path_result.value()};
    return tiles_path.parent_path();
}

ChainableResult<TilesetArtifactPaths>
ProjectTilesetArtifactKeyProvider::artifact_paths_for(const std::string &tileset_name) const
{
    // Get metadata to access variable names
    PT_TRY_ASSIGN_CHAIN_ERR(
        metadata,
        metadata_provider_.metadata_for(tileset_name),
        format_->format("failed to get artifact paths for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        TilesetArtifactPaths);

    // Resolve all INCBIN paths using local helpers
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_path_str,
        ::lookup_incbin_path(metadata.tiles_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("failed to resolve tiles path for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        TilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        palette_path_strs,
        ::lookup_incbin_paths(metadata.palettes_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("failed to resolve palette paths for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        TilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_path_str,
        ::lookup_incbin_path(metadata.metatiles_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("failed to resolve metatiles path for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        TilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_attributes_path_str,
        ::lookup_incbin_path(metadata.metatile_attributes_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format(
            "failed to resolve metatile attributes path for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        TilesetArtifactPaths);

    std::vector<std::filesystem::path> palette_paths;
    palette_paths.reserve(palette_path_strs.size());
    for (const auto &path_str : palette_path_strs) {
        palette_paths.emplace_back(path_str);
    }

    return TilesetArtifactPaths{
        std::filesystem::path{tiles_path_str},
        std::move(palette_paths),
        std::filesystem::path{metatiles_path_str},
        std::filesystem::path{metatile_attributes_path_str}};
}

ChainableResult<AnimationFramePaths>
ProjectTilesetArtifactKeyProvider::porymap_animation_frame_paths_for(const std::string &tileset_name) const
{
    auto metadata_result = metadata_provider_.metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{format_->format(
                "failed to get animation paths for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();
    if (!metadata.has_animations()) {
        // No animations for this tileset
        return AnimationFramePaths{};
    }

    // Compute porytiles_managed and anim_c_file_path locally
    const auto &callback_func = metadata.callback_func();
    bool porytiles_managed = callback_func.has_value() && callback_points_to_porytiles_managed(callback_func.value());

    std::filesystem::path anim_c_file_path;
    if (porytiles_managed) {
        // For Porytiles-managed: <tileset_root>/include/generated_anim_code.h
        // tileset_root() now returns a relative path, so prepend project_root_ for file I/O
        auto tileset_root_result = tileset_root(tileset_name);
        if (!tileset_root_result.has_value()) {
            return ChainableResult<AnimationFramePaths>{
                FormattableError{format_->format(
                    "failed to compute anim_c_file_path for tileset '{}'", FormatParam{tileset_name, Style::bold})},
                tileset_root_result};
        }
        anim_c_file_path = project_root_ / tileset_root_result.value() / "include" / "generated_anim_code.h";
    }
    else {
        // For vanilla: src/tileset_anims.c
        anim_c_file_path = project_root_ / tileset_anims_c_rel_path;
    }

    return parse_anim_incbins_from_file(anim_c_file_path, tileset_name, porytiles_managed, format_);
}

} // namespace porytiles2
