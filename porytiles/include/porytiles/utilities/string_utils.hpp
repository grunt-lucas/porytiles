#pragma once

/// @file string_utils.hpp
///
/// @brief Utility functions for string manipulation and formatting.
///
/// @details
/// This header provides a collection of common string operations used throughout the Porytiles codebase, including:
/// - Whitespace trimming and line ending normalization
/// - String splitting, tokenization, and joining
/// - Case conversion (PascalCase, snake_case, lowercase)
/// - Numeric formatting (hexadecimal, zero-padded digits)
/// - Regular expression matching utilities
///
/// All functions are inline to avoid ODR violations when included in multiple translation units.

#include <algorithm>
#include <cctype>
#include <format>
#include <ranges>
#include <regex>
#include <string>
#include <vector>

#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Checks if a string fully matches a regular expression pattern.
///
/// @details
/// This function compiles the provided pattern into a regex and performs a full string match against the input string.
/// Panics if the pattern is invalid.
///
/// @param str The string to match against the pattern
/// @param pattern The regular expression pattern
/// @pre pattern must be a valid regular expression
/// @return True if the entire string matches the pattern, false otherwise
[[nodiscard]] inline bool check_full_string_match(const std::string &str, const std::string &pattern)
{
    try {
        const std::regex re{pattern};
        return std::regex_match(str, re);
    }
    catch (const std::regex_error &e) {
        panic(std::string{"regex error: "} + std::string{e.what()});
    }
}

/// @brief Removes a prefix from a string if present.
///
/// @details
/// This function checks if the input string starts with the specified prefix. If it does, the function returns a new
/// string with the prefix removed. If the prefix is not present, the original string is returned unchanged.
///
/// @param str The string to potentially trim
/// @param prefix The prefix to remove if present
/// @return A new string with the prefix removed, or the original string if prefix was not present
///
/// @par Examples
/// - `trim_prefix("hello_world", "hello_")` -> `"world"`
/// - `trim_prefix("hello_world", "foo")` -> `"hello_world"`
/// - `trim_prefix("hello", "hello")` -> `""`
/// - `trim_prefix("hello", "hello_world")` -> `"hello"`
[[nodiscard]] inline std::string trim_prefix(const std::string &str, const std::string &prefix)
{
    if (str.starts_with(prefix)) {
        return str.substr(prefix.size());
    }
    return str;
}

/// @brief Removes leading and trailing whitespace from a string in-place.
///
/// @details
/// This function modifies the input string by removing all whitespace characters from the beginning and end of the
/// string. The string is modified directly.
///
/// @param string The string to trim (modified in-place)
inline void trim(std::string &string)
{
    // Trim blank space from the beginning
    string.erase(
        string.begin(), std::ranges::find_if(string, [](const unsigned char ch) { return !std::isspace(ch); }));

    // Trim blank space from the end
    string.erase(
        std::ranges::find_if(string.rbegin(), string.rend(), [](const unsigned char ch) { return !std::isspace(ch); })
            .base(),
        string.end());
}

/// @brief Splits a string into tokens based on a delimiter.
///
/// @details
/// This function splits the input string into a vector of substrings using the specified delimiter. The delimiter
/// itself is not included in the resulting tokens. Empty tokens are preserved if consecutive delimiters are found.
///
/// @param input The string to split
/// @param delimiter The delimiter string to split on
/// @return A vector containing the split tokens
[[nodiscard]] inline std::vector<std::string> split(std::string input, const std::string &delimiter)
{
    std::vector<std::string> result;
    size_t pos;
    while ((pos = input.find(delimiter)) != std::string::npos) {
        std::string token = input.substr(0, pos);
        result.push_back(token);
        input.erase(0, pos + delimiter.length());
    }
    result.push_back(input);
    return result;
}

