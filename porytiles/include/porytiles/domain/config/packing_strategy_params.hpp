#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <ostream>
#include <string>

#include "porytiles/domain/config/search_algorithm.hpp"
#include "porytiles/domain/config/shuffle_strategy.hpp"
#include "porytiles/xcut/config/config_pod_field.hpp"

namespace porytiles {

/// @brief Per-strategy parameters for the BacktrackingStrategy packing algorithm.
///
/// @details
/// When any field has a value, the strategy runs in single-config mode with the provided parameters (unset fields fall
/// back to their defaults). When no fields have values, the strategy uses its built-in preset matrix mode.
struct BacktrackingParams {
    ConfigPODField<SearchAlgorithm> search_algorithm;
    ConfigPODField<std::size_t> node_cutoff;
    ConfigPODField<std::size_t> best_branches;
    ConfigPODField<bool> smart_prune;

    /// @brief Checks whether any parameter has been explicitly set.
    ///
    /// @return @c true if at least one field has a value
    [[nodiscard]] bool has_any() const
    {
        return search_algorithm.has_value() || node_cutoff.has_value() || best_branches.has_value() ||
               smart_prune.has_value();
    }
};

/// @brief Per-strategy parameters for the OverloadAndRemoveStrategy packing algorithm.
///
/// @details
/// When any field has a value, the strategy runs in single-config mode with the provided parameters (unset fields fall
/// back to their defaults). When no fields have values, the strategy uses its built-in preset matrix mode.
struct OverloadAndRemoveParams {
    ConfigPODField<std::size_t> max_attempts;
    ConfigPODField<std::uint64_t> seed;
    ConfigPODField<ShuffleStrategy> shuffle_strategy;

    /// @brief Checks whether any parameter has been explicitly set.
    ///
    /// @return @c true if at least one field has a value
    [[nodiscard]] bool has_any() const
    {
        return max_attempts.has_value() || seed.has_value() || shuffle_strategy.has_value();
    }
};

/// @brief Container for per-strategy packing parameters.
///
/// @details
/// Groups the parameter blocks for all configurable packing strategies. Each strategy's parameter block is independent;
/// only the block matching the selected @c PackingStrategyType is consulted at runtime. If the matching block has no
/// fields set (@c has_any() returns @c false), the strategy uses its built-in preset matrix mode.
struct PackingStrategyParams {
    BacktrackingParams backtracking;
    OverloadAndRemoveParams overload_and_remove;
};

/// @brief Converts a PackingStrategyParams to a human-readable string.
///
/// @param params The params to convert
/// @return A string representation of the params
[[nodiscard]] inline std::string to_string(const PackingStrategyParams &params)
{
    std::string result = "{backtracking={";
    if (params.backtracking.has_any()) {
        bool first = true;
        if (params.backtracking.search_algorithm.has_value()) {
            result += "search_algorithm=" + to_string(*params.backtracking.search_algorithm);
            first = false;
        }
        if (params.backtracking.node_cutoff.has_value()) {
            if (!first) {
                result += ", ";
            }
            result += "node_cutoff=" + std::to_string(*params.backtracking.node_cutoff);
            first = false;
        }
        if (params.backtracking.best_branches.has_value()) {
            if (!first) {
                result += ", ";
            }
            result += "best_branches=" + std::to_string(*params.backtracking.best_branches);
            first = false;
        }
        if (params.backtracking.smart_prune.has_value()) {
            if (!first) {
                result += ", ";
            }
            result += "smart_prune=";
            result += (*params.backtracking.smart_prune ? "true" : "false");
        }
    }
    result += "}, overload_and_remove={";
    if (params.overload_and_remove.has_any()) {
        bool first = true;
        if (params.overload_and_remove.max_attempts.has_value()) {
            result += "max_attempts=" + std::to_string(*params.overload_and_remove.max_attempts);
            first = false;
        }
        if (params.overload_and_remove.seed.has_value()) {
            if (!first) {
                result += ", ";
            }
            result += "seed=" + std::to_string(*params.overload_and_remove.seed);
            first = false;
        }
        if (params.overload_and_remove.shuffle_strategy.has_value()) {
            if (!first) {
                result += ", ";
            }
            result += "shuffle_strategy=" + to_string(*params.overload_and_remove.shuffle_strategy);
        }
    }
    result += "}}";
    return result;
}

/// @brief Stream insertion operator for PackingStrategyParams.
///
/// @param os The output stream
/// @param params The params to output
/// @return Reference to the output stream
inline std::ostream &operator<<(std::ostream &os, const PackingStrategyParams &params)
{
    return os << to_string(params);
}

} // namespace porytiles

template <>
struct std::formatter<porytiles::PackingStrategyParams> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::PackingStrategyParams &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
