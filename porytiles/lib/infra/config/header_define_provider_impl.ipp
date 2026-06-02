#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "porytiles/infra/config/config_provider.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/define_statement.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

// The anonymous namespace ensures internal linkage per translation unit
// This file is intentionally included only in header_define_provider.cpp
namespace {

using namespace porytiles;

/**
 * @brief Constructs a source location string from a DefineStatement's position.
 *
 * @details
 * Creates a formatted string in the form "path:line" where:
 * - path is the file path
 * - line is the 1-indexed line number from the DefineStatement's position
 *
 * @param format The text formatter to use
 * @param file_path The path to the header file
 * @param define The DefineStatement containing position information
 * @return Formatted source location string
 */
std::string make_source_string(const TextFormatter *format, const std::string &file_path, const DefineStatement &define)
{
    return format->format("{}:{}", FormatParam{file_path}, FormatParam{define.position().line});
}

/**
 * @brief Constructs source details showing contextual lines around a DefineStatement.
 *
 * @details
 * Creates a vector of strings showing a contextual view of the header file around the define's location. Uses
 * FileHighlightPrinter for consistent formatting with arrow prefix and styling for highlighted lines.
 *
 * @param format The text formatter to use
 * @param file_lines The cached file lines from CParserFacade
 * @param define The DefineStatement containing position information
 * @return Vector of formatted strings showing the contextual view
 */
std::vector<std::string> make_source_details(
    const TextFormatter *format, const std::vector<std::string> &file_lines, const DefineStatement &define)
{
    if (file_lines.empty()) {
        return {};
    }

    // Convert 1-indexed line to 0-indexed for FileHighlightPrinter
    const std::size_t line_index = define.position().line - 1;
    if (line_index >= file_lines.size()) {
        return {};
    }

    const FileHighlightPrinter printer{format};
    return printer.print(file_lines, std::vector{line_index});
}

/**
 * @brief Gets or creates the CParserFacade for the header file.
 *
 * @details
 * Lazily initializes the parser driver if it doesn't exist yet. Uses std::optional::emplace
 * for in-place construction.
 *
 * @param driver The mutable optional parser driver reference
 * @param header_path The path to the header file
 * @param format The text formatter to use
 * @return Reference to the parser driver
 */
CParserFacade &get_parser_driver(
    std::optional<CParserFacade> &driver, const std::filesystem::path &header_path, const TextFormatter *format)
{
    if (!driver.has_value()) {
        driver.emplace(header_path, format);
    }
    return driver.value();
}

/**
 * @brief Parses a DefineStatement value as std::size_t.
 *
 * @details
 * Converts the integer value from a DefineStatement to std::size_t. Returns a valid LayerValue if the define
 * has an integer value and it's non-negative, or an invalid LayerValue with error details otherwise.
 *
 * @param format The text formatter to use
 * @param define The DefineStatement containing the value to convert
 * @param file_path The header file path (for source info)
 * @param file_lines The cached file lines (for source details)
 * @param key The configuration key name (unused, for API compatibility)
 * @return LayerValue containing the parsed value or error information
 */
LayerValue<std::size_t> parse_size_t(
    const TextFormatter *format,
    const DefineStatement &define,
    const std::string &file_path,
    const std::vector<std::string> &file_lines,
    const std::string & /*key*/)
{
    const auto source = make_source_string(format, file_path, define);
    const auto details = make_source_details(format, file_lines, define);

    if (!define.has_int_value()) {
        const auto error = format->format(
            "'{}' is not an integer value (found {} define)",
            FormatParam{define.name(), Style::bold},
            FormatParam{define.is_flag() ? "flag" : "string"});
        return LayerValue<std::size_t>::invalid(error, source, details);
    }

    const auto value = define.int_value();
    if (value < 0) {
        const auto error =
            format->format("'{}' has negative value {}", FormatParam{define.name(), Style::bold}, FormatParam{value});
        return LayerValue<std::size_t>::invalid(error, source, details);
    }

    return LayerValue<std::size_t>::valid(value, define.name(), source, details);
}

/**
 * @brief Searches a header file for a #define and parses its value using CParserFacade.
 *
 * @details
 * This is the main entry point for searching header defines. It:
 * 1. Gets or creates the CParserFacade for the header file
 * 2. Searches for the specified #define using find_define()
 * 3. Parses the value using the provided parser
 *
 * Returns not_provided() if:
 * - The file doesn't exist
 * - The #define is not found in the file
 *
 * Returns invalid() if:
 * - The #define is found but the value cannot be parsed
 * - There was a parse error in the file
 *
 * @tparam T The type to parse the value as
 * @tparam ParseFunc Function type for parsing (format, define, path, lines, key -> LayerValue<T>)
 * @param driver Mutable reference to the optional CParserFacade
 * @param format The text formatter to use
 * @param header_path The path to the header file
 * @param define_name The name of the #define to search for
 * @param parse_func The function to parse the define value
 * @param key The configuration key name (for error messages)
 * @return LayerValue with the parsed value, error, or not_provided status
 */
template <typename T, typename ParseFunc>
LayerValue<T> search_header_define(
    std::optional<CParserFacade> &driver,
    const TextFormatter *format,
    const std::filesystem::path &header_path,
    const std::string &define_name,
    ParseFunc parse_func,
    const std::string &key,
    const std::string &provider_name)
{
    // Get or create the parser driver
    auto &parser = get_parser_driver(driver, header_path, format);

    // Search for the define
    auto result = parser.find_define(define_name);

    if (!result.has_value()) {
        // Parse error or file not found - check if it's a file-not-found case
        // File not found should return not_provided, parse errors return invalid
        if (!std::filesystem::exists(header_path)) {
            return LayerValue<T>::not_provided();
        }
        // Parse error - return invalid with error message
        const auto base_error_msg = format->format(
            "failed to parse '{}' from '{}':",
            FormatParam{define_name, Style::bold},
            FormatParam{header_path.string(), Style::bold});
        std::vector<std::string> details_err_msg{};
        for (const auto &err : result.chain()) {
            if (const auto &details = err->details(*format); !details.empty()) {
                details_err_msg.append_range(err->details(*format));
                details_err_msg.emplace_back();
            }
        }
        return LayerValue<T>::invalid(base_error_msg, header_path.string(), details_err_msg);
    }

    if (!result.value().has_value()) {
        // Define not found in file
        return LayerValue<T>::not_provided();
    }

    // Parse the value
    auto layer_result = parse_func(format, result.value().value(), header_path.string(), parser.file_lines(), key);
    layer_result.source_key = provider_name;
    return layer_result;
}

} // namespace
