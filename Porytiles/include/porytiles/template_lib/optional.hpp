#pragma once

#include <optional>
#include <utility>

namespace porytiles {

/// @brief `IfPresent` conditionally executes a function
/// with the value contained in a `std::optional`.
///
/// @details If the provided `std::optional` object `o` contains a value
/// (i.e., `o.has_value()` is true),
/// this function calls the provided callable `func` with the contained value (`*o`).
/// The callable `func` is perfectly forwarded using `std::forward`.
///
/// @tparam T The type of the value held by the `std::optional`.
/// @tparam F The type of the function or callable object to execute.
/// Must be callable with an argument of type `const T&`.
///
/// @param o The input `std::optional` object to check.
/// If it contains a value, that value will be passed to `func`.
/// @param func The function or callable object to execute,
/// if `o` contains a value.
/// It will be perfectly forwarded
/// and called with the unwrapped value from `o` (as `const T&`).
///
/// @note This function provides a convenient way to apply an operation
/// to the value of an `std::optional` without manually checking `has_value()`.
/// The callable `func` is invoked with the contained value via `operator*`.
/// `std::forward` ensures the value category (lvalue/rvalue)
/// of the callable `func` itself is preserved during the call.
template <typename T, typename F> void IfPresent(const std::optional<T> &o, F &&func) {
    if (o) {
        std::forward<F>(func)(*o);
    }
}

/// @brief OrElse returns the wrapped value if present, otherwise it calls func.
///
/// https://mariusbancila.ro/blog/2023/05/29/notes-on-std-optional-monadic-operations
template <typename T, typename F> std::optional<T> OrElse(const std::optional<T> &o, F &&func) {
    if (o) {
        return *o;
    }
    return std::forward<F>(func)();
}

} // namespace porytiles