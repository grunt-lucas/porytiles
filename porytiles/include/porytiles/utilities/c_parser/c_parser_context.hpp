#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/result/error.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/**
 * @brief Context object providing rich error formatting for C/C++ parsing.
 *
 * @details
 * CParserContext holds the dependencies needed to create FormattableError messages with source file context. It
 * provides a make_error() factory method that creates multi-line error messages including the relevant source code
 * snippet with highlighted error location via FileHighlightPrinter.
 *
 * The context does not own the file_lines - callers must ensure the file_lines outlive the context. This design allows
 * CParserFacade to own the file content while passing non-owning access to Lexer and Parser.
 *
 * Example usage:
 * @code
 * std::vector<std::string> file_lines = {"#define FOO 1", "#define BAR UNDEFINED"};
 * PlainTextFormatter formatter{};
 * CParserContext context{&file_lines, &formatter, "header.h"};
 *
 * // Create error at line 2, column 13 (UNDEFINED token)
 * auto error = context.make_error({2, 13}, "unknown identifier 'UNDEFINED'");
 * // error.details() produces:
 * //   header.h:2:13: unknown identifier 'UNDEFINED'
 * //      1:   #define FOO 1
 * //   ➞  2:   #define BAR UNDEFINED
 * //                       ^
 * @endcode
 */
class CParserContext {
  public:
    /**
     * @brief Constructs a parser context with formatting dependencies.
     *
     * @param file_lines Pointer to cached file lines (non-owning, must outlive context)
     * @param format Text formatter for styled output
     * @param file_path Optional file path for error message headers (empty if unknown)
     * @pre file_lines must not be nullptr
     */
    CParserContext(
        gsl::not_null<const std::vector<std::string> *> file_lines,
        gsl::not_null<const TextFormatter *> format,
        std::string file_path = "");

    /**
     * @brief Creates a FormattableError with source context.
     *
     * @details
     * Creates a multi-line FormattableError including:
     * - Header line: "file:line:col: message" (or "line:col: message" if no file path)
     * - Source context from FileHighlightPrinter showing surrounding lines
     * - Highlighted error line with arrow prefix and styled content
     * - Caret indicator pointing to the exact column
     *
     * If the position is out of bounds (line beyond file, column beyond line), the error is created without source
     * context, showing only the header line with position information.
     *
     * @param pos Source position (1-based line and column)
     * @param message Error message describing the problem
     * @return FormattableError with rich source context
     */
    [[nodiscard]] FormattableError make_error(SourcePosition pos, const std::string &message) const;

    /**
     * @brief Returns the file lines held by this context.
     *
     * @return Pointer to the file lines vector
     */
    [[nodiscard]] const std::vector<std::string> *file_lines() const;

    /**
     * @brief Returns the text formatter used by this context.
     *
     * @return Pointer to the text formatter
     */
    [[nodiscard]] const TextFormatter *formatter() const;

    /**
     * @brief Returns the file path associated with this context.
     *
     * @return The file path string (empty if no path was provided)
     */
    [[nodiscard]] const std::string &file_path() const;

  private:
    const std::vector<std::string> *file_lines_;
    const TextFormatter *format_;
    std::string file_path_;
};

} // namespace porytiles
