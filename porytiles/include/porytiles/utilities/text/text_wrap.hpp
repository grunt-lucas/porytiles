#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace porytiles {

/// @brief Word-wraps a single logical line to a visible column width, preserving ANSI styling.
///
/// @details
/// Breaks @p line into as many physical lines as needed so that each one occupies at most @p width visible columns.
/// The wrapping is ANSI-aware in two ways:
///
/// - **Zero-width escapes**: ANSI SGR sequences (e.g. `\033[1m`, `\033[38;2;r;g;bm`, `\033[0m`) do not count toward the
///   visible width, and the function never breaks in the middle of one.
/// - **Style carry-over**: when a break lands inside an open style span, the physical line is closed with a reset and
///   the next physical line re-opens the still-active styles, so color never bleeds into the gutter or drops on
///   continuation lines.
///
/// Breaks prefer space boundaries; the single space at a break point is consumed rather than rendered. A word longer
/// than @p width on its own is hard-broken at the column limit. UTF-8 sequences are treated as one visible column each
/// and are never split.
///
/// @param line The logical line to wrap (may contain ANSI escape sequences)
/// @param width The maximum visible columns per physical line; a value of 0 disables wrapping
/// @return The physical lines; a single-element vector containing @p line unchanged when @p width is 0
[[nodiscard]] std::vector<std::string> wrap_ansi_line(const std::string &line, std::size_t width);

} // namespace porytiles
