#pragma once

#include <string>

#include "./result.hpp"

namespace porytiles {

// ReSharper disable once CppParameterMayBeConst
template <typename T> Result<T, BinaryStatus> ParseInt(std::string_view int_string, const int base) {
    T arg;
    std::size_t pos;

    try {
        arg = std::stoi(int_string.data(), &pos, base);
    } catch (const std::exception &e) {
        return Result<T, BinaryStatus>{BinaryStatus::ERROR, "invalid integral string: " + std::string{int_string}};
    }
    if (std::string{int_string}.size() != pos) {
        return Result<T, BinaryStatus>{BinaryStatus::ERROR, "invalid integral string: " + std::string{int_string}};
    }

    return Result<T, BinaryStatus>{arg};
}

// ReSharper disable once CppParameterMayBeConst
template <typename T> Result<T, BinaryStatus> ParseInt(std::string_view int_string) {
    return ParseInt<T>(int_string, 0);
}

} // namespace porytiles