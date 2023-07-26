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
void pt_logln(const Config& config, FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::println(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
    }
}

template <typename... T>
void pt_log(const Config& config, FILE* stream, fmt::format_string<T...> fmt, T&&... args) {
    if (config.verbose) {
        fmt::print(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
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

}

#endif //PORYTILES_LOGGER_H
