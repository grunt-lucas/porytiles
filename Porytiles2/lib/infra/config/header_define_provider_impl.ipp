#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "porytiles2/infra/config/config_provider.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

// The anonymous namespace ensures internal linkage per translation unit
// This file is intentionally included only in header_define_provider.cpp
namespace {

using namespace porytiles2;

// Static cache shared across all HeaderDefineProvider instances
std::map<std::filesystem::path, std::vector<std::string>> header_file_lines_cache;

/**
 * @brief Result of searching for a #define in a header file.
 *
 * @details
 * Contains the line number where the define was found and the string value extracted from it.
 */
struct DefineSearchResult {
    std::size_t line_number; // 0-indexed
    std::string value_string;
};

/**
 * @brief Gets the content of a specific line from a cached header file.
 *
 * @details
 * Returns the content of the specified line from the file contents cache. If the file is not cached or the line number
 * is out of bounds, returns an empty string.
 *
 * @param path The path to the header file
 * @param line_num The line number (0-indexed)
 * @return The line content, or empty string if not found
 */
std::string get_line_content(const std::filesystem::path &path, std::size_t line_num)
{
    const auto it = header_file_lines_cache.find(path);
    if (it != header_file_lines_cache.end() && line_num < it->second.size()) {
        return it->second[line_num];
    }
    return "";
}

/**
 * @brief Constructs a source location string with file path and line number.
 *
 * @details
 * Creates a formatted string in the form "path:line" where:
 * - path is the file path
 * - line is the 1-indexed line number
 *
 * @param format The text formatter to use
 * @param file_path The path to the header file
 * @param line_num The 0-indexed line number
 * @return Formatted source location string
 */
std::string make_source_string(const TextFormatter *format, const std::string &file_path, std::size_t line_num)
{
    return format->format("{}:{}", FormatParam{file_path}, FormatParam{line_num + 1});
}

/**
 * @brief Constructs source details showing contextual lines around the target line.
 *
 * @details
 * Creates a vector of strings showing a contextual view of the header file around the target line. Uses
 * FileHighlightPrinter for consistent formatting with arrow prefix and styling for highlighted lines.
 *
 * For example, with window_size=5 and target line 10:
 * ```
 *       8:   #define SOME_OTHER 100
 *       9:   #define ANOTHER 200
 * >    10:   #define TARGET_VALUE 512abc
 *      11:   #define YET_ANOTHER 300
 *      12:   // comment
 * ```
 *
 * @param format The text formatter to use
 * @param file_path The path to the header file
 * @param line_num The 0-indexed target line number
 * @return Vector of formatted strings showing the contextual view
 */
std::vector<std::string>
make_source_details(const TextFormatter *format, const std::string &file_path, std::size_t line_num)
{
    const std::filesystem::path path{file_path};
    const auto it = header_file_lines_cache.find(path);
    if (it == header_file_lines_cache.end()) {
        return {};
    }

    const auto &lines = it->second;

    if (lines.empty() || line_num >= lines.size()) {
        return {};
    }

    // Use FileHighlightPrinter (line_num is already 0-indexed)
    const FileHighlightPrinter printer{format};
    return printer.print(lines, std::vector<std::size_t>{line_num});
}

/**
 * @brief Loads a header file and caches its lines.
 *
 * @details
 * Reads the header file line-by-line into the cache if not already cached. Returns true if the file exists and was
 * loaded (or was already cached), false if the file doesn't exist.
 *
 * @param path The path to the header file
 * @return true if file is available in cache, false if file doesn't exist
 */
bool load_header_file(const std::filesystem::path &path)
{
    // Check cache first
    if (header_file_lines_cache.find(path) != header_file_lines_cache.end()) {
        return true;
    }

    // File doesn't exist
    if (!std::filesystem::exists(path)) {
        return false;
    }

    // Load file line-by-line
    std::ifstream file{path};
    if (!file.is_open()) {
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    header_file_lines_cache[path] = std::move(lines);

    return true;
}

/**
 * @brief Searches for a #define with the given name in a cached header file.
 *
 * @details
 * Searches through the cached file lines for a #define directive with the specified name. The regex pattern matches:
 * - Optional leading whitespace
 * - #define keyword
 * - Required whitespace
 * - The exact define name
 * - Required whitespace
 * - A sequence of digits (the value)
 *
 * Only simple integer literals are matched (no parentheses, no expressions).
 *
 * @param path The path to the header file (must already be cached)
 * @param define_name The name of the #define to search for
 * @return DefineSearchResult if found, std::nullopt if not found
 */
std::optional<DefineSearchResult> find_define(const std::filesystem::path &path, const std::string &define_name)
{
    const auto it = header_file_lines_cache.find(path);
    if (it == header_file_lines_cache.end()) {
        return std::nullopt;
    }

    // Build regex pattern for: #define NAME <value>
    // Captures any non-whitespace token as group 1 (validation happens in the parser)
    const std::string pattern_str = R"(^\s*#\s*define\s+)" + define_name + R"(\s+(\S+))";
    const std::regex pattern{pattern_str};

    const auto &lines = it->second;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        const auto &line = lines[i];
        if (std::regex_search(line, match, pattern)) {
            return DefineSearchResult{i, match[1].str()};
        }
    }

    return std::nullopt;
}

/**
 * @brief Parses a #define value as std::size_t.
 *
 * @details
 * Attempts to parse the value string from a DefineSearchResult as a std::size_t. Returns a valid LayerValue if parsing
 * succeeds, or an invalid LayerValue with error details if parsing fails.
 *
 * @param format The text formatter to use
 * @param result The DefineSearchResult containing the value to parse
 * @param key The configuration key name (for error messages)
 * @param define_name The configuration define name (for error messages)
 * @param file_path The header file path (for source info)
 * @return LayerValue containing the parsed value or error information
 */
LayerValue<std::size_t> parse_size_t(
    const TextFormatter *format,
    const DefineSearchResult &result,
    const std::string &key,
    const std::string &define_name,
    const std::string &file_path)
{
    try {
        const auto value = std::stoull(result.value_string);
        const auto source = make_source_string(format, file_path, result.line_number);
        const auto details = make_source_details(format, file_path, result.line_number);
        return LayerValue<std::size_t>::valid(value, source, details);
    }
    catch (const std::exception &e) {
        const auto error = format->format(
            "failed to parse '{}' as std::size_t: {}", FormatParam{define_name, Style::bold}, FormatParam{e.what()});
        const auto source = make_source_string(format, file_path, result.line_number);
        const auto details = make_source_details(format, file_path, result.line_number);
        return LayerValue<std::size_t>::invalid(error, source, details);
    }
}

/**
 * @brief Searches a header file for a #define and parses its value.
 *
 * @details
 * This is the main entry point for searching header defines. It:
 * 1. Loads the header file (if not already cached)
 * 2. Searches for the specified #define
 * 3. Parses the value using the provided parser
 *
 * Returns not_provided() if:
 * - The file doesn't exist
 * - The #define is not found in the file
 *
 * Returns invalid() if:
 * - The #define is found but the value cannot be parsed
 *
 * @tparam T The type to parse the value as
 * @tparam ParseFunc Function type for parsing (format, result, key, path -> LayerValue<T>)
 * @param format The text formatter to use
 * @param header_path The path to the header file
 * @param define_name The name of the #define to search for
 * @param parse_func The function to parse the define value
 * @param key The configuration key name (for error messages)
 * @return LayerValue with the parsed value, error, or not_provided status
 */
template <typename T, typename ParseFunc>
LayerValue<T> search_header_define(
    const TextFormatter *format,
    const std::filesystem::path &header_path,
    const std::string &define_name,
    ParseFunc parse_func,
    const std::string &key)
{
    // Try to load the header file
    if (!load_header_file(header_path)) {
        return LayerValue<T>::not_provided();
    }

    // Search for the define
    const auto result = find_define(header_path, define_name);
    if (!result.has_value()) {
        return LayerValue<T>::not_provided();
    }

    // Parse the value
    return parse_func(format, result.value(), key, define_name, header_path.string());
}

} // namespace
