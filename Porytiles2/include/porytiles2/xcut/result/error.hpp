#pragma once

#include <memory>
#include <string>
#include <vector>

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for all error types used in ChainableResult error chains.
 *
 * @details
 * The Error interface defines the contract that all error types must implement to participate in ChainableResult error
 * chains. This interface enables polymorphic error handling while maintaining type safety and proper ownership
 * semantics through the clone pattern.
 *
 * Error implementations should be immutable value types that capture all relevant context about a failure at a specific
 * point in the application. The details() method allows errors to format their messages based on the output context
 * (TTY vs non-TTY), while the clone() method enables proper copying of errors when building error chains.
 *
 * All concrete error types used with ChainableResult must derive from this interface. This requirement is enforced at
 * compile time through static_assert in ChainableResult's constructors.
 */
class Error {
  public:
    virtual ~Error() = default;

    /**
     * @brief Returns a formatted multi-line string representation of the error.
     *
     * @details
     * This method generates a human-readable description of the error, potentially including ANSI formatting codes if
     * the provided TextFormatter indicates TTY output is enabled. Implementations should provide clear, actionable
     * error messages that help users understand what went wrong and potentially how to fix it.
     *
     * The return value is a vector of strings, where each element represents one line of the error message. Single-line
     * errors return a vector with one element, while multi-line errors can return multiple lines for richer
     * diagnostics.
     *
     * @param formatter The TextFormatter to use for conditional formatting based on TTY status
     * @return A vector of formatted strings describing the error, with each element representing one line
     */
    [[nodiscard]] virtual std::vector<std::string> details(const TextFormatter &formatter) const = 0;

    /**
     * @brief Creates a polymorphic copy of this error.
     *
     * @details
     * The clone pattern is necessary because ChainableResult stores errors as unique_ptr<Error>, and errors need to be
     * copied when building error chains from const references. Each concrete error type must implement this method to
     * return a new instance with the same state.
     *
     * @return A unique_ptr to a newly allocated copy of this error
     */
    [[nodiscard]] virtual std::unique_ptr<Error> clone() const = 0;
};

/**
 * @brief General-purpose error implementation with formatted message support.
 *
 * @details
 * FormattableError is a concrete Error implementation designed for common error scenarios where creating a specialized
 * error type would be unnecessary overhead. It supports both simple string messages and formatted messages with styled
 * parameters using TextFormatter and FormatParam.
 *
 * Key features:
 * - Simple construction with a plain string message
 * - Format string support with styled parameter substitution using fmtlib syntax
 * - Automatic TTY-aware styling through TextFormatter integration
 * - Suitable for ad-hoc error reporting without defining custom error types
 *
 * Example usage:
 * ```C++
 * // Simple string error
 * return FormattableError{"file not found"};
 *
 * // Formatted error with styled parameters
 * return FormattableError{"{}: tileset '{}' does not exist",
 *     FormatParam{"error", Style::red | Style::bold}, FormatParam{name, Style::bold}};
 * ```
 *
 * When to use FormattableError vs specialized error types:
 * - Use FormattableError for straightforward error messages that don't require custom behavior
 * - Use specialized Error subclasses when errors need additional context, state, or special formatting logic
 */
class FormattableError final : public Error {
  public:
    /**
     * @brief Constructs an empty FormattableError with no message.
     *
     * @details
     * Creates a FormattableError with no text content. This is primarily used for error chain passthrough scenarios
     * where the current layer doesn't need to add additional error context. Empty FormattableErrors can be detected
     * using the has_details() method and are typically filtered out during error chain visualization.
     */
    FormattableError() = default;

    /**
     * @brief Constructs a FormattableError with a plain text message.
     *
     * @details
     * Creates a FormattableError containing a simple string message with no parameter formatting. This constructor
     * is used for straightforward error messages that don't require styled parameters. The message is stored as a
     * single-line error.
     *
     * @param text The error message text
     */
    explicit FormattableError(std::string text)
    {
        if (!text.empty()) {
            text_.push_back(std::move(text));
        }
    }

    /**
     * @brief Constructs a FormattableError with a format string and styled parameters.
     *
     * @details
     * Creates a FormattableError that uses fmtlib-style formatting to substitute styled parameters into the message.
     * The text parameter should contain `{}` placeholders that will be replaced with the styled text from the params
     * vector when details() is called. The message is stored as a single-line error.
     *
     * @param text The format string with `{}` placeholders
     * @param params Vector of FormatParams to substitute into the format string
     */
    explicit FormattableError(std::string text, std::vector<FormatParam> params)
    {
        if (!text.empty() || !params.empty()) {
            text_.push_back(std::move(text));
            params_.push_back(std::move(params));
        }
    }

