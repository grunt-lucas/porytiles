#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/result/error.hpp"

namespace porytiles2 {

/**
 * @brief Error type for C parser failures with source position information.
 *
 * @details
 * CParserError captures both an error message and the source position where the error occurred, enabling callers to
 * programmatically access the location for rich diagnostics (e.g., highlighting the error in the source file using
 * FileHighlightPrinter).
 *
 * This error type is used by both Lexer and Parser to report syntax errors, invalid literals, and expression
 * evaluation failures. The structured position data allows callers to display context around the error location.
 *
 * Example usage:
 * @code
 * return CParserError{current_position(), "unterminated block comment"};
 * @endcode
 */
class CParserError final : public Error {
  public:
    /**
     * @brief Constructs an empty CParserError with no message.
     *
     * @details
     * Creates a CParserError with a default position and no text content. This is primarily used for error chain
     * passthrough scenarios where the current layer doesn't need to add additional error context.
     */
    CParserError() = default;

    /**
     * @brief Constructs a CParserError with position and message.
     *
     * @param position The source position where the error occurred
     * @param message A description of the error
     */
    CParserError(SourcePosition position, std::string message) : position_{position}, message_{std::move(message)} {}

    /**
     * @brief Returns the source position of the error.
     *
     * @return The source position where the error occurred
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

    /**
     * @brief Returns the error message.
     *
     * @return The error message text
     */
    [[nodiscard]] const std::string &message() const
    {
        return message_;
    }

    /**
     * @brief Returns the formatted error message with position prefix.
     *
     * @details
     * Formats the error as "line X, column Y: <message>" for display. The TextFormatter parameter is accepted for
     * interface compatibility but is not currently used since CParserError uses plain string messages.
     *
     * @param formatter The TextFormatter (unused, for interface compatibility)
     * @return A vector containing a single formatted error line
     */
    [[nodiscard]] std::vector<std::string> details(const TextFormatter &formatter) const override;

    /**
     * @brief Creates a polymorphic copy of this error.
     *
     * @return A unique_ptr to a newly allocated copy of this error
     */
    [[nodiscard]] std::unique_ptr<Error> clone() const override;

  private:
    SourcePosition position_;
    std::string message_;
};

} // namespace porytiles2
