#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"

#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/infra/models/project_tileset_metadata.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_paths.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles;
using namespace porytiles::detail;

const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

// Project source file paths for INCBIN parsing
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";
const std::filesystem::path src_graphics_rel_path = std::filesystem::path{"src"} / "graphics.c";

/// @brief Classifies whether a single INCBIN variable resolves to at least one usable path.
[[nodiscard]] std::optional<IncbinResolutionFailure>
classify_incbin_lookup(const std::map<std::string, IncbinDeclaration> &incbin_vars, const std::string &variable_name)
{
    if (!incbin_vars.contains(variable_name)) {
        return IncbinResolutionFailure::not_found;
    }
    if (incbin_vars.at(variable_name).paths().empty()) {
        return IncbinResolutionFailure::empty_paths;
    }
    return std::nullopt;
}

/// @brief Builds a multi-line error explaining why a tileset's artifact paths could not be resolved.
///
/// @details
/// Lists each failed artifact field with the variable it referenced and the failure reason, then names the source
/// files that were scanned so the user can locate (or add) the missing declaration.
[[nodiscard]] FormattableError build_unresolved_artifact_paths_error(
    const std::string &tileset_name, const std::vector<UnresolvedIncbinVar> &unresolved, const TextFormatter *format)
{
    std::vector<std::string> lines;

    lines.push_back(
        format->format("Could not resolve artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold}));
    lines.emplace_back("");
    lines.emplace_back("The following INCBIN variable(s) could not be resolved:");

    for (const auto &var : unresolved) {
        if (var.reason == IncbinResolutionFailure::not_found) {
            lines.push_back(format->format(
                "  '{}' (field '{}') was not found in any scanned file.",
                FormatParam{var.variable_name, Style::bold},
                FormatParam{var.field_label, Style::bold}));
        }
        else {
            lines.push_back(format->format(
                "  '{}' (field '{}') was found but declared no INCBIN path.",
                FormatParam{var.variable_name, Style::bold},
                FormatParam{var.field_label, Style::bold}));
        }
    }

    lines.emplace_back("");
    lines.push_back(format->format(
        "Scanned files: '{}', '{}', '{}'.",
        FormatParam{graphics_rel_path.string(), Style::bold},
        FormatParam{metatiles_rel_path.string(), Style::bold},
        FormatParam{src_graphics_rel_path.string(), Style::bold}));

    return FormattableError{std::move(lines)};
}

/// @brief Ensures tileset metadata has been parsed from headers.h and cached.
///
/// @details
/// Lazily parses the headers.h file to extract all tileset struct declarations, then immediately converts them into
/// @c ProjectTilesetMetadata objects. The parser types (@c StructInitializerDeclaration) are only used locally within
/// this function and never escape to the header.
///
/// @param project_root The root directory of the pokeemerald-style project
/// @param metadata_parsed Mutable flag tracking whether metadata has been parsed
/// @param tileset_metadata Mutable cache of parsed tileset metadata
/// @param format Text formatter for styled output
/// @return Success or error result
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

    PT_TRY_ASSIGN_CHAIN_ERR(
        struct_decls,
        parser.parse_struct_initializers("gTileset_"),
        void,
        format->format("Failed to parse tileset headers from '{}'.", FormatParam{headers_path.string(), Style::bold}));

    for (auto &struct_decl : struct_decls) {
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

/// @brief Ensures artifact paths have been resolved for all tilesets and cached.
///
/// @details
/// Lazily parses INCBIN declarations from graphics.h, metatiles.h, and src/graphics.c, then resolves paths for each
/// tileset in the metadata cache. The parser types (@c IncbinDeclaration) are only used locally within this function
/// and never escape to the header. Tilesets with unresolvable paths are silently skipped.
///
/// @param project_root The root directory of the pokeemerald-style project
/// @param metadata_parsed Mutable flag tracking whether metadata has been parsed
/// @param tileset_metadata Mutable cache of parsed tileset metadata (must be populated first)
/// @param artifact_paths_parsed Mutable flag tracking whether artifact paths have been resolved
/// @param tileset_artifact_paths Mutable cache of resolved artifact paths
/// @param tileset_unresolved_vars Mutable record of per-tileset artifact fields that failed INCBIN resolution
/// @param format Text formatter for styled output
/// @return Success or error result
[[nodiscard]] ChainableResult<void> ensure_artifact_paths_parsed(
    const std::filesystem::path &project_root,
    bool &metadata_parsed,
    std::map<std::string, ProjectTilesetMetadata> &tileset_metadata,
    bool &artifact_paths_parsed,
    std::map<std::string, ProjectTilesetArtifactPaths> &tileset_artifact_paths,
    std::map<std::string, std::vector<UnresolvedIncbinVar>> &tileset_unresolved_vars,
    const TextFormatter *format)
{
    if (artifact_paths_parsed) {
        return {};
    }

    PT_TRY_CALL_CHAIN_ERR(
        ensure_metadata_parsed(project_root, metadata_parsed, tileset_metadata, format),
        void,
        "Failed to resolve artifact paths.");

    // Parse all INCBIN sources into a local map
    std::map<std::string, IncbinDeclaration> incbin_vars;

    const auto graphics_path = project_root / graphics_rel_path;
    CParserFacade graphics_parser{graphics_path, format};
    PT_TRY_ASSIGN_CHAIN_ERR(
        graphics_incbins,
        graphics_parser.parse_incbin_arrays(),
        void,
        format->format(
            "Failed to parse graphics INCBINs from '{}'.", FormatParam{graphics_path.string(), Style::bold}));
    for (auto &incbin : graphics_incbins) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    const auto metatiles_path = project_root / metatiles_rel_path;
    CParserFacade metatiles_parser{metatiles_path, format};
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_incbins,
        metatiles_parser.parse_incbin_arrays(),
        void,
        format->format(
            "Failed to parse metatiles INCBINs from '{}'.", FormatParam{metatiles_path.string(), Style::bold}));
    for (auto &incbin : metatiles_incbins) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    const auto src_graphics_path = project_root / src_graphics_rel_path;
    CParserFacade src_graphics_parser{src_graphics_path, format};
    PT_TRY_ASSIGN_CHAIN_ERR(
        src_graphics_incbins,
        src_graphics_parser.parse_incbin_arrays(),
        void,
        format->format(
            "Failed to parse graphics INCBINs from '{}'.", FormatParam{src_graphics_path.string(), Style::bold}));
    for (auto &incbin : src_graphics_incbins) {
        incbin_vars.emplace(incbin.variable_name(), std::move(incbin));
    }

    // Resolve paths per-tileset, classifying each of the four artifact fields independently so that a failure can be
    // reported with the exact field, variable, and reason for failure
    for (const auto &[tileset_name, metadata] : tileset_metadata) {
        const std::array<std::pair<std::string, std::string>, 4> fields{{
            {"tiles", metadata.tiles_var()},
            {"palettes", metadata.palettes_var()},
            {"metatiles", metadata.metatiles_var()},
            {"metatileAttributes", metadata.metatile_attributes_var()},
        }};

        std::vector<UnresolvedIncbinVar> unresolved;
        for (const auto &[field_label, variable_name] : fields) {
            auto failure = classify_incbin_lookup(incbin_vars, variable_name);
            if (failure.has_value()) {
                unresolved.push_back(UnresolvedIncbinVar{field_label, variable_name, failure.value()});
            }
        }

        if (!unresolved.empty()) {
            tileset_unresolved_vars.emplace(tileset_name, std::move(unresolved));
            continue;
        }

        const auto &tiles_incbin = incbin_vars.at(metadata.tiles_var());
        const auto &palettes_incbin = incbin_vars.at(metadata.palettes_var());
        const auto &metatiles_incbin = incbin_vars.at(metadata.metatiles_var());
        const auto &attrs_incbin = incbin_vars.at(metadata.metatile_attributes_var());

        std::vector<std::filesystem::path> palette_paths;
        palette_paths.reserve(palettes_incbin.paths().size());
        for (const auto &path_str : palettes_incbin.paths()) {
            palette_paths.emplace_back(path_str);
        }

        tileset_artifact_paths.emplace(
            tileset_name,
            ProjectTilesetArtifactPaths{
                std::filesystem::path{tiles_incbin.paths().front()},
                std::move(palette_paths),
                std::filesystem::path{metatiles_incbin.paths().front()},
                std::filesystem::path{attrs_incbin.paths().front()}});
    }

    artifact_paths_parsed = true;
    return {};
}

} // namespace

namespace porytiles {

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

ChainableResult<std::set<std::string>> ProjectTilesetMetadataProvider::tilesets() const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_metadata_parsed(project_root_, metadata_parsed_, tileset_metadata_, format_),
        std::set<std::string>,
        "Failed to enumerate tilesets.");

    std::set<std::string> names;
    for (const auto &[tileset_name, metadata] : tileset_metadata_) {
        names.insert(tileset_name);
    }
    return names;
}

