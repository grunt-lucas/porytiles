#include "porytiles2/infra/services/incbin_declaration_appender.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

// Project source file paths
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";

const std::string tileset_prefix = "gTileset_";
const std::string porytiles_managed_suffix = "PorytilesManaged_";

/**
 * @brief Extracts shorthand from tileset name (e.g., "gTileset_General" -> "General")
 */
[[nodiscard]] std::string extract_shorthand(const std::string &tileset_name)
{
    if (!tileset_name.starts_with(tileset_prefix)) {
        return "";
    }
    return tileset_name.substr(tileset_prefix.size());
}

/**
 * @brief Generates tiles INCBIN declaration string
 */
[[nodiscard]] std::string
generate_tiles_declaration(const std::string &shorthand, const std::string &bin_path_base, const std::string &snake_dir)
{
    return fmt::format(
        "const u32 gTilesetTiles_{}{}[] = INCBIN_U32(\"{}/{}/porytiles_bin/tiles.4bpp.lz\");",
        porytiles_managed_suffix,
        shorthand,
        bin_path_base,
        snake_dir);
}

/**
 * @brief Generates palettes INCBIN declaration string (multi-line)
 */
[[nodiscard]] std::vector<std::string> generate_palettes_declaration(
    const std::string &shorthand,
    const std::string &bin_path_base,
    const std::string &snake_dir,
    std::size_t num_palettes)
{
    std::vector<std::string> lines;

    lines.push_back(fmt::format("const u16 gTilesetPalettes_{}{}[][16] =", porytiles_managed_suffix, shorthand));
    lines.emplace_back("{");

    for (std::size_t i = 0; i < num_palettes; ++i) {
        const std::string comma = (i < num_palettes - 1) ? "," : "";
        lines.push_back(
            fmt::format(
                "    INCBIN_U16(\"{}/{}/porytiles_bin/palettes/{:02}.gbapal\"){}", bin_path_base, snake_dir, i, comma));
    }

    lines.emplace_back("};");
    return lines;
}

/**
 * @brief Generates metatiles INCBIN declaration string
 */
[[nodiscard]] std::string generate_metatiles_declaration(
    const std::string &shorthand, const std::string &bin_path_base, const std::string &snake_dir)
{
    return fmt::format(
        "const u16 gMetatiles_{}{}[] = INCBIN_U16(\"{}/{}/porytiles_bin/metatiles.bin\");",
        porytiles_managed_suffix,
        shorthand,
        bin_path_base,
        snake_dir);
}

/**
 * @brief Generates metatile attributes INCBIN declaration string.
 *
 * @details
 * Selects the C type and INCBIN macro based on metatile_attr_size:
 * - 2 → const u16 / INCBIN_U16
 * - 4 → const u32 / INCBIN_U32
 */
[[nodiscard]] std::string generate_attributes_declaration(
    const std::string &shorthand,
    const std::string &bin_path_base,
    const std::string &snake_dir,
    std::size_t metatile_attr_size)
{
    const std::string c_type = (metatile_attr_size == 4) ? "u32" : "u16";
    const std::string incbin_macro = (metatile_attr_size == 4) ? "INCBIN_U32" : "INCBIN_U16";
    return fmt::format(
        "const {} gMetatileAttributes_{}{}[] = {}(\"{}/{}/porytiles_bin/metatile_attributes.bin\");",
        c_type,
        porytiles_managed_suffix,
        shorthand,
        incbin_macro,
        bin_path_base,
        snake_dir);
}

/**
 * @brief Reads all lines from a file into a vector
 */
