#include "porytiles2/infra/services/project_tileset_anims_modifier.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace {

using namespace porytiles2;

// Paths relative to project root
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";
const std::filesystem::path tileset_anims_h_rel_path = std::filesystem::path{"include"} / "tileset_anims.h";
// Note: "include/" is omitted because pokeemerald's makefile adds "include" to the include path
const std::string porytiles_generated_include_base = "porytiles_generated/tilesets";

/**
 * @brief Generates the include directive string for a tileset.
 *
 * @details
 * Produces a line like:
 *     #include "porytiles_generated/tilesets/general/generated_anim_code.h"
 *
 * The "include/" prefix is omitted because pokeemerald's makefile already adds
 * "include" as an include directory.
 *
 * @param snake_dir The snake_case tileset directory name (e.g., "general")
 */
[[nodiscard]] std::string generate_include_directive(const std::string &snake_dir)
{
    return fmt::format("#include \"{}/{}/generated_anim_code.h\"", porytiles_generated_include_base, snake_dir);
}

/**
 * @brief Generates the comment line that precedes the auto-generated include in .c file.
 */
[[nodiscard]] std::string generate_porytiles_include_comment()
{
    return "// [Porytiles] Auto-generated include. Do not remove.";
}

/**
 * @brief Generates the comment line that precedes the auto-generated declaration in .h file.
 */
[[nodiscard]] std::string generate_porytiles_declaration_comment()
{
    return "// [Porytiles] Auto-generated declaration. Do not remove.";
}

/**
 * @brief Generates the function declaration string for a tileset.
 *
 * @details
 * Produces a line like:
 *     void InitTilesetAnim_PorytilesManaged_General(void);
 *
 * @param shorthand The PascalCase tileset shorthand (e.g., "General", "BattleFrontier")
 */
[[nodiscard]] std::string generate_declaration(const std::string &shorthand)
{
    return fmt::format("void InitTilesetAnim_PorytilesManaged_{}(void);", shorthand);
}

/**
 * @brief Generates the unique pattern to identify a declaration for a specific tileset.
 *
 * @details
 * Returns a pattern that uniquely identifies this tileset's declaration,
 * e.g., "InitTilesetAnim_PorytilesManaged_General"
 *
 * @param shorthand The PascalCase tileset shorthand (e.g., "General")
 */
[[nodiscard]] std::string generate_declaration_pattern(const std::string &shorthand)
{
    return fmt::format("InitTilesetAnim_PorytilesManaged_{}", shorthand);
}

/**
 * @brief Generates the unique pattern to identify an include for a specific tileset.
 *
 * @details
 * Returns the path portion that uniquely identifies this tileset's include,
 * e.g., "general/generated_anim_code.h"
 *
 * @param snake_dir The snake_case tileset directory name (e.g., "general")
 */
[[nodiscard]] std::string generate_include_pattern(const std::string &snake_dir)
{
    return fmt::format("{}/generated_anim_code.h", snake_dir);
}

/**
 * @brief Checks if the file already contains an include for this tileset.
 *
 * @return The line index if found, or std::nullopt if not present.
 */
[[nodiscard]] std::optional<std::size_t>
find_existing_include(const std::vector<std::string> &lines, const std::string &snake_dir)
{
    const std::string pattern = generate_include_pattern(snake_dir);

    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(pattern) != std::string::npos) {
            return i;
        }
    }
    return std::nullopt;
}

/**
 * @brief Checks if the file already contains a declaration for this tileset.
 *
 * @return The line index if found, or std::nullopt if not present.
 */
[[nodiscard]] std::optional<std::size_t>
find_existing_declaration(const std::vector<std::string> &lines, const std::string &shorthand)
{
    const std::string pattern = generate_declaration_pattern(shorthand);

    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(pattern) != std::string::npos) {
            return i;
        }
    }
    return std::nullopt;
}

/**
 * @brief Finds the line index of the #endif guard in a header file.
 *
 * @return The line index of #endif, or std::nullopt if not found.
 */
[[nodiscard]] std::optional<std::size_t> find_endif_guard(const std::vector<std::string> &lines)
{
    for (std::size_t i = lines.size(); i > 0; --i) {
        const auto &line = lines[i - 1];
        if (line.find("#endif") != std::string::npos) {
            return i - 1;
        }
    }
    return std::nullopt;
}

/**
 * @brief Checks if a line is a Porytiles auto-generated comment.
 */
