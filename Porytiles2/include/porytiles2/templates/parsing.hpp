#pragma once

#include <expected>
#include <string>

namespace porytiles2 {

// ReSharper disable once CppParameterMayBeConst
template <typename T>
std::expected<T, std::string> parse_int(std::string_view int_string, const int base)
{
    T arg;
    std::size_t pos;

    try {
        arg = std::stoi(int_string.data(), &pos, base);
    }
    catch (const std::exception &e) {
        return std::unexpected{"invalid integral string: " + std::string{int_string}};
    }
    if (std::string{int_string}.size() != pos) {
        return std::unexpected{"invalid integral string: " + std::string{int_string}};
    }

    return arg;
}

// ReSharper disable once CppParameterMayBeConst
template <typename T>
std::expected<T, std::string> parse_int(std::string_view int_string)
{
    return parse_int<T>(int_string, 0);
}

} // namespace porytiles2