/// @brief Joins strings into a delimited list, wrapping each element in single quotes.
///
/// @details
/// This function renders a list of names for a user-facing message. The single quotes match the diagnostic style rule
/// that highlightable items (file names, symbol names, config keys) appear quoted, so a joined list reads the same as
/// the individually quoted items elsewhere in the same message. An empty input yields an empty string.
///
/// @param values The strings to quote and join
/// @param delimiter The separator placed between elements
/// @return The joined list, or an empty string when there are no values
///
/// @par Examples
/// - `join_quoted({"a.c", "b.h"})` -> `"'a.c', 'b.h'"`
/// - `join_quoted({"only"})` -> `"'only'"`
/// - `join_quoted({})` -> `""`
[[nodiscard]] inline std::string
join_quoted(const std::vector<std::string> &values, const std::string &delimiter = ", ")
{
    std::string joined;
    for (const auto &value : values) {
        if (!joined.empty()) {
            joined += delimiter;
        }
        joined += "'" + value + "'";
    }
    return joined;
}

/// @brief Removes line ending characters from a string in-place.
///
/// @details
/// This function removes all trailing carriage return (\r) and newline (\n) characters from the end of the string. The
/// string is modified directly.
///
/// @param line The string to trim (modified in-place)
/// @return Reference to the modified string
[[nodiscard]] inline std::string &trim_line_ending(std::string &line)
{
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    return line;
}

/// @brief Removes line ending characters from a string.
///
/// @details
/// This function creates a copy of the input string and removes all trailing carriage return (\r) and newline (\n)
/// characters from the end. The original string is not modified.
///
/// @param line The string to trim
/// @return A new string with line endings removed
[[nodiscard]] inline std::string trim_line_ending(const std::string &line)
{
    std::string result = line;
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    return result;
}

/// @brief Converts an integer value to a hexadecimal string with "0x" prefix.
///
/// @details
/// This function formats the given integer value as a lowercase hexadecimal string prefixed with "0x". For example,
/// `int_to_hex_str(255)` returns `"0xff"`.
///
/// @tparam T An integral type that can be formatted as hexadecimal
/// @param t The integer value to convert
/// @return A string containing the hexadecimal representation with "0x" prefix
template <typename T>
[[nodiscard]] std::string int_to_hex_str(T t)
{
    return std::format("0x{:x}", t);
}

/// @brief Converts an integer value to a minimum two-digit wide string representation.
///
/// @details
/// For example: `pad_two_digits(3)` returns `"03"`, `pad_two_digits(13)` returns `"13"`, `pad_two_digits(133)` returns
/// `"133"`.
///
/// @tparam T An integral type to be formatted
/// @param t The integer value to convert
/// @return A string containing the padded representation
template <typename T>
[[nodiscard]] std::string pad_two_digits(T t)
{
    return std::format("{:02}", t);
}

/// @brief Identity function for to_string with std::string input.
///
/// @details
/// This overload enables generic code that calls to_string() to work with std::string types. It simply returns the
/// input string unchanged. This is needed because std::to_string() only works with numeric types, but generic templates
/// (like config value caching) may need to convert any type to a string representation.
///
/// @param str The string to return
/// @return The input string unchanged
[[nodiscard]] inline std::string to_string(const std::string &str)
{
    return str;
}

/// @brief Converts a boolean to its string representation.
///
/// @details
/// This overload is necessary because std::to_string() has no bool overload and implicitly converts bool to int,
/// resulting in "1" or "0" instead of "true" or "false". This function provides human-readable boolean string
/// representation for use in diagnostic messages and configuration value display.
///
/// @param value The boolean value to convert.
/// @return "true" if value is true, "false" otherwise.
[[nodiscard]] inline std::string to_string(bool value)
{
    return value ? "true" : "false";
}

/// @brief Converts a vector to a string representation with curly brace delimiters.
///
/// @details
/// This function formats a vector as a comma-separated list enclosed in curly braces. For example,
/// `to_string(std::vector<int>{1, 2, 3})` returns `"{1, 2, 3}"`.
///
/// @tparam T The element type of the vector (must be formattable by fmt::format)
/// @param vec The vector to convert to a string
/// @return A string representation of the vector in the format "{elem1, elem2, ...}"
template <typename T>
[[nodiscard]] std::string to_string(const std::vector<T> &vec)
{
    std::string result = "{";
    for (std::size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += to_string(vec[i]);
    }
    result += "}";
    return result;
}

