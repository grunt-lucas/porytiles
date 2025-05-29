#pragma once

#include <expected>
#include <string>

namespace porytiles {

// ReSharper disable once CppParameterMayBeConst
template <typename T> std::expected<T, std::string> ParseInt(std::string_view int_string, const int base) {
    T arg;
    std::size_t pos;

    try {
        arg = std::stoi(int_string.data(), &pos, base);
    } catch (const std::exception &e) {
        return std::expected<T, std::string>{"invalid integral string: " + std::string{int_string}};
    }
    if (std::string{int_string}.size() != pos) {
        return std::expected<T, std::string>{"invalid integral string: " + std::string{int_string}};
    }

    return std::expected<T, std::string>{arg};
}

// ReSharper disable once CppParameterMayBeConst
template <typename T> std::expected<T, std::string> ParseInt(std::string_view int_string) {
    return ParseInt<T>(int_string, 0);
}

} // namespace porytiles