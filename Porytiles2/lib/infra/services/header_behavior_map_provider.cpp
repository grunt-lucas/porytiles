#include "porytiles2/infra/services/header_behavior_map_provider.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

/*
 * TODO: this whole class should provide more robust error printouts when it hits unexpected input
 */

std::vector<std::string> tokenize_line(const std::string &line)
{
    std::vector<std::string> tokens;
    std::istringstream stream{line};
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool try_parse_define_format(
    const std::vector<std::string> &tokens,
    std::unordered_map<std::string, std::uint16_t> &name_to_value,
    std::unordered_map<std::uint16_t, std::string> &value_to_name)
{
    // Define format: #define MB_XXX value
    // tokens[0] = "#define", tokens[1] = "MB_XXX", tokens[2] = "value"
    if (tokens.size() < 3) {
        return false;
    }

    // Check for #define (may be split as "#" and "define" or combined as "#define")
    std::size_t name_index = 0;
    if (tokens[0] == "#define" || (tokens[0] == "#" && tokens.size() >= 2 && tokens[1] == "define")) {
        name_index = (tokens[0] == "#define") ? 1 : 2;
    }
    else {
        return false;
    }

    if (name_index + 1 >= tokens.size()) {
        return false;
    }

    const std::string &behavior_name = tokens[name_index];
    const std::string &value_string = tokens[name_index + 1];

    // Must start with MB_ and not be MB_INVALID
    if (!behavior_name.starts_with("MB_") || behavior_name == "MB_INVALID") {
        return false;
    }

    // Parse value with base 0 (auto-detects hex 0x prefix or decimal)
    auto parse_result = parse_int<int>(value_string, 0);
    if (!parse_result.has_value()) {
        return false;
    }

    const auto value = parse_result.value();
    if (value < 0 || value > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }

    const auto uint_value = static_cast<std::uint16_t>(value);
    name_to_value[behavior_name] = uint_value;
    // Only add to reverse map if not already present (first definition wins)
    if (!value_to_name.contains(uint_value)) {
        value_to_name[uint_value] = behavior_name;
    }
    return true;
}

bool try_parse_enum_format(
    const std::vector<std::string> &tokens,
    std::unordered_map<std::string, std::uint16_t> &name_to_value,
    std::unordered_map<std::uint16_t, std::string> &value_to_name,
    std::uint16_t &enum_counter)
{
    // Enum format: MB_XXX, (with optional trailing comment)
    // tokens[0] = "MB_XXX,"
    if (tokens.empty()) {
        return false;
    }

    std::string first_token = tokens[0];

    // Must start with MB_ and end with comma
    if (!first_token.starts_with("MB_") || first_token.back() != ',') {
        return false;
    }

    // Remove trailing comma to get behavior name
    first_token.pop_back();

    // Skip MB_INVALID
    if (first_token == "MB_INVALID") {
        enum_counter++;
        return true;
    }

    name_to_value[first_token] = enum_counter;
    // Only add to reverse map if not already present (first definition wins)
    if (!value_to_name.contains(enum_counter)) {
        value_to_name[enum_counter] = first_token;
    }
    enum_counter++;
    return true;
}

} // namespace

namespace porytiles2 {

HeaderBehaviorMapProvider::HeaderBehaviorMapProvider(
    const std::filesystem::path &project_root,
    const std::filesystem::path &header_relative_path,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : project_root_{project_root}, header_relative_path_{header_relative_path}, format_{format}, diag_{diag},
      file_printer_{std::make_unique<FileHighlightPrinter>(format)}
{
}

ChainableResult<std::uint16_t> HeaderBehaviorMapProvider::lookup(const std::string &behavior_name) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::uint16_t>{FormattableError{"behavior lookup failed"}, load_result};
    }

    const auto it = name_to_value_.find(behavior_name);
    if (it == name_to_value_.end()) {
        return FormattableError{
            "unknown behavior '{}' not found in '{}'",
            FormatParam{behavior_name, Style::bold},
            FormatParam{header_relative_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<std::string> HeaderBehaviorMapProvider::lookup(std::uint16_t behavior_value) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::string>{FormattableError{"behavior lookup failed"}, load_result};
    }

    const auto it = value_to_name_.find(behavior_value);
    if (it == value_to_name_.end()) {
        return FormattableError{
            "unknown behavior value '{}' not found in '{}'",
            FormatParam{behavior_value, Style::bold},
            FormatParam{header_relative_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<void> HeaderBehaviorMapProvider::ensure_loaded() const
{
    if (loaded_) {
        if (load_failed_) {
            return FormattableError{"behavior header file previously failed to load"};
        }
        return {};
    }

    loaded_ = true;

    const auto header_path = project_root_ / header_relative_path_;

    if (!std::filesystem::exists(header_path)) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(
            format_->format("{}: behavior header file does not exist", FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"behavior header file not found"};
    }

    std::ifstream file{header_path};
    if (!file.is_open()) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(
            format_->format("{}: failed to open behavior header file", FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"failed to open behavior header file"};
    }

    // Slurp the file for FileHighlightPrinter support
    std::string line;
    while (std::getline(file, line)) {
        trim_line_ending(line);
        cached_lines_.push_back(line);
    }

    std::uint16_t enum_counter = 0;

    for (const auto &cached_line : cached_lines_) {
        auto tokens = tokenize_line(cached_line);
        if (tokens.empty()) {
            continue;
        }

        // Try define format first (explicit values take precedence)
        if (try_parse_define_format(tokens, name_to_value_, value_to_name_)) {
            continue;
        }

        // Try enum format (implicit counter-based values)
        try_parse_enum_format(tokens, name_to_value_, value_to_name_, enum_counter);
    }

    if (name_to_value_.empty()) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(format_->format(
            "{}: no behavior definitions found (expected MB_* defines or enum entries)",
            FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"no behavior definitions found in header file"};
    }

    return {};
}

} // namespace porytiles2
