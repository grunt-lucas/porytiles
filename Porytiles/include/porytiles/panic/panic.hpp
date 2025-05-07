#pragma once

#include <fmt/format.h>

#include <concepts>
#include <format>
#include <source_location>
#include <string_view>
#include <type_traits>

namespace porytiles {

/**
 * @brief A wrapper for std::string_view with a taggable std::source_location.
 *
 * @details
 * Inspired by: https://buildingblock.ai/panic
 */
struct string_view_with_source_loc {
    template <class T>
        requires std::constructible_from<std::string_view, T>
    string_view_with_source_loc(const T &msg, std::source_location loc = std::source_location::current()) noexcept
        : msg{msg}, loc{loc} {}

    std::string_view msg;
    std::source_location loc;
};

[[noreturn]] void panic_impl(const char *s) noexcept;

[[noreturn]] inline void panic(const string_view_with_source_loc &s) noexcept {
    const auto msg = fmt::format("{}:{} panic: {}\n", s.loc.file_name(), s.loc.line(), s.msg);
    panic_impl(msg.c_str());
}

inline void assert_or_panic(const bool condition, const string_view_with_source_loc &s) {
    if (!condition) {
        const auto msg = fmt::format("{}:{} panic: {}\n", s.loc.file_name(), s.loc.line(), s.msg);
        panic_impl(msg.c_str());
    }
}

} // namespace porytiles