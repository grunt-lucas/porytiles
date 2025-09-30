#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace porytiles2 {

/**
 * @brief Bitmask flags for text styling options.
 *
 * @details
 * Style is a bitmask enum that allows multiple styling attributes to be combined using bitwise operators. Each flag
 * represents a distinct styling attribute that can be applied to text, such as bold formatting or color choices.
 *
 * Styles can be combined using the bitwise OR operator (|) to apply multiple attributes simultaneously. For example,
 * `Style::bold | Style::red` creates a style with both bold formatting and red color.
 *
 * The enum uses an explicit uint32_t underlying type to ensure consistent bitmask behavior across platforms.
 */
enum class Style : uint32_t {
    none = 0,        ///< No styling applied
    bold = 1 << 0,   ///< Bold text formatting
    red = 1 << 1,    ///< Red text color
    green = 1 << 2,  ///< Green text color
    blue = 1 << 3,   ///< Blue text color
    yellow = 1 << 4, ///< Yellow text color
    cyan = 1 << 5,   ///< Cyan text color
    magenta = 1 << 6 ///< Magenta text color
};

/**
 * @brief Combines two Style flags using bitwise OR.
 *
 * @details
 * Allows multiple styling attributes to be combined into a single Style value. This operator enables natural syntax for
 * combining styles, such as `Style::bold | Style::red`.
 *
 * @param lhs The left-hand Style value
 * @param rhs The right-hand Style value
 * @return A Style value with both input flags set
 */
