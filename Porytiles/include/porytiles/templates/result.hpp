#pragma once

#include <string>
#include <utility>

#include "../panic/panic.hpp"

namespace porytiles {

enum class BinaryStatus {
    OK,
    ERROR,
};

template <typename T, typename StatusEnum> class Result {
    T value_;
    StatusEnum status_;
    std::string message_;

  public:
    // Default construct success status
    explicit Result(T val) : value_{std::move(val)}, status_{StatusEnum{}} {}

    explicit Result(StatusEnum err, std::string msg = "") : status_{err}, message_{std::move(msg)} {}

    [[nodiscard]] bool Ok() const {
        // Assumes first enumerator represents success
        return status_ == StatusEnum{};
    }

    [[nodiscard]] const T &Get() const {
        if (status_ != StatusEnum{}) {
            Panic("Called Result.Get() when Result didn't contain OK");
        }
        return value_;
    }
};

} // namespace porytiles