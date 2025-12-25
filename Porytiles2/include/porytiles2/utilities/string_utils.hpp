#pragma once

/**
 * @file string_utils.hpp
 *
 * @brief Utility functions for string manipulation and formatting.
 *
 * @details
 * This header provides a collection of common string operations used throughout the Porytiles codebase, including:
 * - Whitespace trimming and line ending normalization
 * - String splitting and tokenization
 * - Case conversion (PascalCase, snake_case, lowercase)
 * - Numeric formatting (hexadecimal, zero-padded digits)
 * - Regular expression matching utilities
 *
 * All functions are inline to avoid ODR violations when included in multiple translation units.
 */

#include <cctype>
#include <format>
#include <ranges>
#include <regex>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Checks if a string fully matches a regular expression pattern.
 *
 * @details
 * This function compiles the provided pattern into a regex and performs a full string match against the input string.
 * Panics if the pattern is invalid.
 *
 * @param str The string to match against the pattern
 * @param pattern The regular expression pattern
 * @pre pattern must be a valid regular expression
 * @return True if the entire string matches the pattern, false otherwise
 */
inline bool check_full_string_match(const std::string &str, const std::string &pattern)
{
    try {
        const std::regex re{pattern};
        return std::regex_match(str, re);
    }
    catch (const std::regex_error &e) {
        panic(std::string{"regex error: "} + std::string{e.what()});
    }
}

/**
 * @brief Removes leading and trailing whitespace from a string in-place.
 *
 * @details
 * This function modifies the input string by removing all whitespace characters from the beginning and end of the
 * string. The string is modified directly.
 *
 * @param string The string to trim (modified in-place)
 */
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

/**
 * @brief Splits a string into tokens based on a delimiter.
 *
 * @details
 * This function splits the input string into a vector of substrings using the specified delimiter. The delimiter itself
 * is not included in the resulting tokens. Empty tokens are preserved if consecutive delimiters are found.
 *
 * @param input The string to split
 * @param delimiter The delimiter string to split on
 * @return A vector containing the split tokens
 */
inline std::vector<std::string> split(std::string input, const std::string &delimiter)
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

/**
 * @brief Removes line ending characters from a string in-place.
 *
 * @details
 * This function removes all trailing carriage return (\r) and newline (\n) characters from the end of the string. The
 * string is modified directly.
 *
 * @param line The string to trim (modified in-place)
 * @return Reference to the modified string
 */
inline std::string &trim_line_ending(std::string &line)
{
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    return line;
}

/**
 * @brief Removes line ending characters from a string.
 *
 * @details
 * This function creates a copy of the input string and removes all trailing carriage return (\r) and newline (\n)
 * characters from the end. The original string is not modified.
 *
 * @param line The string to trim
 * @return A new string with line endings removed
 */
inline std::string trim_line_ending(const std::string &line)
{
    std::string result = line;
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    return result;
}

/**
 * @brief Converts an integer value to a hexadecimal string with "0x" prefix.
 *
 * @details
 * This function formats the given integer value as a lowercase hexadecimal string prefixed with "0x". For example,
 * `int_to_hex_str(255)` returns `"0xff"`.
 *
 * @tparam T An integral type that can be formatted as hexadecimal
 * @param t The integer value to convert
 * @return A string containing the hexadecimal representation with "0x" prefix
 */
template <typename T>
std::string int_to_hex_str(T t)
{
    return std::format("0x{:x}", t);
}

/**
 * @brief Converts an integer value to a minimum two-digit wide string representation.
 *
 * @details
 * For example: `pad_two_digits(3)` returns `"03"`, `pad_two_digits(13)` returns `"13"`, `pad_two_digits(133)` returns
 * `"133"`.
 *
 * @tparam T An integral type to be formatted
 * @param t The integer value to convert
 * @return A string containing the padded representation
 */
template <typename T>
std::string pad_two_digits(T t)
{
    return std::format("{:02}", t);
}

/**
 * @brief Constructs a palette filename from a palette index.
 *
 * @details
 * This function formats a palette index as a two-digit padded number with the ".pal" extension. For example,
 * `pal_filename(3)` returns `"03.pal"`, `pal_filename(12)` returns `"12.pal"`.
 *
 * @param pal_index The palette index to format
 * @return A string in the format "XX.pal" where XX is the zero-padded index
 */
inline std::string pal_filename(std::size_t pal_index)
{
    return pad_two_digits(pal_index) + ".pal";
}

/**
 * @brief Converts a vector to a string representation with curly brace delimiters.
 *
 * @details
 * This function formats a vector as a comma-separated list enclosed in curly braces. For example,
 * `to_string(std::vector<int>{1, 2, 3})` returns `"{1, 2, 3}"`.
 *
 * @tparam T The element type of the vector (must be formattable by fmt::format)
 * @param vec The vector to convert to a string
 * @return A string representation of the vector in the format "{elem1, elem2, ...}"
 */
template <typename T>
std::string to_string(const std::vector<T> &vec)
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

/**
 * @brief Converts a string to PascalCase format.
 *
 * @details
 * This function converts an input string to PascalCase by capitalizing the first letter of each word and removing
 * separators. Words are identified by underscore ('_'), hyphen ('-'), or space (' ') delimiters. Characters following
 * a delimiter are capitalized, and the delimiters themselves are removed from the output.
 *
 * @param s The string to convert
 * @return A new string in PascalCase format
 *
 * @par Examples
 * - `"hello_world"` → `"HelloWorld"`
 * - `"foo-bar"` → `"FooBar"`
 * - `"already PascalCase"` → `"AlreadyPascalCase"`
 */
inline std::string to_pascal_case(const std::string &s)
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

/**
 * @brief Converts a string to snake_case format.
 *
 * @details
 * This function converts an input string to snake_case by inserting underscores before uppercase letters and
 * converting all characters to lowercase. Existing separators (underscore, hyphen, space) are converted to
 * underscores. Consecutive uppercase letters are treated as an acronym, with an underscore inserted before the last
 * letter of the acronym when followed by lowercase letters (e.g., "XMLParser" becomes "xml_parser").
 *
 * @param s The string to convert
 * @return A new string in snake_case format
 *
 * @par Examples
 * - `"HelloWorld"` → `"hello_world"`
 * - `"camelCase"` → `"camel_case"`
 * - `"XMLParser"` → `"xml_parser"`
 * - `"already_snake"` → `"already_snake"`
 */
inline std::string to_snake_case(const std::string &s)
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

/**
 * @brief Converts all characters in a string to lowercase.
 *
 * @details
 * This function creates a new string where each character from the input is converted to its lowercase equivalent using
 * std::tolower. Characters that are already lowercase or non-alphabetic remain unchanged. The function uses unsigned
 * char casting internally to avoid undefined behavior with negative char values on platforms where char is signed.
 *
 * @param input The string to convert to lowercase
 * @return A new string with all alphabetic characters converted to lowercase
 *
 * @par Examples
 * - `"Hello World"` → `"hello world"`
 * - `"UPPERCASE"` → `"uppercase"`
 * - `"MixedCase123"` → `"mixedcase123"`
 */
inline std::string to_lower_str(const std::string &input)
{
    std::string output;
    std::ranges::transform(input, std::back_inserter(output), [](const unsigned char c) {
        // Use unsigned char to avoid issues with negative char values
        return std::tolower(c);
    });
    return output;
}

} // namespace porytiles2
