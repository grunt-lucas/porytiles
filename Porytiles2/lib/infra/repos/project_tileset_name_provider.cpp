#include "porytiles2/infra/repos/project_tileset_name_provider.hpp"

#include <filesystem>
#include <string>

#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

// TODO: make this path configurable
const std::filesystem::path tileset_header_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

} // namespace

namespace porytiles2 {

ChainableResult<std::set<std::string>> ProjectTilesetNameProvider::all_tileset_names() const
{
    const auto tileset_header_full_path = project_root_ / tileset_header_path;
    if (!exists(tileset_header_full_path)) {
        return FormattableError{
            "{}: expected tileset headers file not found", FormatParam{tileset_header_full_path, Style::bold}};
    }

    CParserFacade parser{tileset_header_full_path, format_};

    // Parse struct variables with gTileset_ prefix
    // TODO: don't hardcode gTileset_
    auto parse_result = parser.parse_struct_variables("gTileset_");
    if (!parse_result.has_value()) {
        return ChainableResult<std::set<std::string>>{
            FormattableError{
                "'{}': Failed to extract tileset names.", FormatParam{tileset_header_full_path, Style::bold}},
            parse_result};
    }

    // Convert struct variable names to TilesetName objects
    std::set<std::string> tileset_names;
    for (const auto &struct_var : parse_result.value()) {
        tileset_names.insert(struct_var.variable_name());
    }

    return tileset_names;
}

} // namespace porytiles2