[[nodiscard]] constexpr Style operator|(Style lhs, Style rhs)
{
    return static_cast<Style>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

/**
 * @brief Masks Style flags using bitwise AND.
 *
 * @details
 * Performs a bitwise AND operation on Style values, typically used to check if specific flags are set or to mask out
 * certain style attributes.
 *
 * @param lhs The left-hand Style value
 * @param rhs The right-hand Style value
 * @return A Style value containing only the flags present in both inputs
 */
[[nodiscard]] constexpr Style operator&(Style lhs, Style rhs)
{
    return static_cast<Style>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

/**
 * @brief Adds Style flags to an existing Style value using bitwise OR.
 *
 * @details
 * In-place bitwise OR assignment operator that adds additional style flags to an existing Style value.
 *
 * @param lhs The Style value to modify
 * @param rhs The Style flags to add
 * @return Reference to the modified lhs value
 */
constexpr Style &operator|=(Style &lhs, Style rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

/**
 * @brief Masks an existing Style value using bitwise AND.
 *
 * @details
 * In-place bitwise AND assignment operator that masks an existing Style value, typically used to remove certain flags.
 *
 * @param lhs The Style value to modify
 * @param rhs The Style mask to apply
 * @return Reference to the modified lhs value
 */
constexpr Style &operator&=(Style &lhs, Style rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

/**
 * @brief Checks if a specific style flag is set in a Style value.
 *
 * @details
 * Helper function that tests whether a particular style flag is present in a Style value. This is more readable than
 * using the bitwise operators directly for boolean checks.
 *
 * @param styles The Style value to check
 * @param flag The specific flag to test for
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool has_style(Style styles, Style flag)
{
    return (styles & flag) != Style::none;
}

/**
 * @brief A text parameter with associated styling for formatted output.
 *
 * @details
 * FormatParam pairs a text string with Style flags to enable styled parameter substitution in formatted strings.
 * When used with TextFormatter::format(), the text is styled according to the formatter implementation (ANSI codes
 * for TTY, plain text for non-TTY) before being substituted into the format string.
 *
 * Example usage:
 * ```C++
 * FormattableError{"tileset '{}' not found", FormatParam{name, Style::bold}}
 * ```
 *
 * This struct is typically constructed inline when creating error messages or formatted diagnostic output.
 */
class FormatParam {
  public:
    /**
     * @brief Constructs a FormatParam with unstyled text.
     *
     * @details
     * Creates a FormatParam with the given text and no styling applied (Style::none).
     *
     * @param text The text content to be formatted
     */
    explicit FormatParam(std::string text) : text_{std::move(text)}, styles_{Style::none} {}

    /**
     * @brief Constructs a FormatParam with styled text.
     *
     * @details
     * Creates a FormatParam with the given text and specified styling attributes.
     *
     * @param text The text content to be formatted
     * @param styles The styling attributes to apply to the text
     */
    explicit FormatParam(std::string text, Style styles) : text_{std::move(text)}, styles_{styles} {}

    [[nodiscard]] const std::string &text() const
    {
        return text_;
    }

    [[nodiscard]] Style styles() const
    {
        return styles_;
    }

  private:
    std::string text_; ///< The text content to be formatted
    Style styles_;     ///< The styling attributes to apply to the text
};

/**
 * @brief Abstract base class for applying text styling with context-aware formatting.
 *
 * @details
 * TextFormatter provides a polymorphic interface for applying text styles in a way that adapts to the output context.
 * Concrete implementations can choose whether to apply styling based on factors like TTY detection, allowing the same
 * code to produce styled terminal output or plain text for file output.
 *
 * The class provides two key capabilities:
 * - style(): Apply Style flags to individual text strings
 * - format(): Substitute styled FormatParams into format strings using fmtlib syntax
 *
 * Implementations:
 * - PlainTextFormatter: Returns text unchanged, stripping all styles (for non-TTY/files)
 * - AnsiStyledTextFormatter: Applies ANSI escape codes for terminal colors (for TTY)
 *
 * This class integrates with the error reporting system through FormattableError and UserDiagnostics to provide
 * adaptive styling for diagnostic messages.
 */
class TextFormatter {
  public:
    virtual ~TextFormatter() = default;

    /**
     * @brief Applies styling to a text string.
     *
     * @details
     * Pure virtual method that concrete formatters must implement to apply the specified Style flags to the given
     * text. The implementation determines whether to actually apply styling (ANSI codes) or return the text unchanged
     * (plain text).
     *
     * @param text The text to style
     * @param styles The Style flags to apply
     * @return The styled text string (may be unchanged in PlainTextFormatter)
     */
    [[nodiscard]] virtual std::string style(const std::string &text, Style styles) const = 0;

    /**
     * @brief Formats a string with styled parameters using fmtlib syntax.
     *
     * @details
     * Substitutes styled FormatParams into a format string using fmtlib's formatting system. Each FormatParam's text
     * is first styled using the style() method, then substituted into the corresponding `{}` placeholder in the format
     * string.
     *
     * Example:
     * ```C++
     * formatter.format("Error in file '{}'", {FormatParam{filename, Style::bold}})
     * ```
     *
     * @param format_str The format string with `{}` placeholders
     * @param params Vector of FormatParams to substitute into placeholders
     * @return The formatted string with styled parameters substituted
     */
    [[nodiscard]] virtual std::string
    format(const std::string &format_str, const std::vector<FormatParam> &params) const;

    /**
     * @brief Formats a string with styled parameters using variadic template syntax.
     *
     * @details
     * Convenience template that allows passing FormatParams directly as arguments instead of wrapping them in a
     * std::vector. This provides more natural syntax for formatting calls with a known number of parameters.
     *
     * Example:
     * ```C++
     * formatter.format("Error in file '{}'", FormatParam{filename, Style::bold})
     * formatter.format("Expected {} but got {}", FormatParam{expected, Style::green}, FormatParam{actual, Style::red})
     * ```
     *
     * This template is disabled when called with a std::vector<FormatParam> to avoid ambiguity with the base
     * implementation.
     *
     * @tparam FirstParam Type of the first parameter
     * @tparam RestParams Types of remaining parameters
     * @param format_str The format string with `{}` placeholders
     * @param first First FormatParam argument
     * @param rest Remaining FormatParam arguments to substitute into placeholders
     * @return The formatted string with styled parameters substituted
     */
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_same_v<std::decay_t<FirstParam>, FormatParam> &&
            (std::is_same_v<std::decay_t<RestParams>, FormatParam> && ...))
    [[nodiscard]] std::string format(const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        std::vector<FormatParam> param_vector;
        param_vector.reserve(1 + sizeof...(RestParams));
        param_vector.push_back(std::forward<FirstParam>(first));
        (param_vector.push_back(std::forward<RestParams>(rest)), ...);
        return this->format(format_str, param_vector);
    }
};

/**
 * @brief Function type for building formatted messages with TextFormatter access.
 *
 * @details
 * FormattedMessageBuilder is a function type that receives a TextFormatter reference and returns a vector of formatted
 * message lines. This pattern is used in UserDiagnostics to allow diagnostic messages to be generated with appropriate
 * styling based on the output context (TTY vs non-TTY).
 *
 * The builder function can use the TextFormatter to style text dynamically, enabling conditional formatting that adapts
 * to the output destination.
 *
 * Example usage:
 * ```C++
 * diag.warn("tag", [&name](const TextFormatter &fmt) {
 *     return std::vector{fmt.format("Error in '{}'", FormatParam{name, Style::bold | Style::red})};
 * });
 * ```
 */
using FormattedMessageBuilder = std::function<std::vector<std::string>(const TextFormatter &)>;

} // namespace porytiles2
