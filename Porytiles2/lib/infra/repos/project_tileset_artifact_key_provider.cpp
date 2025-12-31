#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <ranges>
#include <string>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/tileset_metadata.hpp"
#include "porytiles2/infra/repos/tileset_artifact_paths.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

// Porytiles artifact paths
const std::filesystem::path anim_dir{"anim"};
const std::filesystem::path include{"include"};
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

// Project source file paths (for parsing metadata)
const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";
const std::filesystem::path src_graphics_rel_path = std::filesystem::path{"src"} / "graphics.c";
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

// Prefixes for parsing callback function names
constexpr std::string_view init_tileset_anim_prefix = "InitTilesetAnim_";

/**
 * @brief Extracts tileset shorthand from a callback function name.
 *
 * @details
 * Parses callback function names to extract the tileset identifier:
 * - "InitTilesetAnim_General" -> ("General", false)
 * - "InitTilesetAnim_PorytilesManaged_General" -> ("General", true)
 *
 * @param callback_func The callback function name from tileset metadata
 * @return Pair of (tileset_shorthand, is_porytiles_managed), or empty string on parse failure
 */
[[nodiscard]] std::pair<std::string, bool> extract_tileset_from_callback(const std::string &callback_func)
{
    if (!callback_func.starts_with(init_tileset_anim_prefix)) {
        return {"", false};
    }

    std::string remainder = callback_func.substr(init_tileset_anim_prefix.size());

    if (remainder.starts_with(anim::porytiles_managed_prefix)) {
        return {remainder.substr(anim::porytiles_managed_prefix.size()), true};
    }

    return {remainder, false};
}

/**
 * @brief Parses an INCBIN variable name to extract animation name and frame index.
 *
 * @details
 * Variable naming patterns:
 * - Vanilla: "gTilesetAnims_General_Flower_Frame0" -> ("Flower", 0)
 * - Porytiles: "gTilesetAnims_PorytilesManaged_General_Water_Frame7" -> ("Water", 7)
 */
[[nodiscard]] std::optional<std::pair<std::string, std::size_t>>
parse_anim_frame_var(const std::string &var_name, const std::string &tileset_shorthand, bool porytiles_managed)
{
    std::string prefix = "gTilesetAnims_";
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

    std::string anim_name = remainder.substr(0, frame_pos);
    std::string frame_str = remainder.substr(frame_pos + 6);

    try {
        std::size_t frame_index = std::stoull(frame_str);
        return std::make_pair(anim_name, frame_index);
    }
    catch (...) {
        return std::nullopt;
    }
}

/**
 * @brief Parses animation frame INCBINs from a C source file.
 */
[[nodiscard]] ChainableResult<AnimationFramePaths> parse_anim_incbins_from_file(
    const std::filesystem::path &c_file,
    const std::string &tileset_shorthand,
    bool porytiles_managed,
    const TextFormatter *format)
{
    CParserFacade parser{c_file, format};

    std::string prefix = "gTilesetAnims_";
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

    std::map<std::string, std::vector<std::pair<std::size_t, std::filesystem::path>>> grouped;

    for (const auto &incbin : incbins_result.value()) {
        auto parsed = parse_anim_frame_var(incbin.variable_name(), tileset_shorthand, porytiles_managed);
        if (!parsed.has_value()) {
            continue;
        }

        auto [anim_name, frame_index] = parsed.value();
        std::string snake_anim_name = to_snake_case(anim_name);

        if (!incbin.paths().empty()) {
            grouped[snake_anim_name].emplace_back(frame_index, std::filesystem::path{incbin.paths().front()});
        }
    }

    AnimationFramePaths result;
    for (auto &[anim_name, frames] : grouped) {
        std::ranges::sort(frames, [](const auto &a, const auto &b) { return a.first < b.first; });

        std::vector<std::filesystem::path> ordered_paths;
        ordered_paths.reserve(frames.size());
        for (const auto &path : frames | std::views::values) {
            ordered_paths.push_back(path);
        }

        result[anim_name] = std::move(ordered_paths);
    }

    return result;
}

/**
 * @brief Ensures tileset struct headers have been parsed and cached.
 */
