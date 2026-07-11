#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

/// @brief Predefined color options for text styling.
///
/// @details
/// PredefinedColor represents the standard 8 ANSI colors that can be used for both foreground and background text
/// styling. The 'none' value indicates no color is set.
enum class PredefinedColor : std::uint8_t {
    none,    ///< No color set
    black,   ///< Black color
    red,     ///< Red color
    green,   ///< Green color
    yellow,  ///< Yellow color
    blue,    ///< Blue color
    magenta, ///< Magenta color
    cyan,    ///< Cyan color
    white    ///< White color
};

/// @brief RGB color representation for extracting color components from Style values.
///
/// @details
/// RgbColor holds the red, green, and blue components of a color extracted from a Style value in RGB mode.
/// This struct is returned by fg_rgb() and bg_rgb() to provide convenient access to individual color channels.
struct RgbColor {
    std::uint8_t r; ///< Red channel (0-255)
    std::uint8_t g; ///< Green channel (0-255)
    std::uint8_t b; ///< Blue channel (0-255)
};

/// @brief Text styling class supporting formatting, foreground colors, and background colors.
///
/// @details
/// Style is a class that allows multiple styling attributes to be combined using the | operator. It supports:
/// - Format attributes: bold, italic
/// - Foreground colors: predefined (8 ANSI colors) or custom RGB
/// - Background colors: predefined (8 ANSI colors) or custom RGB
///
/// Styles can be combined using the | operator to apply multiple attributes simultaneously:
/// ```c++
/// Style::bold | Style::red                    // Bold red foreground
/// Style::red | Style::bg_blue                 // Red foreground on blue background
/// rgb_fg_style(255, 128, 0) | Style::bg_black // Orange foreground on black background
/// ```
///
/// The class maintains backward compatibility with the previous enum-based API while providing more flexibility
/// for combining foreground and background colors.
class Style {
  public:
    // Formatting constants
    static const Style none;      ///< No styling applied
    static const Style bold;      ///< Bold text formatting
    static const Style faint;     ///< Faint text formatting
    static const Style italic;    ///< Italic text formatting
    static const Style underline; ///< Underline text formatting
    static const Style blink;     ///< Blink text formatting

    // Foreground color constants
    static const Style black;   ///< Black foreground color
    static const Style red;     ///< Red foreground color
    static const Style green;   ///< Green foreground color
    static const Style yellow;  ///< Yellow foreground color
    static const Style blue;    ///< Blue foreground color
    static const Style magenta; ///< Magenta foreground color
    static const Style cyan;    ///< Cyan foreground color
    static const Style white;   ///< White foreground color

    // Background color constants
    static const Style bg_black;   ///< Black background color
    static const Style bg_red;     ///< Red background color
    static const Style bg_green;   ///< Green background color
    static const Style bg_yellow;  ///< Yellow background color
    static const Style bg_blue;    ///< Blue background color
    static const Style bg_magenta; ///< Magenta background color
    static const Style bg_cyan;    ///< Cyan background color
    static const Style bg_white;   ///< White background color

    /// @brief Default constructor creating a style with no attributes.
    constexpr Style() = default;

    /// @brief Combines two Style values using the | operator.
    ///
    /// @details
    /// Merges styling attributes from two Style values:
    /// - Format flags (bold, italic) are combined with bitwise OR
    /// - Foreground color: right-hand side wins if both have foreground colors
    /// - Background color: right-hand side wins if both have background colors
    ///
    /// @param other The Style value to combine with
    /// @return A new Style value with combined attributes
    [[nodiscard]] constexpr Style operator|(const Style &other) const
    {
        Style result{};

        // Combine format flags with bitwise OR
        result.format_flags_ = format_flags_ | other.format_flags_;

        // Foreground color: right-hand side wins if both have foreground colors
        if (other.has_fg_color()) {
            result.fg_predefined_ = other.fg_predefined_;
            result.fg_rgb_ = other.fg_rgb_;
            result.has_fg_rgb_ = other.has_fg_rgb_;
        }
        else if (has_fg_color()) {
            result.fg_predefined_ = fg_predefined_;
            result.fg_rgb_ = fg_rgb_;
            result.has_fg_rgb_ = has_fg_rgb_;
        }

        // Background color: right-hand side wins if both have background colors
        if (other.has_bg_color()) {
            result.bg_predefined_ = other.bg_predefined_;
            result.bg_rgb_ = other.bg_rgb_;
            result.has_bg_rgb_ = other.has_bg_rgb_;
        }
        else if (has_bg_color()) {
            result.bg_predefined_ = bg_predefined_;
            result.bg_rgb_ = bg_rgb_;
            result.has_bg_rgb_ = has_bg_rgb_;
        }

        return result;
    }

