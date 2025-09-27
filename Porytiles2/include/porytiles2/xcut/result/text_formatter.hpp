#pragma once

#include <string>
#include <vector>

namespace porytiles2 {

/**
 * @brief A text formatter with a configurable TTY switch.
 *
 * @details
 * The TextFormatter provides conditional text formatting based on TTY output status. When TTY output is enabled,
 * formatting methods return strings with embedded ANSI escape codes for styling. When TTY output is disabled,
 * the methods return plain text without any formatting codes. This allows callers to include conditional
 * formatting in their output without needing to handle TTY detection themselves.
 */
class TextFormatter {
  public:
    explicit TextFormatter(bool is_a_tty) : is_a_tty_{is_a_tty} {}

    /**
     * @brief Format text in bold style.
     *
     * @details
     * Returns the input text wrapped with ANSI bold formatting codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with bold styling (if TTY) or plain text
     */
    [[nodiscard]] std::string bold(const std::string &text) const;

    /**
     * @brief Format text in red color.
     *
     * @details
     * Returns the input text wrapped with ANSI red color codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with red color (if TTY) or plain text
     */
    [[nodiscard]] std::string red(const std::string &text) const;

    /**
     * @brief Format text in cyan color.
     *
     * @details
     * Returns the input text wrapped with ANSI cyan color codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with cyan color (if TTY) or plain text
     */
    [[nodiscard]] std::string cyan(const std::string &text) const;

    /**
     * @brief Format text in magenta color.
     *
     * @details
     * Returns the input text wrapped with ANSI magenta color codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with magenta color (if TTY) or plain text
     */
    [[nodiscard]] std::string magenta(const std::string &text) const;

    /**
     * @brief Format text in bold red style.
     *
     * @details
     * Returns the input text wrapped with ANSI bold + red formatting codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with bold red styling (if TTY) or plain text
     */
    [[nodiscard]] std::string red_bold(const std::string &text) const;

    /**
     * @brief Format text in bold cyan style.
     *
     * @details
     * Returns the input text wrapped with ANSI bold + cyan formatting codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with bold cyan styling (if TTY) or plain text
     */
    [[nodiscard]] std::string cyan_bold(const std::string &text) const;

    /**
     * @brief Format text in bold magenta style.
     *
     * @details
     * Returns the input text wrapped with ANSI bold + magenta formatting codes if TTY output is enabled,
     * otherwise returns the plain text unchanged.
     *
     * @param text The text to format
     * @return Formatted text with bold magenta styling (if TTY) or plain text
     */
    [[nodiscard]] std::string magenta_bold(const std::string &text) const;

    /**
     * @brief Format a template string with parameters rendered in bold.
     *
     * @details
     * This method performs parameter substitution into a format string where each parameter
     * is automatically styled in bold (if TTY output is enabled). The format string should
     * contain "{}" placeholders that will be replaced with the corresponding parameters.
     *
     * @param format_str The format string with "{}" placeholders
     * @param params Vector of parameter values to substitute
     * @return Formatted string with bold parameters (if TTY) or plain substitution
     */
    [[nodiscard]] std::string
    format_with_bold_params(const std::string &format_str, const std::vector<std::string> &params) const;

  private:
    bool is_a_tty_;
};

} // namespace porytiles2
