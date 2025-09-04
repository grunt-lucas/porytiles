#pragma once

#include <memory>
#include <string>

#include "porytiles2/templates/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for all error types used in TraceableResult error chains.
 *
 * @details
 * The Error interface defines the contract that all error types must implement to participate
 * in TraceableResult error traces. This interface enables polymorphic error handling while
 * maintaining type safety and proper ownership semantics through the clone pattern.
 *
 * Error implementations should be immutable value types that capture all relevant context
 * about a failure at a specific point in the application. The details() method allows errors
 * to format their messages based on the output context (TTY vs non-TTY), while the clone()
 * method enables proper copying of errors when building error traces.
 *
 * All concrete error types used with TraceableResult must derive from this interface. This
 * requirement is enforced at compile time through static_assert in TraceableResult's constructors.
 */
class Error {
  public:
    virtual ~Error() = default;

    /**
     * @brief Returns a formatted string representation of the error.
     *
     * @details
     * This method generates a human-readable description of the error, potentially including
     * ANSI formatting codes if the provided TextFormatter indicates TTY output is enabled.
     * Implementations should provide clear, actionable error messages that help users
     * understand what went wrong and potentially how to fix it.
     *
     * @param formatter The TextFormatter to use for conditional formatting based on TTY status
     * @return A formatted string describing the error
     */
    [[nodiscard]] virtual std::string details(const TextFormatter &formatter) const = 0;

    /**
     * @brief Creates a polymorphic copy of this error.
     *
     * @details
     * The clone pattern is necessary because TraceableResult stores errors as unique_ptr<Error>,
     * and errors need to be copied when building error traces from const references. Each concrete
     * error type must implement this method to return a new instance with the same state.
     *
     * @return A unique_ptr to a newly allocated copy of this error
     */
    [[nodiscard]] virtual std::unique_ptr<Error> clone() const = 0;
};

/**
 * @brief A basic Error implementation that stores a plain string message.
 *
 * @details
 * SimpleError provides the simplest possible Error implementation, storing a single string
 * message without any special formatting or structure. This class is useful for quick error
 * creation, wrapping existing string error messages, or when no special formatting is required.
 *
 * The details() method returns the stored string unchanged, ignoring the TextFormatter parameter
 * since no conditional formatting is applied. This makes SimpleError suitable for plain text
 * error messages that should appear the same regardless of output context.
 *
 * SimpleError is marked final to prevent inheritance, as it's designed to be a leaf class
 * in the error hierarchy. For errors requiring custom formatting or additional context,
 * create a new Error subclass rather than extending SimpleError.
 */
class SimpleError final : public Error {
  public:
    /**
     * @brief Constructs a SimpleError with the given message.
     *
     * @param details The error message to store
     */
    explicit SimpleError(std::string details) : details_(std::move(details)) {}

    /**
     * @brief Returns the stored error message unchanged.
     *
     * @details
     * Unlike more sophisticated Error implementations, SimpleError ignores the formatter
     * parameter and always returns the plain string message without any formatting.
     *
     * @param formatter Unused parameter, maintained for interface compatibility
     * @return The stored error message string
     */
    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        return details_;
    }

    /**
     * @brief Creates a copy of this SimpleError.
     *
     * @return A unique_ptr to a new SimpleError with the same message
     */
    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<SimpleError>(details_);
    }

  private:
    std::string details_;
};

} // namespace porytiles2
