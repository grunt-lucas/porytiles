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
 * @brief Ensures tileset metadata has been parsed from headers.h and cached.
 *
 * @details
 * Lazily parses the headers.h file to extract all tileset struct declarations, then immediately converts them into
 * @c ProjectTilesetMetadata objects. The parser types (@c StructInitializerDeclaration) are only used locally within
 * this function and never escape to the header.
 *
 * @param project_root The root directory of the pokeemerald-style project
 * @param metadata_parsed Mutable flag tracking whether metadata has been parsed
 * @param tileset_metadata Mutable cache of parsed tileset metadata
 * @param format Text formatter for styled output
 * @return Success or error result
 */
[[nodiscard]] ChainableResult<void> ensure_metadata_parsed(
    const std::filesystem::path &project_root,
    bool &metadata_parsed,
    std::map<std::string, ProjectTilesetMetadata> &tileset_metadata,
    const TextFormatter *format)
{
    if (metadata_parsed) {
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
        const auto &tileset_name = struct_decl.variable_name();

        auto is_secondary_opt = struct_decl.field_value("isSecondary");
        bool is_secondary = is_secondary_opt.has_value() && is_secondary_opt.value() == "TRUE";

        auto tiles_var = struct_decl.field_value("tiles");
        auto palettes_var = struct_decl.field_value("palettes");
        auto metatiles_var = struct_decl.field_value("metatiles");
        auto metatile_attributes_var = struct_decl.field_value("metatileAttributes");
        auto callback_var = struct_decl.field_value("callback");

        if (!tiles_var.has_value() || !palettes_var.has_value() || !metatiles_var.has_value() ||
            !metatile_attributes_var.has_value()) {
            continue;
        }

        std::optional<std::string> callback_func = std::nullopt;
        if (callback_var.has_value() && callback_var.value() != "NULL") {
            callback_func = callback_var.value();
        }

        tileset_metadata.emplace(
            tileset_name,
            ProjectTilesetMetadata{
                tileset_name,
                is_secondary,
                tiles_var.value(),
                palettes_var.value(),
                metatiles_var.value(),
                metatile_attributes_var.value(),
                callback_func});
    }

    metadata_parsed = true;
    return {};
}

/**
 * @brief Ensures artifact paths have been resolved for all tilesets and cached.
 *
 * @details
 * Lazily parses INCBIN declarations from graphics.h, metatiles.h, and src/graphics.c, then resolves paths for each
 * tileset in the metadata cache. The parser types (@c IncbinDeclaration) are only used locally within this function
 * and never escape to the header. Tilesets with unresolvable paths are silently skipped.
 *
 * @param project_root The root directory of the pokeemerald-style project
 * @param metadata_parsed Mutable flag tracking whether metadata has been parsed
 * @param tileset_metadata Mutable cache of parsed tileset metadata (must be populated first)
 * @param artifact_paths_parsed Mutable flag tracking whether artifact paths have been resolved
 * @param tileset_artifact_paths Mutable cache of resolved artifact paths
 * @param format Text formatter for styled output
 * @return Success or error result
 */
[[nodiscard]] ChainableResult<void> ensure_artifact_paths_parsed(
    const std::filesystem::path &project_root,
    bool &metadata_parsed,
    std::map<std::string, ProjectTilesetMetadata> &tileset_metadata,
    bool &artifact_paths_parsed,
    std::map<std::string, ProjectTilesetArtifactPaths> &tileset_artifact_paths,
    const TextFormatter *format)
{
    if (artifact_paths_parsed) {
        return {};
    }

    if (const auto ensure_result = ensure_metadata_parsed(project_root, metadata_parsed, tileset_metadata, format);
        !ensure_result.has_value()) {
        return ChainableResult<void>{FormattableError{"Failed to resolve artifact paths."}, ensure_result};
    }

    // Parse all INCBIN sources into a local map
    std::map<std::string, IncbinDeclaration> incbin_vars;

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

    // Resolve paths per-tileset
    for (const auto &[tileset_name, metadata] : tileset_metadata) {
        auto tiles_it = incbin_vars.find(metadata.tiles_var());
        auto palettes_it = incbin_vars.find(metadata.palettes_var());
        auto metatiles_it = incbin_vars.find(metadata.metatiles_var());
        auto attrs_it = incbin_vars.find(metadata.metatile_attributes_var());

        if (tiles_it == incbin_vars.end() || palettes_it == incbin_vars.end() || metatiles_it == incbin_vars.end() ||
            attrs_it == incbin_vars.end()) {
            continue;
        }

        if (tiles_it->second.paths().empty() || palettes_it->second.paths().empty() ||
            metatiles_it->second.paths().empty() || attrs_it->second.paths().empty()) {
            continue;
        }

        std::vector<std::filesystem::path> palette_paths;
        palette_paths.reserve(palettes_it->second.paths().size());
        for (const auto &path_str : palettes_it->second.paths()) {
            palette_paths.emplace_back(path_str);
        }

        tileset_artifact_paths.emplace(
            tileset_name,
            ProjectTilesetArtifactPaths{
                std::filesystem::path{tiles_it->second.paths().front()},
                std::move(palette_paths),
                std::filesystem::path{metatiles_it->second.paths().front()},
                std::filesystem::path{attrs_it->second.paths().front()}});
    }

    artifact_paths_parsed = true;
    return {};
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
    if (const auto ensure_result = ensure_metadata_parsed(project_root_, metadata_parsed_, tileset_metadata_, format_);
        !ensure_result.has_value()) {
        return ChainableResult<ProjectTilesetMetadata>{
            FormattableError{
                format_->format("Failed to get metadata for tileset '{}'.", FormatParam{tileset_name, Style::bold})},
            ensure_result};
    }

    if (!tileset_metadata_.contains(tileset_name)) {
        return FormattableError{
            format_->format("Tileset '{}' not found in headers.h file.", FormatParam{tileset_name, Style::bold})};
    }

    return tileset_metadata_.at(tileset_name);
}

ChainableResult<ProjectTilesetArtifactPaths>
ProjectTilesetMetadataProvider::artifact_paths_for(const std::string &tileset_name) const
{
    if (const auto ensure_result = ensure_artifact_paths_parsed(
            project_root_,
            metadata_parsed_,
            tileset_metadata_,
            artifact_paths_parsed_,
            tileset_artifact_paths_,
            format_);
        !ensure_result.has_value()) {
        return ChainableResult<ProjectTilesetArtifactPaths>{
            FormattableError{format_->format(
                "Failed to get artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold})},
            ensure_result};
    }

    if (!tileset_artifact_paths_.contains(tileset_name)) {
        return FormattableError{format_->format(
            "Could not resolve artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold})};
    }

    return tileset_artifact_paths_.at(tileset_name);
}

} // namespace porytiles2