ChainableResult<ProjectTilesetMetadata>
ProjectTilesetMetadataProvider::metadata_for(const std::string &tileset_name) const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_metadata_parsed(project_root_, metadata_parsed_, tileset_metadata_, format_),
        ProjectTilesetMetadata,
        format_->format("Failed to get metadata for tileset '{}'.", FormatParam{tileset_name, Style::bold}));

    if (!tileset_metadata_.contains(tileset_name)) {
        return FormattableError{
            format_->format("Tileset '{}' not found in headers.h file.", FormatParam{tileset_name, Style::bold})};
    }

    return tileset_metadata_.at(tileset_name);
}

ChainableResult<ProjectTilesetArtifactPaths>
ProjectTilesetMetadataProvider::artifact_paths_for(const std::string &tileset_name) const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_artifact_paths_parsed(
            project_root_,
            metadata_parsed_,
            tileset_metadata_,
            artifact_paths_parsed_,
            tileset_artifact_paths_,
            tileset_unresolved_vars_,
            format_),
        ProjectTilesetArtifactPaths,
        format_->format("Failed to get artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold}));

    if (!tileset_artifact_paths_.contains(tileset_name)) {
        if (tileset_unresolved_vars_.contains(tileset_name)) {
            return build_unresolved_artifact_paths_error(
                tileset_name, tileset_unresolved_vars_.at(tileset_name), format_);
        }
        return FormattableError{format_->format(
            "Could not resolve artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold})};
    }

    return tileset_artifact_paths_.at(tileset_name);
}

void ProjectTilesetMetadataProvider::invalidate_metadata_cache() const
{
    metadata_parsed_ = false;
    tileset_metadata_.clear();
    artifact_paths_parsed_ = false;
    tileset_artifact_paths_.clear();
    tileset_unresolved_vars_.clear();
}

} // namespace porytiles
