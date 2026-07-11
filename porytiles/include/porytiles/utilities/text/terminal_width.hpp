#pragma once

#include <cstddef>

namespace porytiles {

/// @brief Resolves the column width to use for wrapping diagnostic output on @p fd.
///
/// @details
/// Consults sources in priority order so callers get the width the user actually wants:
///
/// 1. The `COLUMNS` environment variable, when set to a positive integer. This lets users and tests override detection
///    regardless of whether @p fd is a terminal.
/// 2. `ioctl(fd, TIOCGWINSZ)`, when @p fd is a terminal reporting a non-zero column count.
/// 3. The @p fallback, used when neither source yields a usable width (for example when output is redirected).
///
/// The resolution is intentionally kept as a single free function so a future config layer can decide the width (fixed
/// value, disabled, or auto-detected) and hand the result to the diagnostics printer without the printer needing to
/// know how it was chosen.
///
/// @param fd The file descriptor whose terminal to measure (typically STDERR_FILENO)
/// @param fallback The width to return when no terminal width can be determined
/// @return The resolved column width
[[nodiscard]] std::size_t resolve_terminal_width(int fd, std::size_t fallback = 80);

} // namespace porytiles
