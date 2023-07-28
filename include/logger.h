#ifndef PORYTILES_LOGGER_H
#define PORYTILES_LOGGER_H

#include <string>
#include <cstdio>

#define FMT_HEADER_ONLY
#include "fmt/color.h"
#include "config.h"
#include "program_name.h"

namespace porytiles {

template <typename... T>
void pt_logln(std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    fmt::println(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T>
void pt_log(std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    fmt::print(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T>
void pt_logln_verbose(const Config& config, std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::println(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
    }
}

template <typename... T>
void pt_log_verbose(const Config& config, std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::print(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
    }
}

template <typename... T>
void pt_msgln(std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    fmt::println(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T>
void pt_msg(std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    fmt::print(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T>
void pt_msgln_verbose(const Config& config, std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::println(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
    }
}

template <typename... T>
void pt_msg_verbose(const Config& config, std::FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::print(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
    }
}

template <typename... T>
void pt_err(fmt::format_string<T...> fmt, T&&... args) {
    fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
        fmt::styled("error:",fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)), fmt::format(fmt, std::forward<T>(args)...)
    );
}

template <typename... T>
void pt_warn(fmt::format_string<T...> fmt, T&&... args) {
    fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
                 fmt::styled("warning:",fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta)), fmt::format(fmt, std::forward<T>(args)...)
    );
}

template <typename... T>
void pt_note(fmt::format_string<T...> fmt, T&&... args) {
    fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
                 fmt::styled("note:",fmt::emphasis::bold | fmt::fg(fmt::terminal_color::cyan)), fmt::format(fmt, std::forward<T>(args)...)
    );
}

}

#endif //PORYTILES_LOGGER_H
