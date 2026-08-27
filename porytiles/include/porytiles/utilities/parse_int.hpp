#pragma once

#include <expected>
#include <string>
#include <utility>

namespace porytiles {

// ReSharper disable once CppParameterMayBeConst
template <typename T>
std::expected<T, std::string> parse_int(std::string_view int_string, const int base)
{
    // Copy into a std::string: stoll needs a null-terminated buffer, and a string_view's data() carries no such
    // guarantee.
    const std::string buffer{int_string};
    long long parsed;
    std::size_t pos;

    try {
        parsed = std::stoll(buffer, &pos, base);
    }
    catch (const std::exception &) {
        return std::unexpected{"invalid integral string: " + buffer};
    }

    if (buffer.size() != pos) {
        return std::unexpected{"invalid integral string: " + buffer};
    }

    if (!std::in_range<T>(parsed)) {
        return std::unexpected{"integral value out of range: " + buffer};
    }

    return static_cast<T>(parsed);
}

// ReSharper disable once CppParameterMayBeConst
template <typename T>
std::expected<T, std::string> parse_int(std::string_view int_string)
{
    return parse_int<T>(int_string, 0);
}

} // namespace porytiles
