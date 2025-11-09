#pragma once

#include <concepts>
#include <cstdlib>
#include <source_location>
#include <string_view>

#include "fmt/format.h"

namespace porytiles2 {

// TODO: this should be in the utilities tree, it's a true utility

/**
 * @brief A wrapper for std::string_view with a taggable std::source_location.
 *
 * @details
 * This struct combines a message with source location information for debugging and error reporting. It automatically
 * captures the source location at the point of construction when used with the default parameter. Inspired by:
 * https://buildingblock.ai/panic
 */
struct StringViewSourceLoc {
    /**
     * @brief Constructs a StringViewSourceLoc with a message and optional source location.
     *
     * @details
     * The constructor accepts any type that can be converted to std::string_view and automatically captures the source
     * location if not explicitly provided.
     *
     * @tparam T Type that can be constructed into std::string_view
     * @param msg The message to store
     * @param loc The source location (defaults to current location)
     */
    template <class T>
        requires std::constructible_from<std::string_view, T>
    // NOLINTNEXTLINE
    StringViewSourceLoc(const T &msg, const std::source_location loc = std::source_location::current()) noexcept
        : msg_{msg}, loc_{loc}
    {
    }

    std::string_view msg_;
    std::source_location loc_;
};

/**
 * @brief Unconditionally terminates the program with a panic message.
 *
 * @details
 * This function formats and prints a panic message containing the source location and user message, then aborts the
 * program. The function never returns.
 *
 * @param s The StringViewSourceLoc containing the panic message and location
 */
[[noreturn]] inline void panic(const StringViewSourceLoc &s)
{
    const auto msg = fmt::format("{}:{} panic: {}\n", s.loc_.file_name(), s.loc_.line(), s.msg_);
    std::fputs(msg.c_str(), stderr);
    std::abort();
}

/**
 * @brief Conditionally panics if the given condition is false.
 *
 * @details
 * This function checks the provided condition and if it evaluates to false, formats and prints a panic message with
 * source location information, then aborts the program. If the condition is true, the function returns normally.
 *
 * @param condition The condition to check
 * @param s The StringViewSourceLoc containing the panic message and location
 */
inline void assert_or_panic(const bool condition, const StringViewSourceLoc &s)
{
    if (!condition) {
        const auto msg = fmt::format("{}:{} panic: {}\n", s.loc_.file_name(), s.loc_.line(), s.msg_);
        std::fputs(msg.c_str(), stderr);
        std::abort();
    }
}

} // namespace porytiles2
