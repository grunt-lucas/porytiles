#pragma once

#include <format>
#include <ostream>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"

namespace porytiles2 {

/**
 * @brief Per-animation palette resolution strategy overrides.
 *
 * @details
 * Maps animation names to their specific @c AnimPalResolutionStrategy values. When an animation's name is present in
 * this map, the mapped strategy is used instead of the global fallback strategy. Animations not listed in the map fall
 * back to the global @c AnimPalResolutionStrategy setting.
 */
using AnimPalResolutionStrategyOverrides = std::unordered_map<std::string, AnimPalResolutionStrategy>;

/**
 * @brief Converts an AnimPalResolutionStrategyOverrides map to a human-readable string.
 *
 * @details
 * Produces a brace-enclosed, comma-separated list of key=value pairs. Each value is converted via the
 * @c AnimPalResolutionStrategy @c to_string() overload. An empty map produces "{}".
 *
 * @param overrides The overrides map to convert
 * @return A string representation of the map
 */
[[nodiscard]] inline std::string to_string(const AnimPalResolutionStrategyOverrides &overrides)
{
    if (overrides.empty()) {
        return "{}";
    }
    std::string result = "{";
    bool first = true;
    for (const auto &[key, value] : overrides) {
        if (!first) {
            result += ", ";
        }
        result += key + "=" + to_string(value);
        first = false;
    }
    result += "}";
    return result;
}

/**
 * @brief Stream insertion operator for AnimPalResolutionStrategyOverrides.
 *
 * @param os The output stream
 * @param overrides The overrides map to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const AnimPalResolutionStrategyOverrides &overrides)
{
    return os << to_string(overrides);
}

} // namespace porytiles2

template <>
struct std::formatter<porytiles2::AnimPalResolutionStrategyOverrides> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::AnimPalResolutionStrategyOverrides &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
