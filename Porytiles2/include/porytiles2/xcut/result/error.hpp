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

/*
 Based on the code analysis, here are suggested names for BasicError that better communicate its intent as a
convenient, general-purpose error type for when specialized errors would be overkill:

  Top Recommendations

  1. FormattedError - Emphasizes the key feature of parameter formatting with {} placeholders
  2. MessageError - Clear that it's for simple message-based errors
  3. StringError - Specific about handling string-based error messages
  4. AdHocError - Captures the "when specialized would be overkill" use case

  Other Strong Options

  5. TextError - Simple and clear about text-based errors
  6. QuickError - Emphasizes convenience and speed of use
  7. GenericError - Communicates general-purpose nature
  8. SimpleError - Direct but still somewhat generic
  9. ParameterizedError - Emphasizes the parameter substitution capability
  10. ConvenienceError - Emphasizes ease of use

  My Top Pick

  FormattedError is my recommendation because:
  - It highlights the main technical differentiator (parameter formatting)
  - It's descriptive without being verbose
  - It clearly distinguishes it from truly "basic" errors
  - It tells users what they're getting - an error with formatting capabilities

  The name FormattedError accurately reflects that this class is specifically designed for errors that need parameter
substitution and conditional TTY formatting, which is its key value proposition over truly basic string errors.
 */

class BasicError final : public Error {
  public:
    explicit BasicError(std::string text) : text_{std::move(text)} {}

    explicit BasicError(std::string text, std::vector<FormatParam> params)
        : text_{std::move(text)}, params_{std::move(params)}
    {
    }

    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        if (params_.empty()) {
            return text_;
        }

        return formatter.format(text_, params_);
    }

    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<BasicError>(text_, params_);
    }

  private:
    std::string text_;
    std::vector<FormatParam> params_;
};

} // namespace porytiles2