[[nodiscard]] ChainableResult<void> ensure_headers_parsed(
    const std::filesystem::path &project_root,
    bool &headers_parsed,
    std::map<std::string, StructInitializerDeclaration> &tileset_structs,
    const TextFormatter *format)
{
    if (headers_parsed) {
        return {};
    }

    const auto headers_path = project_root / headers_rel_path;
    CParserFacade parser{headers_path, format};

    auto parse_result = parser.parse_struct_initializers("gTileset_");
    if (!parse_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                format->format("{}: failed to parse tileset headers", FormatParam{headers_path.string(), Style::bold})},
            parse_result};
    }

    for (auto &struct_decl : parse_result.value()) {
        tileset_structs.emplace(struct_decl.variable_name(), std::move(struct_decl));
    }

    headers_parsed = true;
    return {};
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
 * @brief Retrieves metadata for a specific tileset from the struct cache.
 */
[[nodiscard]] ChainableResult<TilesetMetadata> metadata_for(
    const std::string &tileset_name,
    const std::filesystem::path &project_root,
    bool &headers_parsed,
    std::map<std::string, StructInitializerDeclaration> &tileset_structs,
    const TextFormatter *format)
{
    if (const auto ensure_result = ensure_headers_parsed(project_root, headers_parsed, tileset_structs, format);
        !ensure_result.has_value()) {
        return ChainableResult<TilesetMetadata>{
            FormattableError{
                format->format("failed to get metadata for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            ensure_result};
    }

    auto it = tileset_structs.find(tileset_name);
    if (it == tileset_structs.end()) {
        return FormattableError{
            format->format("tileset '{}' not found in headers.h", FormatParam{tileset_name, Style::bold})};
    }

    const auto &struct_decl = it->second;

    auto is_secondary_opt = struct_decl.field_value("isSecondary");
    bool is_secondary = is_secondary_opt.has_value() && is_secondary_opt.value() == "TRUE";

    auto tiles_var = struct_decl.field_value("tiles");
    auto palettes_var = struct_decl.field_value("palettes");
    auto metatiles_var = struct_decl.field_value("metatiles");
    auto metatile_attributes_var = struct_decl.field_value("metatileAttributes");
    auto callback_var = struct_decl.field_value("callback");

    if (!tiles_var.has_value()) {
        return FormattableError{
            format->format("tileset '{}' missing 'tiles' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!palettes_var.has_value()) {
        return FormattableError{
            format->format("tileset '{}' missing 'palettes' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatiles_var.has_value()) {
        return FormattableError{
            format->format("tileset '{}' missing 'metatiles' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatile_attributes_var.has_value()) {
        return FormattableError{
            format->format("tileset '{}' missing 'metatileAttributes' field", FormatParam{tileset_name, Style::bold})};
    }

    std::optional<std::string> callback_func = std::nullopt;
    if (callback_var.has_value() && callback_var.value() != "NULL") {
        callback_func = callback_var.value();
    }

    return TilesetMetadata{
        tileset_name,
        is_secondary,
        tiles_var.value(),
        palettes_var.value(),
        metatiles_var.value(),
        metatile_attributes_var.value(),
        callback_func};
}

/**
 * @brief Retrieves resolved artifact paths for a specific tileset.
 */
[[nodiscard]] ChainableResult<TilesetArtifactPaths> artifact_paths_for_impl(
    const std::string &tileset_name,
    const std::filesystem::path &project_root,
    bool &headers_parsed,
    bool &incbins_parsed,
    std::map<std::string, StructInitializerDeclaration> &tileset_structs,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format)
{
    auto metadata_result = metadata_for(tileset_name, project_root, headers_parsed, tileset_structs, format);
    if (!metadata_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format->format(
                "failed to get artifact paths for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    auto tiles_path_result =
        lookup_incbin_path(metadata.tiles_var(), project_root, incbins_parsed, incbin_vars, format);
    if (!tiles_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format->format(
                "failed to resolve tiles path for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            tiles_path_result};
    }

    auto palette_paths_result =
        lookup_incbin_paths(metadata.palettes_var(), project_root, incbins_parsed, incbin_vars, format);
    if (!palette_paths_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format->format(
                "failed to resolve palette paths for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            palette_paths_result};
    }

    auto metatiles_path_result =
        lookup_incbin_path(metadata.metatiles_var(), project_root, incbins_parsed, incbin_vars, format);
    if (!metatiles_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format->format(
                "failed to resolve metatiles path for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metatiles_path_result};
    }

    auto metatile_attributes_path_result =
        lookup_incbin_path(metadata.metatile_attributes_var(), project_root, incbins_parsed, incbin_vars, format);
    if (!metatile_attributes_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format->format(
                "failed to resolve metatile attributes path for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metatile_attributes_path_result};
    }

    std::vector<std::filesystem::path> palette_paths;
    palette_paths.reserve(palette_paths_result.value().size());
    for (const auto &path_str : palette_paths_result.value()) {
        palette_paths.emplace_back(path_str);
    }

    return TilesetArtifactPaths{
        std::filesystem::path{tiles_path_result.value()},
        std::move(palette_paths),
        std::filesystem::path{metatiles_path_result.value()},
        std::filesystem::path{metatile_attributes_path_result.value()}};
}

/**
 * @brief Retrieves animation frame paths for a specific tileset.
 */
[[nodiscard]] ChainableResult<AnimationFramePaths> animation_frame_paths_for_impl(
    const std::string &tileset_name,
    const std::filesystem::path &project_root,
    bool &headers_parsed,
    bool &incbins_parsed,
    std::map<std::string, StructInitializerDeclaration> &tileset_structs,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format,
    const UserDiagnostics *diag)
{
    auto metadata_result = metadata_for(tileset_name, project_root, headers_parsed, tileset_structs, format);
    if (!metadata_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{format->format(
                "failed to get animation paths for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    if (!metadata.has_animations()) {
        return AnimationFramePaths{};
    }

    auto [tileset_shorthand, porytiles_managed] = extract_tileset_from_callback(metadata.callback_func().value());

    if (tileset_shorthand.empty()) {
        diag->warning(
            "animation-discovery",
            format->format(
                "could not parse tileset name from callback '{}'",
                FormatParam{metadata.callback_func().value(), Style::bold}));
        return AnimationFramePaths{};
    }

    auto artifact_paths_result = artifact_paths_for_impl(
        tileset_name, project_root, headers_parsed, incbins_parsed, tileset_structs, incbin_vars, format);
    if (!artifact_paths_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{"failed to determine tileset root for animation discovery"}, artifact_paths_result};
    }

    const auto tileset_root = project_root / artifact_paths_result.value().tileset_root();
    const auto generated_header = tileset_root / "include" / "generated_anim_code.h";

    if (std::filesystem::exists(generated_header)) {
        return parse_anim_incbins_from_file(generated_header, tileset_shorthand, true, format);
    }

    const auto anims_c = project_root / tileset_anims_c_rel_path;
    if (!std::filesystem::exists(anims_c)) {
        diag->warning(
            "animation-discovery",
            format->format("tileset_anims.c not found at '{}'", FormatParam{anims_c.string(), Style::bold}));
        return AnimationFramePaths{};
    }

    return parse_anim_incbins_from_file(anims_c, tileset_shorthand, porytiles_managed, format);
}

/**
 * @brief Retrieves animation callback information for a specific tileset.
 */
[[nodiscard]] ChainableResult<std::optional<AnimationCallbackInfo>> animation_callback_info_for_impl(
    const std::string &tileset_name,
    const std::filesystem::path &project_root,
    bool &headers_parsed,
    bool &incbins_parsed,
    std::map<std::string, StructInitializerDeclaration> &tileset_structs,
    std::map<std::string, IncbinDeclaration> &incbin_vars,
    const TextFormatter *format,
    const UserDiagnostics *diag)
{
    auto metadata_result = metadata_for(tileset_name, project_root, headers_parsed, tileset_structs, format);
    if (!metadata_result.has_value()) {
        return ChainableResult<std::optional<AnimationCallbackInfo>>{
            FormattableError{format->format(
                "failed to get animation callback info for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    if (!metadata.has_animations()) {
        return std::optional<AnimationCallbackInfo>{std::nullopt};
    }

    const std::string &callback_func_name = metadata.callback_func().value();

    auto [tileset_shorthand, porytiles_managed] = extract_tileset_from_callback(callback_func_name);

    if (tileset_shorthand.empty()) {
        diag->warning(
            "animation-discovery",
            format->format(
                "could not parse tileset name from callback '{}'", FormatParam{callback_func_name, Style::bold}));
        return std::optional<AnimationCallbackInfo>{std::nullopt};
    }

    auto artifact_paths_result = artifact_paths_for_impl(
        tileset_name, project_root, headers_parsed, incbins_parsed, tileset_structs, incbin_vars, format);
    if (!artifact_paths_result.has_value()) {
        return ChainableResult<std::optional<AnimationCallbackInfo>>{
            FormattableError{"failed to determine tileset root for animation callback discovery"},
            artifact_paths_result};
    }

    const auto tileset_root = project_root / artifact_paths_result.value().tileset_root();
    const auto generated_header = tileset_root / "include" / "generated_anim_code.h";

    std::filesystem::path c_file_path;
    if (std::filesystem::exists(generated_header)) {
        c_file_path = generated_header;
        porytiles_managed = true;
    }
    else {
        c_file_path = project_root / tileset_anims_c_rel_path;
        if (!std::filesystem::exists(c_file_path)) {
            diag->warning(
                "animation-discovery",
                format->format("tileset_anims.c not found at '{}'", FormatParam{c_file_path.string(), Style::bold}));
            return std::optional<AnimationCallbackInfo>{std::nullopt};
        }
    }

    return std::optional<AnimationCallbackInfo>{
        AnimationCallbackInfo{callback_func_name, tileset_shorthand, porytiles_managed, c_file_path}};
}

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get metatiles.bin key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.metatiles_path()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get metatile_attributes.bin key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.metatile_attributes_path()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const std::string &name) const
{
    /*
     * TODO: instead of harcoding "tiles.png" here, we should extract the filename from the INCBIN and replace .4bpp
     * extension with .png. This would be a cleaner way to handle things, and could handle a case where the user changed
     * the name of the tiles file.
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get tiles.png key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.tiles_path().parent_path() / "tiles.png"};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_pal_n(const std::string &name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get Porymap pal key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.palettes_dir() / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &name, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get Porymap anim frame key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / anim_dir / anim_name / (std::to_string(frame_index) + std::string{".png"})};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_generated_anim_code(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get generated anim code key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / include / generated_anim_code_header};
}

/*
 * Porytiles artifacts
 */
ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get bottom.png key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / bottom_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_middle_png(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get middle.png key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / middle_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_top_png(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get top.png key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / top_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get attributes.csv key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / attributes_csv};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_pal_n(const std::string &name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get Porytiles pal key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / porytiles_pals / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &name, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get Porytiles anim frame key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{
        tileset_path / porytiles_directory / anim_dir / anim_name / (std::to_string(frame_index) + std::string{".png"})};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_key_frame(
    const std::string &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get Porytiles anim key frame key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_name / key_frame};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_anim_yaml(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get anim.yaml key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim_dir / anim_yaml};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_config(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get config key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / config};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_local_config(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get local config key for tileset '{}'", FormatParam{name, Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / local_config};
}

bool ProjectTilesetArtifactKeyProvider::artifact_exists(const ArtifactKey &key) const
{
    const std::filesystem::path artifact{key.key()};
    return std::filesystem::exists(artifact);
}

bool ProjectTilesetArtifactKeyProvider::tileset_exists(const std::string &name) const
{
    auto ensure_result = ensure_headers_parsed(project_root_, headers_parsed_, tileset_structs_, format_);
    if (!ensure_result.has_value()) {
        return false;
    }
    return tileset_structs_.contains(name);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to discover Porytiles anims for tileset '{}'", FormatParam{name, Style::bold}),
        std::set<std::string>);
    const auto anims_dir = tileset_path / porytiles_directory / anim_dir;

    std::set<std::string> anim_names;

    if (!std::filesystem::exists(anims_dir) || !std::filesystem::is_directory(anims_dir)) {
        return anim_names;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anims_dir)) {
        if (!entry.is_directory()) {
            // TODO: warn user about stray file in porytiles/anim folder?
            continue;
        }

        // Check if key frame exists (required for Porytiles animations)
        const auto key_frame_path = entry.path() / key_frame;
        if (!std::filesystem::exists(key_frame_path)) {
            // TODO: this is an error condition, an anim folder with no key.png is invalid
            continue;
        }

        // Check if 0.png exists (required for Porytiles animations)
        const auto frame_0_path = entry.path() / "0.png";
        if (!std::filesystem::exists(frame_0_path)) {
            // TODO: this is an error condition, an anim folder with no 0.png is invalid
            continue;
        }

        const auto anim_name = entry.path().filename().string();
        anim_names.insert(anim_name);
    }

    return anim_names;
}

ChainableResult<std::set<int>> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to discover Porytiles anim frames for tileset '{}'", FormatParam{name, Style::bold}),
        std::set<int>);
    const auto anim_path = tileset_path / porytiles_directory / anim_dir / anim_name;

    std::set<int> frame_indices;

    if (!std::filesystem::exists(anim_path) || !std::filesystem::is_directory(anim_path)) {
        return frame_indices;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anim_path)) {
        if (!entry.is_regular_file()) {
            // TODO: warn user about stray folder in porytiles/anim/anim_name folder
            continue;
        }

        const auto filename = entry.path().filename().string();

        if (!filename.ends_with(".png")) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }

        // Skip 00.png (frame 0 is required, not discovered), handled in the main discover_anims method
        if (filename == "0.png") {
            continue;
        }

        // Check if it's a valid number
        const auto frame_str = filename.substr(0, filename.size() - 4); // strip ".png"
        if (!std::ranges::all_of(frame_str, ::isdigit)) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }
        const int frame_index = std::stoi(frame_str);
        frame_indices.insert(frame_index);
    }

    return frame_indices;
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        animation_frame_paths_for_impl(
            name, project_root_, headers_parsed_, incbins_parsed_, tileset_structs_, incbin_vars_, format_, diag_),
        format_->format("failed to discover Porymap anims for tileset '{}'", FormatParam{name, Style::bold}),
        std::set<std::string>);

    std::set<std::string> anim_names;
    for (const auto &anim_name : frame_paths | std::views::keys) {
        anim_names.insert(anim_name);
    }
    return anim_names;
}

ChainableResult<std::set<int>> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        animation_frame_paths_for_impl(
            name, project_root_, headers_parsed_, incbins_parsed_, tileset_structs_, incbin_vars_, format_, diag_),
        format_->format("failed to discover Porymap anim frames for tileset '{}'", FormatParam{name, Style::bold}),
        std::set<int>);

    std::set<int> frame_indices;

    const auto it = frame_paths.find(anim_name);
    if (it == frame_paths.end()) {
        return frame_indices;
    }

    const auto &frames = it->second;
    // Skip frame 0 (required, not discovered) - preserves existing semantics
    for (std::size_t i = 1; i < frames.size(); ++i) {
        frame_indices.insert(static_cast<int>(i));
    }

    return frame_indices;
}

ChainableResult<TilesetArtifactPaths> ProjectTilesetArtifactKeyProvider::artifact_paths(const std::string &name) const
{
    return artifact_paths_for_impl(
        name, project_root_, headers_parsed_, incbins_parsed_, tileset_structs_, incbin_vars_, format_);
}

[[nodiscard]] ChainableResult<std::filesystem::path>
ProjectTilesetArtifactKeyProvider::tileset_root(const std::string &name) const
{
    const auto paths_result = artifact_paths(name);
    if (!paths_result.has_value()) {
        return ChainableResult<std::filesystem::path>{paths_result};
    }

    return project_root_ / paths_result.value().tileset_root();
}

ChainableResult<std::optional<AnimationCallbackInfo>>
ProjectTilesetArtifactKeyProvider::animation_callback_info_for(const std::string &name) const
{
    return animation_callback_info_for_impl(
        name, project_root_, headers_parsed_, incbins_parsed_, tileset_structs_, incbin_vars_, format_, diag_);
}

} // namespace porytiles2
