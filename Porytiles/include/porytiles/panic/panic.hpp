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
struct StringViewSourceLoc {
    template <class T>
        requires std::constructible_from<std::string_view, T>
    StringViewSourceLoc(const T &msg, std::source_location loc = std::source_location::current()) noexcept
        : msg_{msg}, loc_{loc} {}

    std::string_view msg_;
    std::source_location loc_;
};

[[noreturn]] void PanicImpl(const char *s) noexcept;

[[noreturn]] inline void Panic(const StringViewSourceLoc &s) noexcept {
    const auto msg = fmt::format("{}:{} panic: {}\n", s.loc_.file_name(), s.loc_.line(), s.msg_);
    PanicImpl(msg.c_str());
}

inline void AssertOrPanic(const bool condition, const StringViewSourceLoc &s) {
    if (!condition) {
        const auto msg = fmt::format("{}:{} panic: {}\n", s.loc_.file_name(), s.loc_.line(), s.msg_);
        PanicImpl(msg.c_str());
    }
}

} // namespace porytiles