    /// @brief Checks if this Style has bold formatting.
    ///
    /// @return True if bold formatting is set
    [[nodiscard]] constexpr bool has_bold() const
    {
        return (format_flags_ & bold_flag) != 0;
    }

    /// @brief Checks if this Style has faint formatting.
    ///
    /// @return True if faint formatting is set
    [[nodiscard]] constexpr bool has_faint() const
    {
        return (format_flags_ & faint_flag) != 0;
    }

    /// @brief Checks if this Style has italic formatting.
    ///
    /// @return True if italic formatting is set
    [[nodiscard]] constexpr bool has_italic() const
    {
        return (format_flags_ & italic_flag) != 0;
    }

    /// @brief Checks if this Style has underline formatting.
    ///
    /// @return True if underline formatting is set
    [[nodiscard]] constexpr bool has_underline() const
    {
        return (format_flags_ & underline_flag) != 0;
    }

    /// @brief Checks if this Style has blink formatting.
    ///
    /// @return True if blink formatting is set
    [[nodiscard]] constexpr bool has_blink() const
    {
        return (format_flags_ & blink_flag) != 0;
    }

    /// @brief Checks if this Style has a foreground color set.
    ///
    /// @return True if a foreground color (predefined or RGB) is set
    [[nodiscard]] constexpr bool has_fg_color() const
    {
        return has_fg_rgb_ || fg_predefined_ != PredefinedColor::none;
    }

    /// @brief Checks if this Style has a background color set.
    ///
    /// @return True if a background color (predefined or RGB) is set
    [[nodiscard]] constexpr bool has_bg_color() const
    {
        return has_bg_rgb_ || bg_predefined_ != PredefinedColor::none;
    }

    /// @brief Checks if the foreground color is in RGB mode.
    ///
    /// @return True if foreground uses custom RGB color
    [[nodiscard]] constexpr bool is_fg_rgb() const
    {
        return has_fg_rgb_;
    }

    /// @brief Checks if the background color is in RGB mode.
    ///
    /// @return True if background uses custom RGB color
    [[nodiscard]] constexpr bool is_bg_rgb() const
    {
        return has_bg_rgb_;
    }

    /// @brief Returns the foreground RGB color.
    ///
    /// @pre is_fg_rgb() must be true
    /// @return The RgbColor representing the foreground color
    [[nodiscard]] constexpr RgbColor fg_rgb() const
    {
        return fg_rgb_;
    }

    /// @brief Returns the background RGB color.
    ///
    /// @pre is_bg_rgb() must be true
    /// @return The RgbColor representing the background color
    [[nodiscard]] constexpr RgbColor bg_rgb() const
    {
        return bg_rgb_;
    }

    /// @brief Returns the predefined foreground color.
    ///
    /// @pre !is_fg_rgb() must be true (foreground is not RGB)
    /// @return The PredefinedColor for the foreground
    [[nodiscard]] constexpr PredefinedColor fg_predefined() const
    {
        return fg_predefined_;
    }

    /// @brief Returns the predefined background color.
    ///
    /// @pre !is_bg_rgb() must be true (background is not RGB)
    /// @return The PredefinedColor for the background
    [[nodiscard]] constexpr PredefinedColor bg_predefined() const
    {
        return bg_predefined_;
    }

