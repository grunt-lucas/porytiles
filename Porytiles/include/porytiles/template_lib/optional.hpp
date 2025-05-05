#pragma once

#include <optional>

namespace porytiles {

template <typename T, typename F> void IfPresent(const std::optional<T> &o, F func) {
    if (o) {
        func(*o);
    }
}

/// @brief OrElse returns the wrapped value if present, otherwise it calls func.
///
/// https://mariusbancila.ro/blog/2023/05/29/notes-on-std-optional-monadic-operations
template <typename T, typename F> std::optional<T> OrElse(const std::optional<T> &o, F func) {
    if (o) {
        return *o;
    }
    return func();
}

} // namespace porytiles