[[nodiscard]] ChainableResult<std::vector<std::string>>
read_file_lines(const std::filesystem::path &path, const TextFormatter *format)
{
    std::ifstream in{path};
    if (!in.is_open()) {
        return FormattableError{
            format->format("{}: failed to open for reading", FormatParam{path.string(), Style::bold})};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

/**
 * @brief Writes all lines to a file
 */
[[nodiscard]] ChainableResult<void>
write_file_lines(const std::filesystem::path &path, const std::vector<std::string> &lines, const TextFormatter *format)
{
    std::ofstream out{path};
    if (!out.is_open()) {
        return FormattableError{
            format->format("{}: failed to open for writing", FormatParam{path.string(), Style::bold})};
    }

    for (const auto &line : lines) {
        out << line << '\n';
    }
    out.flush();

    if (out.fail()) {
        return FormattableError{format->format("{}: failed to write file", FormatParam{path.string(), Style::bold})};
    }

    return {};
}

/**
 * @brief Finds the line index after which to append new declarations.
 *
 * @details
 * Searches backwards from the end of the file to find the last non-empty,
 * non-comment line. Returns the index after that line as the insertion point.
 */
[[nodiscard]] std::size_t find_append_position(const std::vector<std::string> &lines)
{
    // Find the last non-empty, non-comment line
    for (std::size_t i = lines.size(); i > 0; --i) {
        const auto &line = lines[i - 1];
        // Skip empty lines and preprocessor directives at the end
        if (!line.empty() && !line.starts_with("#") && line.find_first_not_of(' ') != std::string::npos) {
            return i;
        }
    }
    return lines.size();
}

/**
 * @brief Checks if a line contains a declaration for the given Porytiles-managed tileset
 */
[[nodiscard]] bool is_porytiles_managed_declaration(const std::string &line, const std::string &shorthand)
{
    const std::string pattern = porytiles_managed_suffix + shorthand;
    return line.find(pattern) != std::string::npos;
}

} // namespace

namespace porytiles2 {

IncbinDeclarationAppender::IncbinDeclarationAppender(
    std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
    : project_root_{std::move(project_root)}, format_{format}
{
}

ChainableResult<void> IncbinDeclarationAppender::append_graphics_declarations(
    const std::string &tileset_name, const std::string &bin_path_base, std::size_t num_palettes) const
{
    const std::string shorthand = extract_shorthand(tileset_name);
    if (shorthand.empty()) {
        return FormattableError{format_->format(
            "tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    const std::string snake_dir = DynamicCasedName{shorthand}.to_snake_case();
    const auto graphics_path = project_root_ / graphics_rel_path;

    // Read existing file
    auto lines_result = read_file_lines(graphics_path, format_);
    if (!lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to read graphics.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            lines_result};
    }
    auto lines = std::move(lines_result.value());

    // Generate declarations
    const std::string tiles_decl = generate_tiles_declaration(shorthand, bin_path_base, snake_dir);
    auto palettes_decl_lines = generate_palettes_declaration(shorthand, bin_path_base, snake_dir, num_palettes);

    // Find position and append
    const std::size_t pos = find_append_position(lines);

    // Insert blank line separator
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos), "");

    // Insert tiles declaration
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos) + 1, tiles_decl);

    // Insert blank line before palettes
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos) + 2, "");

    // Insert palettes declaration (multi-line)
    for (std::size_t i = 0; i < palettes_decl_lines.size(); ++i) {
        lines.insert(
            lines.begin() + static_cast<std::ptrdiff_t>(pos) + 3 + static_cast<std::ptrdiff_t>(i),
            palettes_decl_lines[i]);
    }

    // Write file back
    return write_file_lines(graphics_path, lines, format_);
}

ChainableResult<void> IncbinDeclarationAppender::append_metatiles_declarations(
    const std::string &tileset_name, const std::string &bin_path_base, std::size_t metatile_attr_size) const
{
    const std::string shorthand = extract_shorthand(tileset_name);
    if (shorthand.empty()) {
        return FormattableError{format_->format(
            "tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    const std::string snake_dir = DynamicCasedName{shorthand}.to_snake_case();
    const auto metatiles_path = project_root_ / metatiles_rel_path;

    // Read existing file
    auto lines_result = read_file_lines(metatiles_path, format_);
    if (!lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to read metatiles.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            lines_result};
    }
    auto lines = std::move(lines_result.value());

    // Generate declarations
    const std::string metatiles_decl = generate_metatiles_declaration(shorthand, bin_path_base, snake_dir);
    const std::string attributes_decl =
        generate_attributes_declaration(shorthand, bin_path_base, snake_dir, metatile_attr_size);

    // Find position and append
    const std::size_t pos = find_append_position(lines);

    // Insert blank line separator
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos), "");

    // Insert metatiles declaration
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos) + 1, metatiles_decl);

    // Insert attributes declaration
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(pos) + 2, attributes_decl);

    // Write file back
    return write_file_lines(metatiles_path, lines, format_);
}

ChainableResult<void> IncbinDeclarationAppender::remove_declarations(const std::string &tileset_name) const
{
    const std::string shorthand = extract_shorthand(tileset_name);
    if (shorthand.empty()) {
        return FormattableError{format_->format(
            "tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    // Remove from graphics.h
    {
        const auto graphics_path = project_root_ / graphics_rel_path;

        auto lines_result = read_file_lines(graphics_path, format_);
        if (!lines_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{"Failed to read graphics.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                lines_result};
        }
        auto lines = std::move(lines_result.value());

        // Remove lines containing PorytilesManaged_{Shorthand}
        // Also handle multi-line palette arrays by tracking brace depth
        std::vector<std::string> filtered_lines;
        bool in_palette_array = false;

        for (const auto &line : lines) {
            if (is_porytiles_managed_declaration(line, shorthand)) {
                // Check if this starts a multi-line declaration
                if (line.find("[][16] =") != std::string::npos) {
                    in_palette_array = true;
                }
                // Skip this line (single-line declaration or start of multi-line)
                continue;
            }

            if (in_palette_array) {
                // Skip lines until we find the closing brace
                if (line.find("};") != std::string::npos) {
                    in_palette_array = false;
                }
                continue;
            }

            filtered_lines.push_back(line);
        }

        auto write_result = write_file_lines(graphics_path, filtered_lines, format_);
        if (!write_result.has_value()) {
            return write_result;
        }
    }

    // Remove from metatiles.h
    {
        const auto metatiles_path = project_root_ / metatiles_rel_path;

        auto lines_result = read_file_lines(metatiles_path, format_);
        if (!lines_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{
                    "Failed to read metatiles.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                lines_result};
        }
        auto lines = std::move(lines_result.value());

        // Remove lines containing PorytilesManaged_{Shorthand}
        std::vector<std::string> filtered_lines;
        for (const auto &line : lines) {
            if (!is_porytiles_managed_declaration(line, shorthand)) {
                filtered_lines.push_back(line);
            }
        }

        auto write_result = write_file_lines(metatiles_path, filtered_lines, format_);
        if (!write_result.has_value()) {
            return write_result;
        }
    }

    return {};
}

} // namespace porytiles2
