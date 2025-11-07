#pragma once

#include <ranges>
#include <regex>
#include <string>

#include "fmt/format.h"

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Checks if a string fully matches a regular expression pattern.
 *
 * @details
 * This function compiles the provided pattern into a regex and performs a full
 * string match against the input string. Panics if the pattern is invalid.
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
        panic(fmt::format("regex error: {}", e.what()));
    }
}

/**
 * @brief Removes leading and trailing whitespace from a string in-place.
 *
 * @details
 * This function modifies the input string by removing all whitespace characters
 * from the beginning and end of the string. The string is modified directly.
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
 * This function splits the input string into a vector of substrings using the
 * specified delimiter. The delimiter itself is not included in the resulting tokens.
 * Empty tokens are preserved if consecutive delimiters are found.
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
 * This function removes all trailing carriage return (\r) and newline (\n)
 * characters from the end of the string. The string is modified directly.
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
 * This function creates a copy of the input string and removes all trailing
 * carriage return (\r) and newline (\n) characters from the end. The original
 * string is not modified.
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

} // namespace porytiles2