[[nodiscard]] bool is_porytiles_comment(const std::string &line)
{
    return line.find("[Porytiles]") != std::string::npos;
}

/**
 * @brief Reads all lines from a file into a vector
 */
[[nodiscard]] ChainableResult<std::vector<std::string>>
read_file_lines(const std::filesystem::path &path, const UserDiagnostics *diag)
{
    std::ifstream in{path};
    if (!in.is_open()) {
        return FormattableError{
            diag->formatter().format("{}: failed to open for reading", FormatParam{path.string(), Style::bold})};
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
write_file_lines(const std::filesystem::path &path, const std::vector<std::string> &lines, const UserDiagnostics *diag)
{
    std::ofstream out{path};
    if (!out.is_open()) {
        return FormattableError{
            diag->formatter().format("{}: failed to open for writing", FormatParam{path.string(), Style::bold})};
    }

    for (const auto &line : lines) {
        out << line << '\n';
    }
    out.flush();

    if (out.fail()) {
        return FormattableError{
            diag->formatter().format("{}: failed to write file", FormatParam{path.string(), Style::bold})};
    }

    return {};
}

} // namespace

namespace porytiles2 {

ProjectTilesetAnimsModifier::ProjectTilesetAnimsModifier(
    std::filesystem::path project_root,
    gsl::not_null<const InfraConfig *> config,
    gsl::not_null<const UserDiagnostics *> diagnostics)
    : project_root_{std::move(project_root)}, config_{config}, diagnostics_{diagnostics}
{
}

ChainableResult<void>
ProjectTilesetAnimsModifier::wire_include_for_tileset(const std::string &tileset_name, bool is_secondary) const
{
    // Step 1: Validate tileset name starts with expected prefix
    constexpr std::string_view tileset_prefix = "gTileset_";
    if (!tileset_name.starts_with(tileset_prefix)) {
        return FormattableError{diagnostics_->formatter().format(
            "tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    // Step 2: Extract shorthand and convert to snake_case for directory name
    const std::string shorthand = extract_tileset_shorthand(tileset_name);
    const std::string snake_dir = to_snake_case(shorthand);

    // Step 3: Read .c file
    const auto anims_c_path = project_root_ / tileset_anims_c_rel_path;
    auto c_lines_result = read_file_lines(anims_c_path, diagnostics_);
    if (!c_lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to read tileset_anims.c for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            c_lines_result};
    }
    auto c_lines = std::move(c_lines_result.value());

    // Step 4: Read .h file
    const auto anims_h_path = project_root_ / tileset_anims_h_rel_path;
    auto h_lines_result = read_file_lines(anims_h_path, diagnostics_);
    if (!h_lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to read tileset_anims.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            h_lines_result};
    }
    auto h_lines = std::move(h_lines_result.value());

    // Step 5: Check idempotency for .c file - warn and skip if already present
    const bool include_exists = find_existing_include(c_lines, snake_dir).has_value();
    const bool declaration_exists = find_existing_declaration(h_lines, shorthand).has_value();

    if (include_exists && declaration_exists) {
        diagnostics_->remark(
            "wire-tileset-animation",
            "anim include and declaration for '{}' already exist, skipping wire operation",
            FormatParam{tileset_name, Style::bold});
        return {};
    }

    // Step 6: Add include to .c file if not present
    if (!include_exists) {
        const std::string include_comment = generate_porytiles_include_comment();
        const std::string include_line = generate_include_directive(snake_dir);

        c_lines.emplace_back("");
        c_lines.push_back(include_comment);
        c_lines.push_back(include_line);
    }

    // Step 7: Add declaration to .h file if not present
    if (!declaration_exists) {
        // Find #endif to insert before it
        const auto endif_index = find_endif_guard(h_lines);
        if (!endif_index.has_value()) {
            return FormattableError{diagnostics_->formatter().format(
                "failed to find #endif guard in tileset_anims.h for tileset '{}'",
                FormatParam{tileset_name, Style::bold})};
        }

        const std::string decl_comment = generate_porytiles_declaration_comment();
        const std::string decl_line = generate_declaration(shorthand);

        // Insert blank line (if needed), comment, declaration, and trailing blank before #endif
        auto insert_pos = h_lines.begin() + static_cast<std::ptrdiff_t>(endif_index.value());

        // Only insert leading blank if the line before #endif is not already blank
        const std::size_t idx = endif_index.value();
        if (idx == 0 || !h_lines[idx - 1].empty()) {
            insert_pos = h_lines.insert(insert_pos, "");
            ++insert_pos;
        }

        insert_pos = h_lines.insert(insert_pos, decl_comment);
        ++insert_pos;
        insert_pos = h_lines.insert(insert_pos, decl_line);
        ++insert_pos;
        // Add trailing blank line to ensure spacing before #endif
        h_lines.insert(insert_pos, "");
    }

    // Step 8: Write both files back
    auto c_write_result = write_file_lines(anims_c_path, c_lines, diagnostics_);
    if (!c_write_result.has_value()) {
        return c_write_result;
    }

    return write_file_lines(anims_h_path, h_lines, diagnostics_);
}

ChainableResult<void>
ProjectTilesetAnimsModifier::remove_include_for_tileset(const std::string &tileset_name, bool is_secondary) const
{
    // Note: is_secondary is not used in removal since the pattern matching is based solely on
    // the tileset directory name, not the tier. This allows removal to work regardless of which
    // tier the include was originally added for.
    (void)is_secondary;

    // Step 1: Validate tileset name starts with expected prefix
    constexpr std::string_view tileset_prefix = "gTileset_";
    if (!tileset_name.starts_with(tileset_prefix)) {
        return FormattableError{diagnostics_->formatter().format(
            "tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    // Step 2: Extract shorthand and convert to snake_case for directory name
    const std::string shorthand = extract_tileset_shorthand(tileset_name);
    const std::string snake_dir = to_snake_case(shorthand);

    // Step 3: Read .c file
    const auto anims_c_path = project_root_ / tileset_anims_c_rel_path;
    auto c_lines_result = read_file_lines(anims_c_path, diagnostics_);
    if (!c_lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to read tileset_anims.c for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            c_lines_result};
    }
    auto c_lines = std::move(c_lines_result.value());

    // Step 4: Read .h file
    const auto anims_h_path = project_root_ / tileset_anims_h_rel_path;
    auto h_lines_result = read_file_lines(anims_h_path, diagnostics_);
    if (!h_lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to read tileset_anims.h for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            h_lines_result};
    }
    auto h_lines = std::move(h_lines_result.value());

    // Step 5: Check if anything exists to remove
    const auto include_index = find_existing_include(c_lines, snake_dir);
    const auto decl_index = find_existing_declaration(h_lines, shorthand);

    if (!include_index.has_value() && !decl_index.has_value()) {
        diagnostics_->remark(
            "wire-tileset-animation",
            "anim include and declaration for '{}' not found, skipping remove operation",
            FormatParam{tileset_name, Style::bold});
        return {};
    }

    // Step 6: Remove the include line from .c file if present
    if (include_index.has_value()) {
        std::size_t idx = include_index.value();
        c_lines.erase(c_lines.begin() + static_cast<std::ptrdiff_t>(idx));

        // Remove the preceding Porytiles comment if present
        if (idx > 0 && is_porytiles_comment(c_lines[idx - 1])) {
            c_lines.erase(c_lines.begin() + static_cast<std::ptrdiff_t>(idx - 1));
            --idx;

            // Also remove the blank line before the comment if present
            if (idx > 0 && c_lines[idx - 1].empty()) {
                c_lines.erase(c_lines.begin() + static_cast<std::ptrdiff_t>(idx - 1));
            }
        }
    }

    // Step 7: Remove the declaration line from .h file if present
    if (decl_index.has_value()) {
        std::size_t idx = decl_index.value();
        h_lines.erase(h_lines.begin() + static_cast<std::ptrdiff_t>(idx));

        // Remove the preceding Porytiles comment if present
        if (idx > 0 && is_porytiles_comment(h_lines[idx - 1])) {
            h_lines.erase(h_lines.begin() + static_cast<std::ptrdiff_t>(idx - 1));
            --idx;

            // Also remove the blank line before the comment if present
            if (idx > 0 && h_lines[idx - 1].empty()) {
                h_lines.erase(h_lines.begin() + static_cast<std::ptrdiff_t>(idx - 1));
            }
        }
    }

    // Step 8: Write both files back
    auto c_write_result = write_file_lines(anims_c_path, c_lines, diagnostics_);
    if (!c_write_result.has_value()) {
        return c_write_result;
    }

    return write_file_lines(anims_h_path, h_lines, diagnostics_);
}

} // namespace porytiles2
