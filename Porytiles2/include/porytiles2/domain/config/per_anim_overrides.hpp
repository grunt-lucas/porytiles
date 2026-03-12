#pragma once

#include <format>
#include <ostream>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/config/per_anim_override.hpp"

namespace porytiles2 {

/**
 * @brief Per-animation configuration map.
 *
 * @details
 * Maps animation names to their @c PerAnimOverride entries. When an animation's name is present in this map, the mapped
 * configuration is used instead of (or merged with) the global defaults. Animations not listed in the map fall back
 * entirely to the global settings.
 */
using PerAnimOverrides = std::unordered_map<std::string, PerAnimOverride>;

/**
 * @brief Converts an PerAnimOverride map to a human-readable string.
 *
 * @details
 * Produces a brace-enclosed, comma-separated list of animation names. An empty map produces "{}".
 *
 * @param configs The configs map to convert
 * @return A string representation of the map
 */
[[nodiscard]] inline std::string to_string(const PerAnimOverrides &configs)
{
    if (configs.empty()) {
        return "{}";
    }
    std::string result = "{";
    bool first = true;
    for (const auto &[key, value] : configs) {
        if (!first) {
            result += ", ";
        }
        result += key + "={linking=" + (value.linking.has_value() ? to_string(*value.linking) : "none") +
                  ", pal_strategy=" +
                  (value.pal_resolution_strategy.has_value() ? to_string(*value.pal_resolution_strategy) : "none") +
                  ", per_tile_strategies=" + std::to_string(value.per_tile_pal_resolution_strategies.size()) + "}";
        first = false;
    }
    result += "}";
    return result;
}

/**
 * @brief Stream insertion operator for AnimConfigs.
 *
 * @param os The output stream
 * @param configs The configs map to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const PerAnimOverrides &configs)
{
    return os << to_string(configs);
}

} // namespace porytiles2

template <>
struct std::formatter<porytiles2::PerAnimOverrides> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::PerAnimOverrides &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
