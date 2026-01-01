#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"

#include <filesystem>
#include <map>
#include <string>

#include "porytiles2/infra/models/project_tileset_metadata.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

// TODO: don't hardcode these
const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

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

} // namespace

namespace porytiles2 {

bool ProjectTilesetMetadataProvider::tileset_exists(const std::string &tileset_name) const
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
                format_->format("failed to get metadata for tileset '{}'", FormatParam{tileset_name, Style::bold})},
            ensure_result};
    }

    auto it = tileset_structs_.find(tileset_name);
    if (it == tileset_structs_.end()) {
        return FormattableError{
            format_->format("tileset '{}' not found in headers.h", FormatParam{tileset_name, Style::bold})};
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
            format_->format("tileset '{}' missing 'tiles' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!palettes_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'palettes' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatiles_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'metatiles' field", FormatParam{tileset_name, Style::bold})};
    }
    if (!metatile_attributes_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'metatileAttributes' field", FormatParam{tileset_name, Style::bold})};
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

} // namespace porytiles2
