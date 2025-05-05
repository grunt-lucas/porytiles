#pragma once

/// The Porytiles Template Library is a small extension of the STL with some
/// additional utility functionality that Porytiles uses in multiple places.

#include <optional>

namespace porytiles {
template <typename T, typename F> void if_present(const std::optional<T> &o, F f) {
    if (o) {
        f(*o);
    }
}
} // namespace porytiles