  private:
    static constexpr std::uint8_t bold_flag = 1 << 0;
    static constexpr std::uint8_t faint_flag = 1 << 1;
    static constexpr std::uint8_t italic_flag = 1 << 2;
    static constexpr std::uint8_t underline_flag = 1 << 3;
    static constexpr std::uint8_t blink_flag = 1 << 4;

    std::uint8_t format_flags_{0};
    PredefinedColor fg_predefined_{PredefinedColor::none};
    PredefinedColor bg_predefined_{PredefinedColor::none};
    RgbColor fg_rgb_{0, 0, 0};
    RgbColor bg_rgb_{0, 0, 0};
    bool has_fg_rgb_{false};
    bool has_bg_rgb_{false};

    // Private constructors for creating specific style types
    enum class FormatFlagTag { bold, faint, italic, underline, blink };
    enum class FgColorTag { predefined, rgb };
    enum class BgColorTag { predefined, rgb };

    constexpr explicit Style(FormatFlagTag tag)
    {
        switch (tag) {
        case FormatFlagTag::bold:
            format_flags_ = bold_flag;
            break;
        case FormatFlagTag::faint:
            format_flags_ = faint_flag;
            break;
        case FormatFlagTag::italic:
            format_flags_ = italic_flag;
            break;
        case FormatFlagTag::underline:
            format_flags_ = underline_flag;
            break;
        case FormatFlagTag::blink:
            format_flags_ = blink_flag;
            break;
        default:
            panic("unhandled FormatFlagTag value");
        }
    }

    constexpr explicit Style(FgColorTag, PredefinedColor color) : fg_predefined_{color} {}

    constexpr explicit Style(BgColorTag, PredefinedColor color) : bg_predefined_{color} {}

    constexpr explicit Style(FgColorTag, std::uint8_t r, std::uint8_t g, std::uint8_t b)
        : fg_rgb_{r, g, b}, has_fg_rgb_{true}
    {
    }

    constexpr explicit Style(BgColorTag, std::uint8_t r, std::uint8_t g, std::uint8_t b)
        : bg_rgb_{r, g, b}, has_bg_rgb_{true}
    {
    }

    // Friend functions for creating RGB styles
    friend constexpr Style rgb_fg_style(std::uint8_t r, std::uint8_t g, std::uint8_t b);
    friend constexpr Style rgb_bg_style(std::uint8_t r, std::uint8_t g, std::uint8_t b);
};

/// @brief Creates a Style value with a custom RGB foreground color.
///
/// @details
/// Constructs a Style value with an RGB foreground color using the provided red, green, and blue channel values.
/// The resulting Style can be combined with formatting flags and background colors using the | operator:
/// ```c++
/// Style custom = rgb_fg_style(255, 128, 0) | Style::bold;
/// Style with_bg = rgb_fg_style(255, 128, 0) | Style::bg_black;
/// ```
///
/// @param r Red channel value (0-255)
/// @param g Green channel value (0-255)
/// @param b Blue channel value (0-255)
/// @return A Style value with RGB foreground color
[[nodiscard]] constexpr Style rgb_fg_style(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return Style{Style::FgColorTag::rgb, r, g, b};
}

/// @brief Creates a Style value with a custom RGB background color.
///
/// @details
/// Constructs a Style value with an RGB background color using the provided red, green, and blue channel values.
/// The resulting Style can be combined with formatting flags and foreground colors using the | operator:
/// ```c++
/// Style custom = rgb_bg_style(64, 64, 64) | Style::bold;
/// Style with_fg = Style::red | rgb_bg_style(255, 255, 0);
/// ```
///
/// @param r Red channel value (0-255)
/// @param g Green channel value (0-255)
/// @param b Blue channel value (0-255)
/// @return A Style value with RGB background color
[[nodiscard]] constexpr Style rgb_bg_style(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return Style{Style::BgColorTag::rgb, r, g, b};
}