/// @brief Converts a string to PascalCase format.
///
/// @details
/// This function converts an input string to PascalCase by capitalizing the first letter of each word and removing
/// separators. Words are identified by underscore ('_'), hyphen ('-'), or space (' ') delimiters. Characters following
/// a delimiter are capitalized, and the delimiters themselves are removed from the output.
///
/// @param s The string to convert
/// @return A new string in PascalCase format
///
/// @par Examples
/// - `"hello_world"` -> `"HelloWorld"`
/// - `"foo-bar"` -> `"FooBar"`
/// - `"already PascalCase"` -> `"AlreadyPascalCase"`
[[nodiscard]] inline std::string to_pascal_case(const std::string &s)
{
    if (s.empty()) {
        return s;
    }

    std::string result;
    result.reserve(s.size());

    bool capitalize_next = true;
    for (const char c : s) {
        if (c == '_' || c == '-' || c == ' ') {
            capitalize_next = true;
        }
        else {
            if (capitalize_next) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                capitalize_next = false;
            }
            else {
                result += c;
            }
        }
    }

    return result;
}

/// @brief Converts a string to snake_case format.
///
/// @details
/// This function converts an input string to snake_case by inserting underscores before uppercase letters and
/// converting all characters to lowercase. Existing separators (underscore, hyphen, space) are converted to
/// underscores. Consecutive uppercase letters are treated as an acronym, with an underscore inserted before the last
/// letter of the acronym when followed by lowercase letters (e.g., "XMLParser" becomes "xml_parser").
///
/// @param s The string to convert
/// @return A new string in snake_case format
///
/// @par Examples
/// - `"HelloWorld"` -> `"hello_world"`
/// - `"camelCase"` -> `"camel_case"`
/// - `"XMLParser"` -> `"xml_parser"`
/// - `"already_snake"` -> `"already_snake"`
[[nodiscard]] inline std::string to_snake_case(const std::string &s)
{
    if (s.empty()) {
        return s;
    }

    std::string result;
    result.reserve(s.size() + s.size() / 4); // Reserve extra for underscores

    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];

        // Handle separators: convert to underscore
        if (c == '_' || c == '-' || c == ' ') {
            // Avoid leading underscore or consecutive underscores
            if (!result.empty() && result.back() != '_') {
                result += '_';
            }
            continue;
        }

        // Handle uppercase letters
        if (std::isupper(static_cast<unsigned char>(c))) {
            // Insert underscore before uppercase if:
            // 1. Not at the start
            // 2. Previous char wasn't an underscore
            // 3. Either previous char was lowercase, OR next char is lowercase (for acronyms like XMLParser)
            if (!result.empty() && result.back() != '_') {
                const bool prev_is_lower = i > 0 && std::islower(static_cast<unsigned char>(s[i - 1]));
                const bool next_is_lower = i + 1 < s.size() && std::islower(static_cast<unsigned char>(s[i + 1]));
                if (prev_is_lower || next_is_lower) {
                    result += '_';
                }
            }
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    // Remove trailing underscore if present
    if (!result.empty() && result.back() == '_') {
        result.pop_back();
    }

    return result;
}

/// @brief Converts all characters in a string to lowercase.
///
/// @details
/// This function creates a new string where each character from the input is converted to its lowercase equivalent
/// using std::tolower. Characters that are already lowercase or non-alphabetic remain unchanged. The function uses
/// unsigned char casting internally to avoid undefined behavior with negative char values on platforms where char is
/// signed.
///
/// @param input The string to convert to lowercase
/// @return A new string with all alphabetic characters converted to lowercase
///
/// @par Examples
/// - `"Hello World"` -> `"hello world"`
/// - `"UPPERCASE"` -> `"uppercase"`
/// - `"MixedCase123"` -> `"mixedcase123"`
[[nodiscard]] inline std::string to_lower_str(const std::string &input)
{
    std::string output;
    std::ranges::transform(input, std::back_inserter(output), [](const unsigned char c) {
        // Use unsigned char to avoid issues with negative char values
        return std::tolower(c);
    });
    return output;
}

