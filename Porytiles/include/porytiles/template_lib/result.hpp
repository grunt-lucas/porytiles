#pragma once

#include <string>
#include <utility>

namespace porytiles {

enum class BinaryResult {
    SUCCESS,
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

    [[nodiscard]] bool HasSuccess() const {
        // Assumes first enumerator represents success
        return status_ == StatusEnum{};
    }

    template <typename F> void IfSuccess(F func) {
        if (HasSuccess()) {
            func(value_);
        }
    }
};

} // namespace porytiles