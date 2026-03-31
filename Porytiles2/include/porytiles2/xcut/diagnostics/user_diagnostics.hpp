#pragma once

#include <ranges>
#include <string>
#include <type_traits>

#include "gsl/pointers"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

namespace porytiles2 {

/**
 * @brief Abstract class for structured error reporting and diagnostic output.
 *
 * @details
 * UserDiagnostics provides a polymorphic interface for communicating diagnostics, warnings, errors, and fatal
 * conditions to users. The class supports multiple severity levels and integrates with the ChainableResult error system
 * to provide hierarchical error chain visualization.
 *
 * The diagnostic system categorizes output into:
 * - Remark: Compiler messages explaining internal compiler mechanisms / decisions of possible interest to the user
 * - Warnings: Non-fatal issues that indicate possible user input mistakes
 * - Errors: Serious issues requiring attention, operation will die but may continue in order to generate more errors
 * - Fatal Errors: Complete failure scenarios with full error context
 * - Notes: Informational messages for user awareness, associated with a remark, warning, or error
 *
 * All methods support both single-line messages (std::string) and multi-line messages (std::vector<std::string>) for
 * flexible diagnostic formatting.
 */
class UserDiagnostics {
  public:
    virtual ~UserDiagnostics() = default;

    explicit UserDiagnostics(gsl::not_null<const TextFormatter *> format) : format_(format) {}

