#pragma once

#include <format>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Controls how tile orderings are generated during multi-start packing.
 *
 * @details
 * The packing algorithm can attempt multiple tile orderings to find a valid solution. This enum
 * controls the strategy used to generate those orderings after the initial FFD (First Fit Decreasing)
 * attempt.
 */
enum class ShuffleStrategy {
    /**
     * @brief One FFD attempt only, no multi-start retries.
     */
    single_ffd,
    /**
     * @brief FFD first, then perturbed FFD orderings that preserve the large-first property.
     */
    noisy_ffd,
    /**
     * @brief FFD first, then fully random shuffles (original behavior).
     */
    random
};

/**
 * @brief Converts a ShuffleStrategy to its canonical string representation.
 *
 * @param s The value to convert
 * @return The canonical snake_case string representation
 */
[[nodiscard]] inline std::string to_string(const ShuffleStrategy s)
{
    switch (s) {
    case ShuffleStrategy::single_ffd:
        return "single_ffd";
    case ShuffleStrategy::noisy_ffd:
        return "noisy_ffd";
    case ShuffleStrategy::random:
        return "random";
    }
    panic("unhandled ShuffleStrategy value");
}

/**
 * @brief Stream insertion operator for ShuffleStrategy.
 *
 * @param os The output stream
 * @param s The value to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const ShuffleStrategy s)
{
    return os << to_string(s);
}

} // namespace porytiles2

template <>
struct std::formatter<porytiles2::ShuffleStrategy> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::ShuffleStrategy &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
