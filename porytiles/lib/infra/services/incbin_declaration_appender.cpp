#include "porytiles/infra/services/incbin_declaration_appender.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "fmt/format.h"

#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles;

// Project source file paths
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";

const std::string porytiles_managed_suffix = "PorytilesManaged_";

/// @brief Generates tiles INCBIN declaration string
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

/// @brief Generates palettes INCBIN declaration string (multi-line)
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

/// @brief Generates metatiles INCBIN declaration string
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

/// @brief Generates metatile attributes INCBIN declaration string.
///
/// @details
/// Selects the C type and INCBIN macro based on attribute_bytes:
/// - 1 -> const u8 / INCBIN_U8
/// - 2 -> const u16 / INCBIN_U16
/// - 4 -> const u32 / INCBIN_U32
[[nodiscard]] std::string generate_attributes_declaration(
    const std::string &shorthand,
    const std::string &bin_path_base,
    const std::string &snake_dir,
    std::size_t attribute_bytes)
{
    const std::string c_type = (attribute_bytes == 4) ? "u32" : (attribute_bytes == 1) ? "u8" : "u16";
    const std::string incbin_macro = (attribute_bytes == 4)   ? "INCBIN_U32"
                                     : (attribute_bytes == 1) ? "INCBIN_U8"
                                                              : "INCBIN_U16";
    return fmt::format(
        "const {} gMetatileAttributes_{}{}[] = {}(\"{}/{}/porytiles_bin/metatile_attributes.bin\");",
        c_type,
        porytiles_managed_suffix,
        shorthand,
        incbin_macro,
        bin_path_base,
        snake_dir);
}

/// @brief Reads all lines from a file into a vector
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

/// @brief Writes all lines to a file
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

/// @brief Checks if a line contains a declaration for the given Porytiles-managed tileset.
///
/// @details
/// Matches `PorytilesManaged_{Shorthand}` only when the shorthand is not a prefix of a
/// longer identifier. Without this boundary check, shorthand "General" would falsely match
/// "PorytilesManaged_GeneralTwo", so removing one tileset's declarations could clobber another's.
[[nodiscard]] bool is_porytiles_managed_declaration(const std::string &line, const std::string &shorthand)
{
    const std::string pattern = porytiles_managed_suffix + shorthand;
    for (std::size_t pos = line.find(pattern); pos != std::string::npos; pos = line.find(pattern, pos + 1)) {
        const std::size_t after = pos + pattern.size();
        if (after >= line.size()) {
            return true;
        }
        const auto next = static_cast<unsigned char>(line[after]);
        if (std::isalnum(next) == 0 && next != '_') {
            return true;
        }
    }
    return false;
}

/// @brief Removes all managed graphics declarations for the given shorthand from a line list.
///
/// @details
/// Drops the single-line tiles declaration and the multi-line palette array. The palette array
/// spans from its `[][16] =` header to the closing `};`, so a skip region is tracked across lines.
[[nodiscard]] std::vector<std::string>
strip_graphics_declarations(const std::vector<std::string> &lines, const std::string &shorthand)
{
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

    return filtered_lines;
}

/// @brief Removes all managed metatiles declarations for the given shorthand from a line list.
///
/// @details
/// The metatiles and attributes declarations are both single-line, so no multi-line tracking is needed.
[[nodiscard]] std::vector<std::string>
strip_metatiles_declarations(const std::vector<std::string> &lines, const std::string &shorthand)
{
    std::vector<std::string> filtered_lines;
    for (const auto &line : lines) {
        if (!is_porytiles_managed_declaration(line, shorthand)) {
            filtered_lines.push_back(line);
        }
    }
    return filtered_lines;
}

/// @brief Pops trailing blank or whitespace-only lines so a repeated upsert stays byte-identical.
void trim_trailing_blank_lines(std::vector<std::string> &lines)
{
    while (!lines.empty() && lines.back().find_first_not_of(" \t") == std::string::npos) {
        lines.pop_back();
    }
}

} // namespace

