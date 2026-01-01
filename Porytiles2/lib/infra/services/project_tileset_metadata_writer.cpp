#include "porytiles2/infra/services/project_tileset_metadata_writer.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

// TODO: don't hardcode this path
// Project source file path for headers.h
const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

} // namespace

namespace porytiles2 {

ProjectTilesetMetadataWriter::ProjectTilesetMetadataWriter(
    std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
    : project_root_{std::move(project_root)}, format_{format}
{
}

ChainableResult<void> ProjectTilesetMetadataWriter::update_callback(
    const std::string &tileset_name, const std::string &new_callback_value) const
{
    const auto headers_path = project_root_ / headers_rel_path;

    // Parse the file to locate the tileset struct
    CParserFacade parser{headers_path, format_};
    auto parse_result = parser.parse_struct_initializers("gTileset_");
    if (!parse_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format(
                "{}: failed to parse tileset headers", FormatParam{headers_path.string(), Style::bold})},
            parse_result};
    }

    // Find the target tileset struct by exact name match
    const StructInitializerDeclaration *target_struct = nullptr;
    for (const auto &struct_decl : parse_result.value()) {
        if (struct_decl.variable_name() == tileset_name) {
            target_struct = &struct_decl;
            break;
        }
    }

    if (target_struct == nullptr) {
        return FormattableError{
            format_->format("tileset '{}' not found in headers.h", FormatParam{tileset_name, Style::bold})};
    }

    // Find the callback field to get its line number
    const DesignatedInitializerField *callback_field = nullptr;
    for (const auto &field : target_struct->fields()) {
        if (field.field_name() == "callback") {
            callback_field = &field;
            break;
        }
    }

    if (callback_field == nullptr) {
        return FormattableError{
            format_->format("tileset '{}' has no .callback field", FormatParam{tileset_name, Style::bold})};
    }

    // Get file lines and replace the callback line
    std::vector<std::string> file_lines = parser.file_lines();
    const std::size_t callback_line_index = callback_field->position().line - 1; // Convert to 0-based index

    if (callback_line_index >= file_lines.size()) {
        return FormattableError{format_->format(
            "callback field line {} out of bounds for file with {} lines",
            FormatParam{std::to_string(callback_field->position().line), Style::bold},
            FormatParam{std::to_string(file_lines.size()), Style::bold})};
    }

    file_lines[callback_line_index] = "    .callback = " + new_callback_value + ",";

    // Write the file back
    std::ofstream out{headers_path};
    if (!out.is_open()) {
        return FormattableError{
            format_->format("{}: failed to open for writing", FormatParam{headers_path.string(), Style::bold})};
    }

    for (const auto &line : file_lines) {
        out << line << '\n';
    }
    out.flush();

    if (out.fail()) {
        return FormattableError{
            format_->format("{}: failed to write file", FormatParam{headers_path.string(), Style::bold})};
    }

    return {};
}

} // namespace porytiles2
