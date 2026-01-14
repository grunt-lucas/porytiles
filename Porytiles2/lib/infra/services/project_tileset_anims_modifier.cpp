#include "porytiles2/infra/services/project_tileset_anims_modifier.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace {

using namespace porytiles2;

// Path to tileset_anims.c relative to project root
const std::filesystem::path tileset_anims_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

const std::string tileset_prefix = "gTileset_";

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
 * @brief Generates the include directive string for a tileset.
 *
 * @details
 * Produces a line like:
 *     #include "data/tilesets/primary/general/porytiles_bin/include/generated_anim_code.h"
 *
 * @param base_path The base tileset path from config (e.g., "data/tilesets/primary")
 * @param snake_dir The snake_case tileset directory name (e.g., "general")
 */
[[nodiscard]] std::string generate_include_directive(const std::string &base_path, const std::string &snake_dir)
{
    return fmt::format("#include \"{}/{}/porytiles_bin/include/generated_anim_code.h\"", base_path, snake_dir);
}

/**
 * @brief Generates the comment line that precedes the auto-generated include.
 */
[[nodiscard]] std::string generate_porytiles_comment()
{
    return "// [Porytiles] Auto-generated include. Do not remove.";
}

/**
 * @brief Generates the unique pattern to identify an include for a specific tileset.
 *
 * @details
 * Returns the path portion that uniquely identifies this tileset's include,
 * e.g., "general/porytiles_bin/include/generated_anim_code.h"
 *
 * @param snake_dir The snake_case tileset directory name (e.g., "general")
 */
[[nodiscard]] std::string generate_include_pattern(const std::string &snake_dir)
{
    return fmt::format("{}/porytiles_bin/include/generated_anim_code.h", snake_dir);
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

} // namespace

namespace porytiles2 {

ProjectTilesetAnimsModifier::ProjectTilesetAnimsModifier(
    std::filesystem::path project_root,
    gsl::not_null<const InfraConfig *> config,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diagnostics)
    : project_root_{std::move(project_root)}, config_{config}, format_{format}, diagnostics_{diagnostics}
{
}

ChainableResult<void>
ProjectTilesetAnimsModifier::wire_include_for_tileset(const std::string &tileset_name, bool is_secondary) const
{
    // Step 1: Extract and validate shorthand
    const std::string shorthand = extract_shorthand(tileset_name);
    if (shorthand.empty()) {
        return FormattableError{
            format_->format("tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    // Step 2: Convert to snake_case for directory name
    const std::string snake_dir = to_snake_case(shorthand);

    // Step 3: Get base path from config
    auto base_path_result = is_secondary
                                ? config_->tileset_paths_secondary_bin(ConfigScopeType::tileset, tileset_name)
                                : config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name);
    if (!base_path_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to get tileset bin path from config for '{}'", FormatParam{tileset_name, Style::bold}},
            base_path_result};
    }
    const std::string base_path = base_path_result.value().value();

    // Step 4: Read file
    const auto anims_path = project_root_ / tileset_anims_rel_path;
    auto lines_result = read_file_lines(anims_path, format_);
    if (!lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to read tileset_anims.c for tileset '{}'", FormatParam{tileset_name, Style::bold}},
            lines_result};
    }
    auto lines = std::move(lines_result.value());

    // Step 5: Check idempotency - warn and skip if already present
    if (find_existing_include(lines, snake_dir).has_value()) {
        diagnostics_->warning(
            "tileset-anims",
            format_->format(
                "include for '{}' already exists in tileset_anims.c, skipping", FormatParam{tileset_name, Style::bold}));
        return {};
    }

    // Step 6: Generate include directive and comment
    const std::string comment_line = generate_porytiles_comment();
    const std::string include_line = generate_include_directive(base_path, snake_dir);

    // Step 7: Append at the bottom of the file
    lines.push_back("");
    lines.push_back(comment_line);
    lines.push_back(include_line);

    // Step 8: Write file back
    return write_file_lines(anims_path, lines, format_);
}

ChainableResult<void>
ProjectTilesetAnimsModifier::remove_include_for_tileset(const std::string &tileset_name, bool is_secondary) const
{
    // Note: is_secondary is not used in removal since the pattern matching is based solely on
    // the tileset directory name, not the tier. This allows removal to work regardless of which
    // tier the include was originally added for.
    (void)is_secondary;

    // Step 1: Extract and validate shorthand
    const std::string shorthand = extract_shorthand(tileset_name);
    if (shorthand.empty()) {
        return FormattableError{
            format_->format("tileset name '{}' does not start with 'gTileset_'", FormatParam{tileset_name, Style::bold})};
    }

    // Step 2: Convert to snake_case for directory name
    const std::string snake_dir = to_snake_case(shorthand);

    // Step 3: Read file
    const auto anims_path = project_root_ / tileset_anims_rel_path;
    auto lines_result = read_file_lines(anims_path, format_);
    if (!lines_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to read tileset_anims.c for tileset '{}'", FormatParam{tileset_name, Style::bold}},
            lines_result};
    }
    auto lines = std::move(lines_result.value());

    // Step 4: Find existing include line
    const auto existing_index = find_existing_include(lines, snake_dir);
    if (!existing_index.has_value()) {
        diagnostics_->warning(
            "tileset-anims",
            format_->format(
                "include for '{}' not found in tileset_anims.c, skipping removal",
                FormatParam{tileset_name, Style::bold}));
        return {};
    }

    // Step 5: Remove the include line
    const std::size_t include_idx = existing_index.value();
    lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(include_idx));

    // Step 6: Remove the preceding Porytiles comment if present
    // Note: After erasing include_idx, the comment that was at (include_idx - 1) is now at (include_idx - 1)
    if (include_idx > 0 && is_porytiles_comment(lines[include_idx - 1])) {
        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(include_idx - 1));

        // Step 7: Also remove the blank line before the comment if present
        if (include_idx >= 2 && lines[include_idx - 2].empty()) {
            lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(include_idx - 2));
        }
    }

    // Step 8: Write file back
    return write_file_lines(anims_path, lines, format_);
}

} // namespace porytiles2
