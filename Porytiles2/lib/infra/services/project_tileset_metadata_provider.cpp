#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "porytiles2/infra/models/project_tileset_metadata.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_paths.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

// TODO: don't hardcode these
const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

// Project source file paths for INCBIN parsing
// TODO: don't hardcode
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";
const std::filesystem::path src_graphics_rel_path = std::filesystem::path{"src"} / "graphics.c";

/**
 * @brief Ensures tileset struct headers have been parsed and cached.
 *
 * @details
 * Lazily parses the headers.h file to extract all tileset struct declarations.
 * Results are cached to avoid redundant parsing on subsequent calls.
 *
 * @param project_root The root directory of the pokeemerald-style project
 * @param headers_parsed Mutable flag tracking whether headers have been parsed
 * @param tileset_structs Mutable cache of parsed tileset struct declarations
 * @param format Text formatter for styled output
 * @return Success or error result
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
            FormattableError{format->format(
                "Failed to parse tileset headers from '{}'.", FormatParam{headers_path.string(), Style::bold})},
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
                "Failed to parse graphics INCBINs from '{}'.", FormatParam{graphics_path.string(), Style::bold})},
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
                "Failed to parse metatiles INCBINs from '{}'.", FormatParam{metatiles_path.string(), Style::bold})},
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
                "Failed to parse graphics INCBINs from '{}'.", FormatParam{src_graphics_path.string(), Style::bold})},
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
                format->format("Failed to look up INCBIN path for '{}'.", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    const auto it = incbin_vars.find(variable_name);
    if (it == incbin_vars.end()) {
        return FormattableError{
            format->format("INCBIN variable '{}' not found.", FormatParam{variable_name, Style::bold})};
    }

    if (it->second.paths().empty()) {
        return FormattableError{
            format->format("INCBIN variable '{}' has no paths.", FormatParam{variable_name, Style::bold})};
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
                format->format("Failed to look up INCBIN paths for '{}'.", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    auto it = incbin_vars.find(variable_name);
    if (it == incbin_vars.end()) {
        return FormattableError{
            format->format("INCBIN variable '{}' not found.", FormatParam{variable_name, Style::bold})};
    }

    return it->second.paths();
}

} // namespace

namespace porytiles2 {

bool ProjectTilesetMetadataProvider::exists(const std::string &tileset_name) const
{
    const auto metadata_result = metadata_for(tileset_name);
    return metadata_result.has_value();
}

ChainableResult<bool> ProjectTilesetMetadataProvider::is_secondary(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(metadata, metadata_for(tileset_name), bool);
    return metadata.is_secondary();
}

ChainableResult<bool> ProjectTilesetMetadataProvider::has_animations(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(metadata, metadata_for(tileset_name), bool);
    return metadata.has_animations();
}

ChainableResult<ProjectTilesetMetadata>
ProjectTilesetMetadataProvider::metadata_for(const std::string &tileset_name) const
{
    // Parse headers.h for tileset struct
    if (const auto ensure_result = ensure_headers_parsed(project_root_, headers_parsed_, tileset_structs_, format_);
        !ensure_result.has_value()) {
        return ChainableResult<ProjectTilesetMetadata>{
            FormattableError{
                format_->format("Failed to get metadata for tileset '{}'.", FormatParam{tileset_name, Style::bold})},
            ensure_result};
    }

    auto it = tileset_structs_.find(tileset_name);
    if (it == tileset_structs_.end()) {
        return FormattableError{
            format_->format("Tileset '{}' not found in headers.h file.", FormatParam{tileset_name, Style::bold})};
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
            format_->format("Tileset '{}' missing 'tiles' field.", FormatParam{tileset_name, Style::bold})};
    }
    if (!palettes_var.has_value()) {
        return FormattableError{
            format_->format("Tileset '{}' missing 'palettes' field.", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatiles_var.has_value()) {
        return FormattableError{
            format_->format("Tileset '{}' missing 'metatiles' field.", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatile_attributes_var.has_value()) {
        return FormattableError{format_->format(
            "Tileset '{}' missing 'metatileAttributes' field.", FormatParam{tileset_name, Style::bold})};
    }

    std::optional<std::string> callback_func = std::nullopt;
    if (callback_var.has_value() && callback_var.value() != "NULL") {
        callback_func = callback_var.value();
    }

    return ProjectTilesetMetadata{
        tileset_name,
        is_secondary,
        tiles_var.value(),
        palettes_var.value(),
        metatiles_var.value(),
        metatile_attributes_var.value(),
        callback_func};
}

ChainableResult<ProjectTilesetArtifactPaths>
ProjectTilesetMetadataProvider::artifact_paths_for(const std::string &tileset_name) const
{
    // Get metadata to access variable names
    PT_TRY_ASSIGN_CHAIN_ERR(
        metadata,
        metadata_for(tileset_name),
        format_->format("Failed to get artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold}),
        ProjectTilesetArtifactPaths);

    // Resolve all INCBIN paths using local helpers
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_path_str,
        ::lookup_incbin_path(metadata.tiles_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("Failed to resolve tiles path for tileset '{}'.", FormatParam{tileset_name, Style::bold}),
        ProjectTilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        palette_path_strs,
        ::lookup_incbin_paths(metadata.palettes_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("Failed to resolve palette paths for tileset '{}'.", FormatParam{tileset_name, Style::bold}),
        ProjectTilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_path_str,
        ::lookup_incbin_path(metadata.metatiles_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format("Failed to resolve metatiles path for tileset '{}'.", FormatParam{tileset_name, Style::bold}),
        ProjectTilesetArtifactPaths);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_attributes_path_str,
        ::lookup_incbin_path(metadata.metatile_attributes_var(), project_root_, incbins_parsed_, incbin_vars_, format_),
        format_->format(
            "Failed to resolve metatile attributes path for tileset '{}'.", FormatParam{tileset_name, Style::bold}),
        ProjectTilesetArtifactPaths);

    std::vector<std::filesystem::path> palette_paths;
    palette_paths.reserve(palette_path_strs.size());
    for (const auto &path_str : palette_path_strs) {
        palette_paths.emplace_back(path_str);
    }

    return ProjectTilesetArtifactPaths{
        std::filesystem::path{tiles_path_str},
        std::move(palette_paths),
        std::filesystem::path{metatiles_path_str},
        std::filesystem::path{metatile_attributes_path_str}};
}

} // namespace porytiles2