    /**
     * @brief Constructs a FormattableError with a format string and variadic styled parameters.
     *
     * @details
     * Convenience constructor that allows passing FormatParams directly as arguments instead of wrapping them in a
     * std::vector. This provides more natural syntax for error construction with a known number of parameters.
     * The message is stored as a single-line error.
     *
     * Example:
     * ```C++
     * FormattableError{"expected {} but got {}", FormatParam{expected, Style::green}, FormatParam{actual, Style::red}}
     * ```
     *
     * @tparam FirstParam Type of the first parameter
     * @tparam RestParams Types of remaining parameters
     * @param text The format string with `{}` placeholders
     * @param first First FormatParam argument
     * @param rest Remaining FormatParam arguments to substitute into the format string
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_same_v<std::decay_t<FirstParam>, FormatParam> &&
            (std::is_same_v<std::decay_t<RestParams>, FormatParam> && ...))
    explicit FormattableError(std::string text, FirstParam &&first, RestParams &&...rest)
    {
        text_.push_back(std::move(text));

        std::vector<FormatParam> line_params;
        line_params.reserve(1 + sizeof...(RestParams));
        line_params.push_back(std::forward<FirstParam>(first));
        (line_params.push_back(std::forward<RestParams>(rest)), ...);
        params_.push_back(std::move(line_params));
    }

    /**
     * @brief Constructs a FormattableError with multiple plain text lines.
     *
     * @details
     * Creates a FormattableError containing multiple lines of text with no parameter formatting. This constructor
     * is used for multi-line error messages that don't require styled parameters.
     *
     * @param lines Vector of error message lines
     */
    explicit FormattableError(std::vector<std::string> lines) : text_{std::move(lines)} {}

    /**
     * @brief Constructs a FormattableError with multiple formatted lines.
     *
     * @details
     * Creates a FormattableError with multiple lines, where each line can have its own styled parameters. The lines
     * vector should contain format strings with `{}` placeholders, and the params vector should contain a
     * corresponding vector of FormatParams for each line.
     *
     * If params is shorter than lines, the extra lines will have no parameters. If params is longer than lines, the
     * extra parameter vectors will be ignored.
     *
     * @param lines Vector of format strings, one per line
     * @param params Vector of parameter vectors, one per line
     */
    explicit FormattableError(std::vector<std::string> lines, std::vector<std::vector<FormatParam>> params)
        : text_{std::move(lines)}, params_{std::move(params)}
    {
    }

    /**
     * @brief Returns the formatted error message lines with appropriate styling.
     *
     * @details
     * Generates the error message lines by formatting each line independently. For lines without parameters, the
     * plain text is returned. For lines with parameters, the text is formatted with styled parameters using the
     * provided TextFormatter. The formatter determines whether to apply ANSI styling codes or return plain text based
     * on the output context.
     *
     * @param formatter The TextFormatter to use for applying styles
     * @return A vector of formatted error message lines with styling applied as appropriate
     */
    [[nodiscard]] std::vector<std::string> details(const TextFormatter &formatter) const override
    {
        std::vector<std::string> result;
        result.reserve(text_.size());

        for (std::size_t i = 0; i < text_.size(); ++i) {
            if (i < params_.size() && !params_[i].empty()) {
                result.push_back(formatter.format(text_[i], params_[i]));
            }
            else {
                result.push_back(text_[i]);
            }
        }

        return result;
    }

    /**
     * @brief Checks whether this FormattableError contains any message content.
     *
     * @details
     * Returns true if the error contains at least one non-empty line, false otherwise. This method is used to
     * distinguish between errors that carry meaningful information and empty errors created for passthrough purposes.
     * Empty errors (created with the default constructor, empty string, or only empty lines) are typically filtered
     * out during error chain visualization in UserDiagnostics::fatal().
     *
     * @return True if the error contains at least one non-empty line, false if the error is empty
     */
    [[nodiscard]] bool has_details() const
    {
        for (const auto &line : text_) {
            if (!line.empty()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Creates a polymorphic copy of this FormattableError.
     *
     * @details
     * Implements the Error clone pattern by creating a new FormattableError with the same text and parameters. This
     * allows FormattableError instances to be copied when building ChainableResult error chains.
     *
     * @return A unique_ptr to a newly allocated copy of this error
     */
    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<FormattableError>(text_, params_);
    }

  private:
    std::vector<std::string> text_;
    std::vector<std::vector<FormatParam>> params_;
};

} // namespace porytiles2
