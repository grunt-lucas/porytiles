#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

#include <ranges>

namespace {

using namespace porytiles;

// Project source file path for headers.h
const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";

} // namespace

namespace porytiles {

ProjectTilesetMetadataWriter::ProjectTilesetMetadataWriter(
    std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
    : project_root_{std::move(project_root)}, format_{format}
{
}

ChainableResult<void> ProjectTilesetMetadataWriter::update_callback(
    const std::string &tileset_name, const std::string &new_callback_value) const
{
    return update_fields(tileset_name, {{"callback", new_callback_value}});
}

ChainableResult<void> ProjectTilesetMetadataWriter::update_fields(
    const std::string &tileset_name, const std::map<std::string, std::string> &field_updates) const
{
    if (field_updates.empty()) {
        return {};
    }

    const auto headers_path = project_root_ / headers_rel_path;

    // Parse the file to locate the tileset struct
    CParserFacade parser{headers_path, format_};
    PT_TRY_ASSIGN_CHAIN_ERR(
        struct_decls,
        parser.parse_struct_initializers(std::string{tileset_name_prefix}),
        void,
        format_->format("{}: failed to parse tileset headers", FormatParam{headers_path.string(), Style::bold}));

    // Find the target tileset struct by exact name match
    const StructInitializerDeclaration *target_struct = nullptr;
    for (const auto &struct_decl : struct_decls) {
        if (struct_decl.variable_name() == tileset_name) {
            target_struct = &struct_decl;
            break;
        }
    }

    if (target_struct == nullptr) {
        return FormattableError{
            format_->format("tileset '{}' not found in headers.h", FormatParam{tileset_name, Style::bold})};
    }

    // Build a map of field_name -> line_index for fields we need to update
    std::map<std::string, std::size_t> field_line_indices;
    for (const auto &field : target_struct->fields()) {
        if (field_updates.contains(field.field_name())) {
            field_line_indices[field.field_name()] = field.position().line - 1; // Convert to 0-based
        }
    }

    // Verify all requested fields were found
    for (const auto &field_name : field_updates | std::views::keys) {
        if (!field_line_indices.contains(field_name)) {
            return FormattableError{format_->format(
                "tileset '{}' has no .{} field",
                FormatParam{tileset_name, Style::bold},
                FormatParam{field_name, Style::bold})};
        }
    }

    // Get file lines and update all fields
    std::vector<std::string> file_lines = parser.file_lines();
    for (const auto &[field_name, new_value] : field_updates) {
        const std::size_t line_index = field_line_indices.at(field_name);
        if (line_index >= file_lines.size()) {
            return FormattableError{format_->format(
                ".{} field line {} out of bounds for file with {} lines",
                FormatParam{field_name, Style::bold},
                FormatParam{std::to_string(line_index + 1), Style::bold},
                FormatParam{std::to_string(file_lines.size()), Style::bold})};
        }
        file_lines[line_index] = "    ." + field_name + " = " + new_value + ",";
    }

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

ChainableResult<void> ProjectTilesetMetadataWriter::update_to_porytiles_managed(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(shorthand, require_tileset_shorthand(tileset_name), void);

    std::map<std::string, std::string> updates{
        {"tiles", "gTilesetTiles_PorytilesManaged_" + shorthand},
        {"palettes", "gTilesetPalettes_PorytilesManaged_" + shorthand},
        {"metatiles", "gMetatiles_PorytilesManaged_" + shorthand},
        {"metatileAttributes", "gMetatileAttributes_PorytilesManaged_" + shorthand}};

    return update_fields(tileset_name, updates);
}

ChainableResult<void>
ProjectTilesetMetadataWriter::create_tileset_struct(const std::string &tileset_name, bool is_secondary) const
{
    PT_TRY_ASSIGN_PASS_ERR(shorthand, require_tileset_shorthand(tileset_name), void);

    const auto headers_path = project_root_ / headers_rel_path;

    // First, verify the tileset doesn't already exist
    CParserFacade parser{headers_path, format_};
    PT_TRY_ASSIGN_CHAIN_ERR(
        struct_decls,
        parser.parse_struct_initializers(std::string{tileset_name_prefix}),
        void,
        format_->format("{}: failed to parse tileset headers", FormatParam{headers_path.string(), Style::bold}));

    // Check if tileset already exists
    for (const auto &struct_decl : struct_decls) {
        if (struct_decl.variable_name() == tileset_name) {
            return FormattableError{
                format_->format("tileset '{}' already exists in headers.h", FormatParam{tileset_name, Style::bold})};
        }
    }

    // Generate the new tileset struct with Porytiles-managed field values
    const std::string is_secondary_value = is_secondary ? "TRUE" : "FALSE";
    const std::string tiles_value = "gTilesetTiles_PorytilesManaged_" + shorthand;
    const std::string palettes_value = "gTilesetPalettes_PorytilesManaged_" + shorthand;
    const std::string metatiles_value = "gMetatiles_PorytilesManaged_" + shorthand;
    const std::string attributes_value = "gMetatileAttributes_PorytilesManaged_" + shorthand;

    std::ostringstream struct_text;
    struct_text << "\n";
    struct_text << "const struct Tileset " << tileset_name << " =\n";
    struct_text << "{\n";
    struct_text << "    .isCompressed = TRUE,\n";
    struct_text << "    .isSecondary = " << is_secondary_value << ",\n";
    struct_text << "    .tiles = " << tiles_value << ",\n";
    struct_text << "    .palettes = " << palettes_value << ",\n";
    struct_text << "    .metatiles = " << metatiles_value << ",\n";
    struct_text << "    .metatileAttributes = " << attributes_value << ",\n";
    struct_text << "    .callback = NULL,\n";
    struct_text << "};\n";

    // Append to the end of headers.h
    std::ofstream out{headers_path, std::ios::app};
    if (!out.is_open()) {
        return FormattableError{
            format_->format("{}: failed to open for appending", FormatParam{headers_path.string(), Style::bold})};
    }

    out << struct_text.str();
    out.flush();

    if (out.fail()) {
        return FormattableError{
            format_->format("{}: failed to append tileset struct", FormatParam{headers_path.string(), Style::bold})};
    }

    return {};
}

} // namespace porytiles
