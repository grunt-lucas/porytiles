/**
 * @file
 * @brief Domain configuration validators for domain-layer models.
 *
 * @note All validators return ChainableResult<ConfigValue<T>> to support composable validation chains.
 * @see xcut_config_validators.hpp for layer cross-cutting validators.
 * @see chainable_result.hpp for the ChainableResult monadic error handling type.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

namespace porytiles2 {

/**
 * @brief Validates that an Rgba32 alpha component is opaque.
 *
 * @param val The config value to validate.
 * @return ChainableResult containing either the original value or a FormattableError if validation fails.
 * @post If successful, the returned value is guaranteed to have opaque alpha value.
 */
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
        std::ranges::copy(format_text, std::back_inserter(err_text));
        std::ranges::copy(format_params, std::back_inserter(params));
        return FormattableError{err_text, params};
    }
    return val;
}

} // namespace porytiles2
