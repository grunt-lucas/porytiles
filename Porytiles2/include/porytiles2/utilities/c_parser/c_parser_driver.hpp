#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/c_parser/c_parser_context.hpp"
#include "porytiles2/utilities/c_parser/define_statement.hpp"
#include "porytiles2/utilities/c_parser/enum_declaration.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief High-level driver for parsing C/C++ source files.
 *
 * @details
 * CParserDriver orchestrates the complete parsing pipeline: file loading, lexing, and parsing. It owns the file
 * content and provides rich error formatting through FileHighlightPrinter integration.
 *
 * The driver provides a simple interface for extracting specific constructs from C/C++ source files:
 * - parse_defines() extracts all #define preprocessor directives
 * - parse_enums() extracts all enum declarations
 *
 * Error handling uses ChainableResult with FormattableError, providing multi-line error messages with source
 * code context highlighting showing exactly where errors occurred.
 *
 * Example usage:
 * @code
 * PlainTextFormatter formatter;
 * CParserDriver driver{"include/constants.h", &formatter};
 *
 * auto defines_result = driver.parse_defines();
 * if (!defines_result.has_value()) {
 *     // Error chain contains rich formatted output with source highlighting
 *     for (const auto& err : defines_result.chain()) {
 *         for (const auto& line : err->details(formatter)) {
 *             std::cerr << line << '\n';
 *         }
 *     }
 * }
 * @endcode
 */
class CParserDriver {
  public:
    /**
     * @brief Constructs a driver for parsing the specified file.
     *
     * @details
     * The file is not loaded until a parse method is called. This allows for efficient construction when the driver
     * may not be used, or when multiple drivers are created but only some are actually needed.
     *
     * @param file_path Path to the C/C++ source file to parse
     * @param format Formatter for error message styling (non-owning, must outlive driver)
     */
    CParserDriver(std::filesystem::path file_path, gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Parses all #define statements from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts all #define preprocessor directives. Returns
     * a vector of DefineStatement objects containing the macro names and evaluated values.
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @return A vector of DefineStatement on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse_defines();

    /**
     * @brief Parses all enum declarations from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts all enum declarations. Returns a vector of
     * EnumDeclaration objects containing the enum names and members with their values.
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @return A vector of EnumDeclaration on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<EnumDeclaration>> parse_enums();

    /**
     * @brief Returns the cached file lines.
     *
     * @details
     * Returns a const reference to the file lines loaded during parsing. If the file has not been loaded yet (no parse
     * method called), returns an empty vector.
     *
     * @return Const reference to the file lines vector
     */
    [[nodiscard]] const std::vector<std::string> &file_lines() const;

  private:
    [[nodiscard]] ChainableResult<void> ensure_loaded();

    std::filesystem::path file_path_;
    const TextFormatter *format_;
    std::vector<std::string> file_lines_;
    std::string content_;
    std::unique_ptr<CParserContext> context_;
    bool loaded_{false};
    bool load_failed_{false};
    FormattableError load_error_;
};

} // namespace porytiles2
