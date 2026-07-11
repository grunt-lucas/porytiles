/// @file
/// @brief Domain configuration validators for domain-layer models.
///
/// @note All validators return ChainableResult<ConfigValue<T>> to support composable validation chains.
/// @see xcut_config_validators.hpp for layer cross-cutting validators.
/// @see chainable_result.hpp for the ChainableResult monadic error handling type.

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "porytiles/domain/config/packing_strategy_type.hpp"
#include "porytiles/domain/config/tile_sharing_packing.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/config/config_value.hpp"

namespace porytiles {

/// @brief Validates that an Rgba32 alpha component is opaque.
///
/// @param val The config value to validate.
/// @return ChainableResult containing either the original value or a FormattableError if validation fails.
/// @post If successful, the returned value is guaranteed to have opaque alpha value.
[[nodiscard]] inline ChainableResult<ConfigValue<Rgba32>> rgba_alpha_component_opaque(const ConfigValue<Rgba32> &val)
{
    if (val.value().alpha() != Rgba32::alpha_opaque) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' alpha component '{}' is not opaque ('{}').");
        params.emplace_back(
            std::vector{
                FormatParam{val.canonical_name(), Style::bold},
                FormatParam{std::to_string(val.value().alpha()), Style::bold},
                FormatParam{std::to_string(Rgba32::alpha_opaque), Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        err_text.append_range(format_text);
        params.append_range(format_params);
        return FormattableError{err_text, params};
    }
    return val;
}

/// @brief Validates that 'biased' or 'optimal' tile sharing packing requires 'backtracking' packing strategy.
///
/// @details
/// When tile sharing packing is set to @c TileSharingPacking::biased or @c TileSharingPacking::optimal, the packing
/// strategy must be @c PackingStrategyType::backtracking. If tile sharing packing is @c TileSharingPacking::off, no
/// constraint is enforced. This is a cross-field validator that fetches the packing strategy from the config interface.
///
/// @tparam T The type of the config value (expected to be TileSharingPacking)
/// @tparam ConfigInterface The config interface type (e.g., DomainConfig)
/// @tparam FetchFunc Callable type that fetches the packing strategy config value
/// @param val The tile sharing packing config value being validated
/// @param config The config interface to fetch the packing strategy from
/// @param type The config scope type (tileset or layout)
/// @param scope The scope name (tileset or layout name)
/// @param other_field_name The name of the packing strategy field
/// @param fetch_other Callable that fetches the packing strategy config value
/// @return ChainableResult containing either the original value or an error
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> require_packing_strategy_backtracking(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    if (val.value() == TileSharingPacking::off) {
        return val;
    }

    auto other_result = fetch_other(config, type, scope);

    if (!other_result.has_value()) {
        return other_result.error();
    }

    const auto &other_val = other_result.value();

    if (other_val.value() != PackingStrategyType::backtracking) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' set to '{}' requires '{}' to be '{}'.");
        params.emplace_back(
            std::vector{
                FormatParam{val.canonical_name(), Style::bold},
                FormatParam{to_string(val.value()), Style::bold},
                FormatParam{other_field_name, Style::bold},
                FormatParam{to_string(PackingStrategyType::backtracking), Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        err_text.append_range(format_text);
        params.append_range(format_params);

        err_text.emplace_back("");
        params.emplace_back();
        err_text.emplace_back("{}");
        params.emplace_back(std::vector{FormatParam{"Comparison value:", Style::italic}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [other_format_text, other_format_params] = other_val.format_data();
        err_text.append_range(other_format_text);
        params.append_range(other_format_params);

        return FormattableError{err_text, params};
    }

    return val;
}

} // namespace porytiles