/// @brief Creates a Style value with a custom RGB foreground color (backward compatibility alias).
///
/// @details
/// This function is an alias for rgb_fg_style() to maintain backward compatibility with code that used
/// the previous enum-based API. New code should prefer rgb_fg_style() for clarity.
///
/// @param r Red channel value (0-255)
/// @param g Green channel value (0-255)
/// @param b Blue channel value (0-255)
/// @return A Style value with RGB foreground color
[[nodiscard]] constexpr Style rgb_style(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return rgb_fg_style(r, g, b);
}

/// @brief A text parameter with associated styling for formatted output.
///
/// @details
/// FormatParam pairs a text string with Style flags to enable styled parameter substitution in formatted strings.
/// When used with TextFormatter::format(), the text is styled according to the formatter implementation (ANSI codes
/// for TTY, plain text for non-TTY) before being substituted into the format string.
///
/// Example usage:
/// ```C++
/// FormattableError{"tileset '{}' not found", FormatParam{name, Style::bold}}
/// ```
///
/// This struct is typically constructed inline when creating error messages or formatted diagnostic output.
class FormatParam {
  public:
    /// @brief Constructs a FormatParam with unstyled text.
    ///
    /// @details
    /// Creates a FormatParam with the given text and no styling applied (Style::none).
    ///
    /// @param text The text content to be formatted
    FormatParam(std::string text) : text_{std::move(text)}, styles_{Style::none} {}

    /// @brief Constructs a FormatParam with styled text.
    ///
    /// @details
    /// Creates a FormatParam with the given text and specified styling attributes.
    ///
    /// @param text The text content to be formatted
    /// @param styles The styling attributes to apply to the text
    explicit FormatParam(std::string text, Style styles) : text_{std::move(text)}, styles_{styles} {}

    /// @brief Constructs a FormatParam by converting a value to string.
    ///
    /// @details
    /// Creates a FormatParam by converting the given value to a string using @c std::format.
    /// This constructor is constrained to accept types that satisfy @c std::formattable<T, char>,
    /// excluding @c std::string to avoid ambiguity with the existing string constructor.
    ///
    /// @tparam T The type of the value (must satisfy @c std::formattable<T, char>)
    /// @param value The value to convert to string
    template <typename T>
        requires(
            !std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, FormatParam> &&
            std::formattable<std::decay_t<T>, char>)
    FormatParam(T &&value) : FormatParam(resolve_string(std::forward<T>(value)))
    {
    }

    /// @brief Constructs a FormatParam by converting a value to styled string.
    ///
    /// @details
    /// Creates a FormatParam by converting the given value to a string using @c std::format,
    /// with the specified styling attributes applied.
    ///
    /// @tparam T The type of the value (must satisfy @c std::formattable<T, char>)
    /// @param value The value to convert to string
    /// @param styles The styling attributes to apply to the text
    template <typename T>
        requires(
            !std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, FormatParam> &&
            std::formattable<std::decay_t<T>, char>)
    explicit FormatParam(T &&value, Style styles) : FormatParam(resolve_string(std::forward<T>(value)), styles)
    {
    }

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

    /// @brief Resolves a value to its string representation via @c std::format.
    ///
    /// @details
    /// To add formatting support for a new type, provide a @c std::formatter<T> specialization
    /// that delegates to the type's @c porytiles::to_string() overload.
    ///
    /// @tparam T The type of value to convert (must satisfy @c std::formattable<T, char>)
    /// @param value The value to convert to string
    /// @return String representation of the value
    template <typename T>
    static std::string resolve_string(T &&value)
    {
        return std::format("{}", std::forward<T>(value));
    }
};

/// @brief Abstract base class for applying text styling with context-aware formatting.
///
/// @details
/// TextFormatter provides a polymorphic interface for applying text styles in a way that adapts to the output context.
/// Concrete implementations can choose whether to apply styling based on factors like TTY detection, allowing the same
/// code to produce styled terminal output or plain text for file output.
///
/// The class provides two key capabilities:
/// - style(): Apply Style flags to individual text strings
/// - format(): Substitute styled FormatParams into format strings using fmtlib syntax
///
/// Implementations:
/// - PlainTextFormatter: Returns text unchanged, stripping all styles (for non-TTY/files)
/// - AnsiStyledTextFormatter: Applies ANSI escape codes for terminal colors (for TTY)
///
/// This class integrates with the error reporting system through FormattableError and UserDiagnostics to provide
/// adaptive styling for diagnostic messages.
class TextFormatter {
  public:
    virtual ~TextFormatter() = default;

