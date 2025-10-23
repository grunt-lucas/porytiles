#pragma once

#include <functional>
#include <ranges>
#include <string>

#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/xcut/result/error.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for structured error reporting and diagnostic output.
 *
 * @details
 * UserDiagnostics provides a polymorphic interface for communicating diagnostics, warnings, errors, and fatal
 * conditions to users. The class supports multiple severity levels and integrates with the ChainableResult error system
 * to provide hierarchical error chain visualization.
 *
 * The diagnostic system categorizes output into:
 * - Notes: Informational messages for user awareness
 * - Warning Notes: Notes that are emitted in concert with a particular warning
 * - Warnings: Non-fatal issues with categorization tags
 * - Errors: Serious issues requiring attention
 * - Fatal Errors: Complete failure scenarios with full error context
 *
 * All methods support both single-line messages (std::string) and multi-line messages (std::vector<std::string>) for
 * flexible diagnostic formatting.
 */
class UserDiagnostics {
  public:
    virtual ~UserDiagnostics() = default;

    /**
     * @brief Display an informational note message.
     *
     * @details
     * Notes are the lowest severity diagnostic level, used for informational messages that help users understand what's
     * happening without indicating any problems.
     *
     * @param msg The informational message to display
     */
    void note(const std::string &msg) const
    {
        note(std::vector{msg});
    }

    /**
     * @brief Display a multi-line informational note message.
     *
     * @details
     * Virtual method for displaying multi-line informational messages. Implementations typically format the first line
     * with a "note:" prefix and subsequent lines with appropriate indentation.
     *
     * @param lines Vector of strings representing each line of the note
     */
    virtual void note(const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Display a note message with a warning tag.
     *
     * @details
     * Warning notes are note messages that are emitted in concert with a specific warning. They have tags so that like
     * warnings they can be filtered by the user.
     *
     * @param tag Categorization tag for the warning note
     * @param msg The warning note message to display
     */
    void warn_note(const std::string &tag, const std::string &msg) const
    {
        warn_note(tag, std::vector{msg});
    }

    /**
     * @brief Display a multi-line tagged warning note message.
     *
     * @details
     * Virtual method for displaying multi-line warning notes with categorization. Implementations typically format
     * these with note-style prefixes but include the tag for categorization purposes.
     *
     * @param tag Categorization tag for the warning note
     * @param lines Vector of strings representing each line of the warning note
     */
    virtual void warn_note(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    /**
     * @brief Display a tagged warning message.
     *
     * @details
     * Warnings indicate non-fatal issues that users should be aware of. They include a categorization tag to help users
     * understand and/or filter the type of warning being reported.
     *
     * @param tag Categorization tag for the warning
     * @param msg The warning message to display
     */
    void warn(const std::string &tag, const std::string &msg) const
    {
        warn(tag, std::vector{msg});
    }

    /**
     * @brief Display a multi-line tagged warning message.
     *
     * @details
     * Virtual method for displaying multi-line warnings with categorization. Implementations typically format the first
     * line with a "warning:" prefix and the tag, with subsequent lines appropriately indented.
     *
     * @param tag Categorization tag for the warning
     * @param lines Vector of strings representing each line of the warning
     */
    virtual void warn(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    // /**
    //  * @brief Display a tagged warning message using a formatter-aware builder function.
    //  *
    //  * @details
    //  * This overload allows callers to provide a function that dynamically generates warning messages with access to
    //  * text formatting capabilities. The TextFormatter is provided to enable conditional styling based on TTY output
    //  * settings.
    //  *
    //  * @param tag Categorization tag for the warning
    //  * @param msg_builder Function that receives a TextFormatter reference and returns formatted message lines
    //  */
    // void warn(const std::string &tag, const FormattedMessageBuilder &msg_builder) const
    // {
    //     // TODO: inject this formatter
    //     AnsiStyledTextFormatter formatter{};
    //     warn(tag, msg_builder(formatter));
    // }

    /**
     * @brief Display an error message.
     *
     * @details
     * Errors indicate serious issues that require attention but don't necessarily cause immediate failure of the
     * current operation.
     *
     * @param msg The error message to display
     */
    void err(const std::string &msg) const
    {
        err(std::vector{msg});
    }

    /**
     * @brief Display a multi-line error message.
     *
     * @details
     * Virtual method for displaying multi-line error messages. Implementations typically format the first line with
     * an "error:" prefix and subsequent lines with appropriate indentation.
     *
     * @param lines Vector of strings representing each line of the error
     */
    virtual void err(const std::vector<std::string> &lines) const = 0;

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
     * @tparam T The success value type of the ChainableResult
     * @tparam E The error type of the ChainableResult
     * @param result The failed ChainableResult containing the error chain
     * @pre result.has_value() must be false (contains an error)
     * @pre result.chain() must not be empty
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

        // If all errors were blank FormattableErrors (defensive, shouldn't happen), return early
        if (filtered_chain.empty()) {
            panic("filtered error chain was empty, there should always be at least one FormattableError with details");
        }

        /*
         * TODO: should we provide some mechanism to support multi-line error messages for each fatal step? This would
         * allow for more intuitive error messages in many cases. E.g. LayerModeConverter::detect_layer_mode writes a
         * detailed err before returning FormattableError with the root cause. The problem is that from the user
         * perspective, it's not immediately obvious that the err, which gets printed first, is related to the fatal
         * chain. This is a case where it makes sense to have the root cause be detailed and multi-lined.
         *
         * In other cases, it might make sense to print a bunch of different errors before showing the fatal. I am
         * thinking of tile max color count validation. We'd want to iterate over every input tile and report all tiles
         * that have > 15 colors before bailing. It would be a frustrating user experience to have the program die on
         * the first tile it hits if they have > 15 colors. It would also be strange to have a root cause message with
         * dozens of lines detailing each tile that failed. Having a separate error message for each tile failure reads
         * more naturally. So there really is a need for both types of error reporting.
         */

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
};

} // namespace porytiles2
