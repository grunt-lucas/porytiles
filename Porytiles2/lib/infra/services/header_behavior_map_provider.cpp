#include "porytiles2/infra/services/header_behavior_map_provider.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "porytiles2/utilities/parse_int.hpp"

namespace porytiles2 {

namespace {

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

HeaderBehaviorMapProvider::HeaderBehaviorMapProvider(std::filesystem::path header_path)
    : header_path_{std::move(header_path)}
{
}

std::optional<std::uint16_t> HeaderBehaviorMapProvider::lookup(const std::string &behavior_name) const
{
    ensure_loaded();

    const auto it = name_to_value_.find(behavior_name);
    if (it == name_to_value_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> HeaderBehaviorMapProvider::lookup(std::uint16_t behavior_value) const
{
    ensure_loaded();

    const auto it = value_to_name_.find(behavior_value);
    if (it == value_to_name_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void HeaderBehaviorMapProvider::ensure_loaded() const
{
    if (loaded_) {
        return;
    }

    loaded_ = true;

    if (!std::filesystem::exists(header_path_)) {
        return;
    }

    std::ifstream file{header_path_};
    if (!file.is_open()) {
        return;
    }

    std::uint16_t enum_counter = 0;
    std::string line;

    while (std::getline(file, line)) {
        auto tokens = tokenize_line(line);
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
}

} // namespace porytiles2