    /// @brief Applies styling to a text string.
    ///
    /// @details
    /// Pure virtual method that concrete formatters must implement to apply the specified Style flags to the given
    /// text. The implementation determines whether to actually apply styling (ANSI codes) or return the text unchanged
    /// (plain text).
    ///
    /// @param text The text to style
    /// @param styles The Style flags to apply
    /// @return The styled text string (may be unchanged in PlainTextFormatter)
    [[nodiscard]] virtual std::string style(const std::string &text, Style styles) const = 0;

    /// @brief Formats a string with styled parameters using fmtlib syntax.
    ///
    /// @details
    /// Substitutes styled FormatParams into a format string using fmtlib's formatting system. Each FormatParam's text
    /// is first styled using the style() method, then substituted into the corresponding `{}` placeholder in the format
    /// string.
    ///
    /// Example:
    /// ```C++
    /// formatter.format("Error in file '{}'", {FormatParam{filename, Style::bold}})
    /// ```
    ///
    /// @param format_str The format string with `{}` placeholders
    /// @param params Vector of FormatParams to substitute into placeholders
    /// @return The formatted string with styled parameters substituted
    [[nodiscard]] virtual std::string
    format(const std::string &format_str, const std::vector<FormatParam> &params) const;

    /// @brief Formats a string with styled parameters using variadic template syntax.
    ///
    /// @details
    /// Convenience template that allows passing FormatParams directly as arguments instead of wrapping them in a
    /// std::vector. This provides more natural syntax for formatting calls with a known number of parameters.
    ///
    /// Example:
    /// ```C++
    /// formatter.format("Error in file '{}'", FormatParam{filename, Style::bold | Style::red});
    /// formatter.format("Expected {} but got {}", expected, actual);
    /// ```
    ///
    /// This template is disabled when called with a std::vector<FormatParam> to avoid ambiguity with the base
    /// implementation.
    ///
    /// @tparam FirstParam Type of the first parameter
    /// @tparam RestParams Types of remaining parameters
    /// @param format_str The format string with `{}` placeholders
    /// @param first First FormatParam argument
    /// @param rest Remaining FormatParam arguments to substitute into placeholders
    /// @return The formatted string with styled parameters substituted
    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_constructible_v<FormatParam, FirstParam> &&
            (std::is_constructible_v<FormatParam, RestParams> && ...))
    [[nodiscard]] std::string format(const std::string &format_str, FirstParam &&first, RestParams &&...rest) const
    {
        std::vector<FormatParam> param_vector;
        param_vector.reserve(1 + sizeof...(RestParams));
        param_vector.emplace_back(std::forward<FirstParam>(first));
        (param_vector.emplace_back(std::forward<RestParams>(rest)), ...);
        return this->format(format_str, param_vector);
    }
};

/// @brief Function type for building formatted messages with TextFormatter access.
///
/// @details
/// FormattedMessageBuilder is a function type that receives a TextFormatter reference and returns a vector of formatted
/// message lines. This pattern is used in UserDiagnostics to allow diagnostic messages to be generated with appropriate
/// styling based on the output context (TTY vs non-TTY).
///
/// The builder function can use the TextFormatter to style text dynamically, enabling conditional formatting that
/// adapts to the output destination.
///
/// Example usage:
/// ```C++
/// diag.warn("tag", [&name](const TextFormatter &fmt) {
///     return std::vector{fmt.format("Error in '{}'", FormatParam{name, Style::bold | Style::red})};
/// });
/// ```
using FormattedMessageBuilder = std::function<std::vector<std::string>(const TextFormatter &)>;

} // namespace porytiles