namespace porytiles {

IncbinDeclarationAppender::IncbinDeclarationAppender(
    std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
    : project_root_{std::move(project_root)}, format_{format}
{
}

ChainableResult<void> IncbinDeclarationAppender::append_graphics_declarations(
    const std::string &tileset_name, const std::string &bin_path_base, std::size_t num_palettes) const
{
    PT_TRY_ASSIGN_PASS_ERR(shorthand, require_tileset_shorthand(tileset_name), void);

    const std::string snake_dir = DynamicCasedName{shorthand}.to_snake_case();
    const auto graphics_path = project_root_ / graphics_rel_path;

    // Read existing file
    PT_TRY_ASSIGN_CHAIN_ERR(
        lines,
        read_file_lines(graphics_path, format_),
        void,
        "Failed to read graphics.h for tileset '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Generate declarations
    const std::string tiles_decl = generate_tiles_declaration(shorthand, bin_path_base, snake_dir);
    auto palettes_decl_lines = generate_palettes_declaration(shorthand, bin_path_base, snake_dir, num_palettes);

    // Remove any existing managed declarations (wherever they are, including copies misplaced inside a trailing
    // preprocessor conditional), then append fresh ones after the last non-blank line, which is always at preprocessor
    // conditional depth 0.
    lines = strip_graphics_declarations(lines, shorthand);
    trim_trailing_blank_lines(lines);

    lines.emplace_back("");
    lines.push_back(tiles_decl);
    lines.emplace_back("");
    lines.append_range(palettes_decl_lines);

    // Write file back
    return write_file_lines(graphics_path, lines, format_);
}

ChainableResult<void> IncbinDeclarationAppender::append_metatiles_declarations(
    const std::string &tileset_name, const std::string &bin_path_base, std::size_t attribute_bytes) const
{
    PT_TRY_ASSIGN_PASS_ERR(shorthand, require_tileset_shorthand(tileset_name), void);

    const std::string snake_dir = DynamicCasedName{shorthand}.to_snake_case();
    const auto metatiles_path = project_root_ / metatiles_rel_path;

    // Read existing file
    PT_TRY_ASSIGN_CHAIN_ERR(
        lines,
        read_file_lines(metatiles_path, format_),
        void,
        "Failed to read metatiles.h for tileset '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Generate declarations
    const std::string metatiles_decl = generate_metatiles_declaration(shorthand, bin_path_base, snake_dir);
    const std::string attributes_decl =
        generate_attributes_declaration(shorthand, bin_path_base, snake_dir, attribute_bytes);

    // Remove any existing managed declarations (wherever they are, including copies misplaced inside a trailing
    // preprocessor conditional), then append fresh ones after the last non-blank line, which is always at preprocessor
    // conditional depth 0.
    lines = strip_metatiles_declarations(lines, shorthand);
    trim_trailing_blank_lines(lines);

    lines.emplace_back("");
    lines.push_back(metatiles_decl);
    lines.push_back(attributes_decl);

    // Write file back
    return write_file_lines(metatiles_path, lines, format_);
}

ChainableResult<void> IncbinDeclarationAppender::remove_declarations(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(shorthand, require_tileset_shorthand(tileset_name), void);

    // Remove from graphics.h
    {
        const auto graphics_path = project_root_ / graphics_rel_path;

        PT_TRY_ASSIGN_CHAIN_ERR(
            lines,
            read_file_lines(graphics_path, format_),
            void,
            "Failed to read graphics.h for tileset '{}'.",
            FormatParam(tileset_name, Style::bold));

        const std::vector<std::string> filtered_lines = strip_graphics_declarations(lines, shorthand);

        auto write_result = write_file_lines(graphics_path, filtered_lines, format_);
        if (!write_result.has_value()) {
            return write_result;
        }
    }

    // Remove from metatiles.h
    {
        const auto metatiles_path = project_root_ / metatiles_rel_path;

        PT_TRY_ASSIGN_CHAIN_ERR(
            lines,
            read_file_lines(metatiles_path, format_),
            void,
            "Failed to read metatiles.h for tileset '{}'.",
            FormatParam(tileset_name, Style::bold));

        const std::vector<std::string> filtered_lines = strip_metatiles_declarations(lines, shorthand);

        auto write_result = write_file_lines(metatiles_path, filtered_lines, format_);
        if (!write_result.has_value()) {
            return write_result;
        }
    }

    return {};
}

} // namespace porytiles
