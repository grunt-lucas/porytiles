#include "porytiles2/infra/services/project_tileset_name_provider.hpp"

#include <filesystem>

#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path tileset_header_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

} // namespace

namespace porytiles2 {

ChainableResult<std::set<TilesetName>> ProjectTilesetNameProvider::all_tileset_names() const
{
    const auto tileset_header_full_path = project_root_ / tileset_header_path;
    if (!exists(tileset_header_full_path)) {
        return FormattableError{
            "{}: expected tileset headers file not found", FormatParam{tileset_header_full_path, Style::bold}};
    }

    CParserFacade parser{tileset_header_full_path, format_};

    // Parse struct variables with gTileset_ prefix
    auto parse_result = parser.parse_struct_variables(TilesetName::prefix);
    if (!parse_result.has_value()) {
        return ChainableResult<std::set<TilesetName>>{
            FormattableError{"{}: failed to extract tileset names", FormatParam{tileset_header_full_path, Style::bold}},
            parse_result};
    }

    // Convert struct variable names to TilesetName objects
    std::set<TilesetName> tileset_names;
    for (const auto &struct_var : parse_result.value()) {
        auto tileset_name_result = TilesetName::from(struct_var.variable_name());
        if (!tileset_name_result.has_value()) {
            // This should not happen since we filtered by prefix, but handle gracefully
            return ChainableResult<std::set<TilesetName>>{
                FormattableError{
                    "{}: invalid tileset name '{}'",
                    FormatParam{tileset_header_full_path, Style::bold},
                    FormatParam{struct_var.variable_name(), Style::bold}},
                tileset_name_result};
        }
        tileset_names.insert(std::move(tileset_name_result).value());
    }

    return tileset_names;
}

} // namespace porytiles2