    /**
     * @brief Display a tagged remark message.
     *
     * @details
     * Remarks are the lowest severity diagnostic level, used for communicating internal compiler mechanisms or
     * decisions that may be of interest to users. Unlike notes (which clarify other remarks/warnings/errors), remarks
     * are standalone informational messages about compiler behavior such as optimization decisions, tile assignment
     * choices, or palette allocation strategies.
     *
     * Implementations typically format the first line with a "remark:" prefix and the tag, with subsequent lines
     * appropriately indented.
     *
     * @param tag Categorization tag for the remark
     * @param lines Vector of strings representing each line of the message
     */
    virtual void remark(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void remark(const std::string &tag, const std::string &msg) const
    {
        remark(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted remark messages.
     *
     * @details
     * Enables inline formatting of remark messages by accepting a format string and variadic FormatParam arguments.
     * The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.remark("tile-assign", "Assigned tile {} to palette {}", tile_id, pal_idx);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the remark
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void remark(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        remark(tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Display a tagged warning message.
     *
     * @details
     * Warnings indicate non-fatal issues that users should be aware of. They include a categorization tag to help
     * users understand and/or filter the type of warning being reported.
     *
     * Implementations typically format the first line with a "warning:" prefix and the tag, with subsequent lines
     * appropriately indented.
     *
     * @param tag Categorization tag for the warning
     * @param lines Vector of strings representing each line of the warning
     */
    virtual void warning(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void warning(const std::string &tag, const std::string &msg) const
    {
        warning(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted warning messages.
     *
     * @details
     * Enables inline formatting of warning messages by accepting a format string and variadic FormatParam arguments.
     * The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.warning("parse", "File {} has {} errors", filename, count);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the warning
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void warning(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        warning(
            tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Display a tagged error message.
     *
     * @details
     * Errors indicate serious issues that require attention but don't necessarily cause immediate failure of the
     * current operation. They include a categorization tag to help users understand and/or filter the type of error
     * being reported.
     *
     * Implementations typically format the first line with an "error:" prefix and subsequent lines with appropriate
     * indentation.
     *
     * @param tag Categorization tag for the error
     * @param lines Vector of strings representing each line of the error
     */
    virtual void error(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void error(const std::string &tag, const std::string &msg) const
    {
        error(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted error messages.
     *
     * @details
     * Enables inline formatting of error messages by accepting a format string and variadic FormatParam arguments.
     * The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.error("validation", "Expected {} but got {}", expected, actual);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the error
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void error(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        error(tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Display a tagged note message associated with a remark.
     *
     * @details
     * Notes are informational messages that further clarify a parent diagnostic. This method is specifically for notes
     * that follow a remark diagnostic, enabling proper filtering - when a remark is filtered out, its associated notes
     * can also be suppressed.
     *
     * Implementations typically format the first line with a "note:" prefix and subsequent lines with appropriate
     * indentation. All note types use identical styling regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent remark's tag)
     * @param lines Vector of strings representing each line of the message
     */
    virtual void remark_note(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void remark_note(const std::string &tag, const std::string &msg) const
    {
        remark_note(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted remark note messages.
     *
     * @details
     * Enables inline formatting of remark note messages by accepting a format string and variadic FormatParam
     * arguments. The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.remark_note("tile-assign", "Previous assignment was at index {}", prev_idx);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the note (should match the parent remark's tag)
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void
    remark_note(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        remark_note(
            tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Display a tagged note message associated with a warning.
     *
     * @details
     * Notes are informational messages that further clarify a parent diagnostic. This method is specifically for notes
     * that follow a warning diagnostic, enabling proper filtering - when a warning is filtered out, its associated
     * notes can also be suppressed.
     *
     * Implementations typically format the first line with a "note:" prefix and subsequent lines with appropriate
     * indentation. All note types use identical styling regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent warning's tag)
     * @param lines Vector of strings representing each line of the message
     */
    virtual void warning_note(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void warning_note(const std::string &tag, const std::string &msg) const
    {
        warning_note(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted warning note messages.
     *
     * @details
     * Enables inline formatting of warning note messages by accepting a format string and variadic FormatParam
     * arguments. The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.warning_note("parse", "Consider using {} instead", alternative);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the note (should match the parent warning's tag)
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void
    warning_note(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        warning_note(
            tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Display a tagged note message associated with an error.
     *
     * @details
     * Notes are informational messages that further clarify a parent diagnostic. This method is specifically for notes
     * that follow an error diagnostic, enabling proper filtering - when an error is filtered out, its associated notes
     * can also be suppressed.
     *
     * Implementations typically format the first line with a "note:" prefix and subsequent lines with appropriate
     * indentation. All note types use identical styling regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent error's tag)
     * @param lines Vector of strings representing each line of the message
     */
    virtual void error_note(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Convenience overload for single line messages.
     */
    void error_note(const std::string &tag, const std::string &msg) const
    {
        error_note(tag, std::vector{msg});
    }

    /**
     * @brief Variadic template overload for formatted error note messages.
     *
     * @details
     * Enables inline formatting of error note messages by accepting a format string and variadic FormatParam arguments.
     * The format string uses fmtlib-style `{}` placeholders.
     *
     * Example:
     * ```C++
     * diag.error_note("validation", "See definition at {}:{}", file, line);
     * ```
     *
     * @tparam FirstParam Type of the first format parameter
     * @tparam RestParams Types of remaining format parameters
     * @param tag Categorization tag for the note (should match the parent error's tag)
     * @param format_str Format string with `{}` placeholders
     * @param first First parameter to substitute
     * @param rest Remaining parameters to substitute
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    void
    error_note(const std::string &tag, const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        error_note(
            tag, formatter().format(format_str, std::forward<FirstParam>(first), std::forward<RestParams>(rest)...));
    }

    /**
     * @brief Emit the proximate (immediate) error in a fatal error chain.
     *
     * @details
     * Virtual method for displaying the most immediate error in a fatal error chain. This is typically the first error
     * encountered and is displayed with the highest visual prominence.
     *
     * @param err The proximate error to display
     */
    virtual void emit_fatal_proximate(const Error &err) const = 0;

    /**
     * @brief Emit an intermediate step error in a fatal error chain.
     *
     * @details
     * Virtual method for displaying intermediate errors in a fatal error chain. These are errors that occurred between
     * the proximate and root causes, typically displayed with tree-like formatting to show the error hierarchy.
     *
     * @param err The step error to display
     */
    virtual void emit_fatal_step(const Error &err) const = 0;

    /**
     * @brief Emit the root cause error in a fatal error chain.
     *
     * @details
     * Virtual method for displaying the root cause error in a fatal error chain. This represents the original
     * underlying cause of the failure and is typically displayed as the final item in the error hierarchy.
     *
     * @param err The root cause error to display
     */
    virtual void emit_fatal_root(const Error &err) const = 0;

    /**
     * @brief Display a fatal error with complete error chain visualization.
     *
     * @details
     * Processes a ChainableResult error chain to display a hierarchical view of all errors that led to the fatal
     * condition. The error chain is displayed in order from proximate (immediate) cause to root cause, creating a
     * tree-like structure that helps users understand the failure path.
     *
     * The method handles three types of errors in the chain:
     * - Proximate: The immediate error (always present)
     * - Steps: Intermediate causes (if chain has >2 errors)
     * - Root: The original cause (if chain has >1 error)
     *
     * @pre result.has_value() must be false (contains an error)
     * @pre result.chain() must not be empty
     *
     * @tparam T The success value type of the ChainableResult
     * @tparam E The error type of the ChainableResult
     *
     * @param result The failed ChainableResult containing the error chain
     */
    template <typename T, typename E>
    void fatal(const ChainableResult<T, E> &result) const
    {
        // Preconditions
        const auto &chain = result.chain();
        assert_or_panic(!result.has_value(), "result was not of error type");
        assert_or_panic(!chain.empty(), "error chain was empty");

        // filter out FormattableErrors with no details
        auto filtered_view = chain | std::views::filter([](const auto &err) {
                                 const auto formattable_err = dynamic_cast<const FormattableError *>(err.get());
                                 if (formattable_err == nullptr) {
                                     return true;
                                 }
                                 return formattable_err->has_details();
                             });

        std::vector<const Error *> filtered_chain;
        for (const auto &err : filtered_view) {
            filtered_chain.push_back(err.get());
        }

        // Unreachable.
        if (filtered_chain.empty()) {
            panic("filtered error chain was empty, there should always be at least one FormattableError with details");
        }

        emit_fatal_proximate(*filtered_chain.at(0));
        if (filtered_chain.size() > 1) {
            // Emit steps for all but the first and last
            const auto middle_range =
                std::ranges::views::drop(filtered_chain, 1) | std::ranges::views::take(filtered_chain.size() - 2);
            for (const auto *err : middle_range) {
                emit_fatal_step(*err);
            }
            // Emit the last one as root
            emit_fatal_root(*filtered_chain.back());
        }
    }

    [[nodiscard]] const TextFormatter &formatter() const
    {
        return *format_;
    }

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2
