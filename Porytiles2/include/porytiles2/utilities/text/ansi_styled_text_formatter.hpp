#pragma once

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief TextFormatter implementation that applies ANSI escape codes for terminal styling.
 *
 * @details
 * AnsiStyledTextFormatter is a concrete implementation of TextFormatter that applies ANSI escape codes to text based
 * on the provided Style flags. This formatter is designed for TTY output where terminal emulators can interpret ANSI
 * codes to display colored and styled text.
 *
 * Multiple style flags can be combined (e.g., Style::bold | Style::red) to produce text that has both attributes.
 * The styled text is automatically terminated with an ANSI reset code (\033[0m) to prevent styling from bleeding into
 * subsequent output.
 *
 * Usage context:
 * - Interactive terminal sessions (TTY)
 * - Terminal-based user interfaces
 * - Console diagnostic output
 * - Any scenario where ANSI color codes are supported and desired
 */
class AnsiStyledTextFormatter final : public TextFormatter {
  public:
    /**
     * @brief Applies ANSI escape codes to style text according to the specified Style flags.
     *
     * @details
     * Wraps the input text with appropriate ANSI escape codes based on the provided Style flags. If Style::none is
     * provided, the text is returned unchanged. Otherwise, the text is prefixed with ANSI codes for each set flag and
     * suffixed with an ANSI reset code.
     *
     * The ANSI codes are applied in a specific order (bold first, then colors) to ensure consistent rendering across
     * terminal emulators.
     *
     * @param text The text to style
     * @param styles The Style flags specifying which attributes to apply
     * @return The text wrapped with appropriate ANSI escape codes, or unchanged if styles is Style::none
     */
    [[nodiscard]] std::string style(const std::string &text, Style styles) const override;
};

} // namespace porytiles2
