#pragma once

#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/**
 * @brief TextFormatter implementation that strips all styling from text.
 *
 * @details
 * PlainTextFormatter is a concrete implementation of TextFormatter that returns text without any styling applied,
 * regardless of the Style flags provided. This formatter is intended for use when output is directed to non-TTY
 * destinations such as files or pipes, where ANSI escape codes would appear as visible garbage characters.
 *
 * The style() method ignores all Style flags and returns the input text unchanged, effectively stripping any
 * requested styling. This ensures clean, readable output in contexts where terminal styling is not appropriate.
 *
 * Usage context:
 * - File output (logs, reports)
 * - Pipe output to other programs
 * - Non-TTY terminal contexts
 * - Any scenario where styled output would be undesirable
 */
class PlainTextFormatter final : public TextFormatter {
  public:
    /**
     * @brief Returns text unchanged, ignoring all styling.
     *
     * @details
     * Implements the style() interface by simply returning the input text without applying any formatting, regardless
     * of the Style flags provided. This ensures output remains clean and readable in non-TTY contexts.
     *
     * @param text The text to format
     * @param style The Style flags (ignored in this implementation)
     * @return The original text without any modifications
     */
    [[nodiscard]] std::string style(const std::string &text, Style style) const override;
};

} // namespace porytiles
