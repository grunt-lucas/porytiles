#pragma once

#include "fmt/color.h"

namespace porytiles2 {

/**
 * @brief A text formatter with a configurable TTY switch.
 *
 * @details
 * If TTY output is set, this formatter will return ANSI codes that callers can use to format text with various
 * requested stylings. If the TextFormatter's TTY output is not set, the styling methods return a blank styling. This
 * essentially allows clients of TextFormatter to include conditional formatting codes in their output strings, without
 * needing to concern themselves with the TTY status of their consumers.
 */
class TextFormatter {
  public:
    explicit TextFormatter(bool is_a_tty) : is_a_tty_{is_a_tty} {}

    // TODO: these functions return fmtlib types, which violates good design principles
    // We shouldn't be exposing infra concerns in our template library
    [[nodiscard]] fmt::text_style style(const fmt::text_style &ts) const
    {
        return is_a_tty_ ? ts : fmt::text_style{};
    }

    [[nodiscard]] fmt::text_style bold() const
    {
        return style(fmt::emphasis::bold);
    }

    [[nodiscard]] fmt::text_style red() const
    {
        return style(fmt::fg(fmt::terminal_color::red));
    }

  private:
    bool is_a_tty_;
};

} // namespace porytiles2
