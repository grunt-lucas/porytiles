#pragma once

#include <memory>
#include <string>
#include <vector>

#include "fmt/args.h"
#include "fmt/format.h"

#include "porytiles2/templates/text_formatter.hpp"

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
 * @brief A simple, concrete error implementation for basic error messages.
 *
 * @details
 * BasicError provides a straightforward implementation of the Error interface for representing simple error messages
 * with optional parameterized formatting. It supports both plain text errors and formatted errors where parameters
 * can be substituted into placeholders in the error message using fmt-style formatting (e.g., "{}" placeholders).
 * When formatted for TTY output, parameters are automatically bolded for emphasis.
 */
class BasicError final : public Error {
  public:
    /**
     * @brief Constructs a BasicError with a plain text message.
     *
     * @param text The error message text
     */
    explicit BasicError(std::string text) : text_{std::move(text)} {}

    /**
     * @brief Constructs a BasicError with a parameterized message.
     *
     * @details
     * Creates an error with a format string and parameters. The text should contain "{}" placeholders that will be
     * replaced with the corresponding parameters when details() is called.
     *
     * @param text The error message format string with "{}" placeholders
     * @param params Vector of parameter values to substitute into the format string
     */
    explicit BasicError(std::string text, std::vector<std::string> params)
        : text_{std::move(text)}, params_{std::move(params)}
    {
    }

    /**
     * @brief Returns the formatted error message.
     *
     * @details
     * If the error has no parameters, returns the plain text message. If parameters are present, substitutes them into
     * the format string's placeholders. When TTY formatting is enabled, parameters are rendered in bold for emphasis.
     *
     * @param formatter The TextFormatter for conditional TTY formatting
     * @return The formatted error message with parameters substituted and optionally bolded
     */
    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        if (params_.empty()) {
            return text_;
        }

        // Create a dynamic format argument store
        fmt::dynamic_format_arg_store<fmt::format_context> store;

        // Add each parameter as a bolded string to the store
        for (const auto &param : params_) {
            store.push_back(fmt::format(formatter.bold(), "{}", param));
        }
        return fmt::vformat(text_, store);
    }

    /**
     * @brief Creates a copy of this BasicError.
     *
     * @return A unique_ptr to a new BasicError with the same message
     */
    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<BasicError>(text_, params_);
    }

  private:
    std::string text_;
    std::vector<std::string> params_;
};

} // namespace porytiles2
