#ifndef PORYTILES_LOGGER_H
#define PORYTILES_LOGGER_H

#include <cstdio>
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include "config.h"
#include "program_name.h"

namespace porytiles {

template <typename... T>
void pt_logln(const Config &config, std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  if (config.verbose) {
    fmt::println(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
  }
}

template <typename... T> void pt_log(const Config &config, std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  if (config.verbose) {
    fmt::print(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
  }
}

template <typename... T> void pt_msg(std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  fmt::print(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_err(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("error:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)),
               fmt::styled(fmt::format(fmt, std::forward<T>(args)...), fmt::emphasis::bold));
}

template <typename... T> void pt_fatal_err(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("fatal error:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)),
               fmt::styled(fmt::format(fmt, std::forward<T>(args)...), fmt::emphasis::bold));
}

template <typename... T> void pt_warn(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("warning:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta)),
               fmt::styled(fmt::format(fmt, std::forward<T>(args)...), fmt::emphasis::bold));
}

template <typename... T> void pt_note(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("note:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::cyan)),
               fmt::format(fmt, std::forward<T>(args)...));
}

} // namespace porytiles

#endif // PORYTILES_LOGGER_H