/// @brief Constructs a palette filename from a palette index.
///
/// @details
/// This function formats a palette index as a two-digit padded number with the ".pal" extension. For example,
/// `palette_filename(3)` returns `"03.pal"`, `palette_filename(12)` returns `"12.pal"`.
///
/// @param palette_index The palette index to format
/// @return A string in the format "XX.pal" where XX is the zero-padded index
[[nodiscard]] inline std::string palette_filename(std::size_t palette_index)
{
    return pad_two_digits(palette_index) + ".pal";
}

/// @brief The naming prefix for every tileset variable in a decomp project (e.g. "gTileset_General").
inline constexpr std::string_view tileset_name_prefix = "gTileset_";

/// @brief Extracts the Pascal-case tileset short name from the full name.
///
/// @details
/// Removes the @c tileset_name_prefix from a tileset name if present. This is commonly used when generating animation
/// variable names or parsing animation code, where the short name (e.g., "General") is used instead of the full name
/// (e.g., "gTileset_General").
///
/// @param tileset_name The full tileset name (e.g., "gTileset_General")
/// @return The short name (e.g., "General"), or the original string if prefix not present
///
/// @par Examples
/// - `"gTileset_General"` -> `"General"`
/// - `"gTileset_Petalburg"` -> `"Petalburg"`
/// - `"General"` -> `"General"` (no prefix, unchanged)
[[nodiscard]] inline std::string extract_tileset_shorthand(const std::string &tileset_name)
{
    if (tileset_name.starts_with(tileset_name_prefix)) {
        return tileset_name.substr(tileset_name_prefix.size());
    }
    return tileset_name;
}

/// @brief Extracts the tileset short name from the full name while requiring the @c tileset_name_prefix to be present.
///
/// @details
/// The validating counterpart of @c extract_tileset_shorthand(): callers that must not silently accept an unprefixed
/// name (e.g. code interleaving the shorthand with generated C symbol names) can use this to fail with a uniform
/// diagnostic instead of producing a corrupted shorthand.
///
/// @param tileset_name The full tileset name (e.g., "gTileset_General")
/// @return The short name (e.g., "General"), or an error when the prefix is missing or the shorthand is empty
[[nodiscard]] inline ChainableResult<std::string> require_tileset_shorthand(const std::string &tileset_name)
{
    if (!tileset_name.starts_with(tileset_name_prefix)) {
        return FormattableError{
            "Tileset name '{}' does not start with '{}'.",
            FormatParam{tileset_name, Style::bold},
            FormatParam{std::string{tileset_name_prefix}, Style::bold}};
    }
    if (tileset_name.size() == tileset_name_prefix.size()) {
        return FormattableError{
            "Tileset name '{}' has no characters after the '{}' prefix.",
            FormatParam{tileset_name, Style::bold},
            FormatParam{std::string{tileset_name_prefix}, Style::bold}};
    }
    return tileset_name.substr(tileset_name_prefix.size());
}

/// @brief Extracts the tileset short name and wraps it in a DynamicCasedName.
///
/// @details
/// Removes the @c tileset_name_prefix from a tileset name if present, then constructs a DynamicCasedName from the
/// resulting shorthand. This combines @c extract_tileset_shorthand() and @c DynamicCasedName construction into a
/// single convenience function, which is the most common usage pattern across the codebase.
///
/// @param tileset_name The full tileset name (e.g., "gTileset_General")
/// @return A DynamicCasedName wrapping the short name (e.g., DynamicCasedName{"General"})
///
/// @par Examples
/// - `"gTileset_General"` -> `DynamicCasedName{"General"}`
/// - `"gTileset_Petalburg"` -> `DynamicCasedName{"Petalburg"}`
/// - `"General"` -> `DynamicCasedName{"General"}` (no prefix, unchanged)
[[nodiscard]] inline DynamicCasedName extract_tileset_cased_name(const std::string &tileset_name)
{
    return DynamicCasedName{extract_tileset_shorthand(tileset_name)};
}

} // namespace porytiles
