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
     * @brief Returns a formatted string representation of the error.
     *
     * @details
     * This method generates a human-readable description of the error, potentially including ANSI formatting codes if
     * the provided TextFormatter indicates TTY output is enabled. Implementations should provide clear, actionable
     * error messages that help users understand what went wrong and potentially how to fix it.
     *
     * @param formatter The TextFormatter to use for conditional formatting based on TTY status
     * @return A formatted string describing the error
     */
    [[nodiscard]] virtual std::string details(const TextFormatter &formatter) const = 0;

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
 * return FormattableError{"tileset '{}' does not exist", FormatParam{name, Style::bold}};
 * ```
 *
 * When to use FormattableError vs specialized error types:
 * - Use FormattableError for straightforward error messages that don't require custom behavior
 * - Use specialized Error subclasses when errors need additional context, state, or special formatting logic
 */
class FormattableError final : public Error {
  public:
    /**
     * @brief Constructs a FormattableError with a plain text message.
     *
     * @details
     * Creates a FormattableError containing a simple string message with no parameter formatting. This constructor is
     * used for straightforward error messages that don't require styled parameters.
     *
     * @param text The error message text
     */
    explicit FormattableError(std::string text) : text_{std::move(text)} {}

    /**
     * @brief Constructs a FormattableError with a format string and styled parameters.
     *
     * @details
     * Creates a FormattableError that uses fmtlib-style formatting to substitute styled parameters into the message.
     * The text parameter should contain `{}` placeholders that will be replaced with the styled text from the params
     * vector when details() is called.
     *
     * Example:
     * ```C++
     * FormattableError{"file '{}' not found", FormatParam{filename, Style::bold}}
     * ```
     *
     * @param text The format string with `{}` placeholders
     * @param params Vector of FormatParams to substitute into the format string
     */
    explicit FormattableError(std::string text, std::vector<FormatParam> params)
        : text_{std::move(text)}, params_{std::move(params)}
    {
    }

    /**
     * @brief Constructs a FormattableError with a format string and variadic styled parameters.
     *
     * @details
     * Convenience constructor that allows passing FormatParams directly as arguments instead of wrapping them in a
     * std::vector. This provides more natural syntax for error construction with a known number of parameters.
     *
     * Example:
     * ```C++
     * FormattableError{"file '{}' not found", FormatParam{filename, Style::bold}}
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
    explicit FormattableError(std::string text, FirstParam &&first, RestParams &&...rest) : text_{std::move(text)}
    {
        params_.reserve(1 + sizeof...(RestParams));
        params_.push_back(std::forward<FirstParam>(first));
        (params_.push_back(std::forward<RestParams>(rest)), ...);
    }

    /**
     * @brief Returns the formatted error message with appropriate styling.
     *
     * @details
     * Generates the error message by either returning the plain text (if no parameters were provided) or formatting
     * the text with styled parameters using the provided TextFormatter. The formatter determines whether to apply
     * ANSI styling codes or return plain text based on the output context.
     *
     * @param formatter The TextFormatter to use for applying styles
     * @return The formatted error message with styling applied as appropriate
     */
    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        if (params_.empty()) {
            return text_;
        }

        return formatter.format(text_, params_);
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
    std::string text_;
    std::vector<FormatParam> params_;
};

} // namespace porytiles2
