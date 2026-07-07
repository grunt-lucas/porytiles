/**
 * @file
 * @brief Cross-cutting configuration validators shared across all architectural layers.
 *
 * @details
 * This file provides layer-agnostic configuration validators that can be used by DomainConfig, AppConfig, and
 * InfraConfig without introducing circular dependencies. The validators are organized into two categories:
 *
 * **Validator Categories**
 *
 * 1. **Single-Value Validators**: Validate a single config value in isolation (e.g., size_t_val_greater_than_zero)
 * 2. **Cross-Field Validators**: Validate a config value by comparing it against other config values within the same
 *    layer (e.g., compare_greater_than, compare_less_than). These validators accept a ConfigInterface and can fetch
 *    other values for comparison.
 *
 * **Design Rationale: Why xcut Layer for Common Validators?**
 *
 * The xcut (cross-cutting) layer sits above the domain, app, and infra layers in the architectural hierarchy. By
 * placing common validators here, we achieve several benefits:
 *
 * 1. **Dependency Inversion**: Lower layers (domain, app, infra) can depend on xcut without creating cycles
 * 2. **Code Reuse**: Validators that don't depend on layer-specific types can be shared across all layers
 * 3. **Separation of Concerns**: Generic validation logic is separated from domain/app/infra-specific logic
 *
 * **Layer-Specific Validators: When and Why?**
 *
 * When a validator needs to reference layer-specific types (e.g., TilesPalMode from the infra layer), it cannot be
 * defined here in xcut because that would require xcut to depend on a lower-level layer, creating a circular
 * dependency. In these cases, the validator should be defined in a layer-specific header such as
 * `app_config_validators.hpp`, `domain_config_validators.hpp`, or `infra_config_validators.hpp`.
 *
 * The config generation system supports this pattern seamlessly because validators are referenced by name as strings
 * in the YAML schema. During code generation, these validator function names are copy-pasted into the generated
 * layer config files. The generation process is agnostic to where the validators are defined - it only cares that the
 * function name matches. This allows each layer's config header to include both the common xcut validators and its
 * own layer-specific validators without any dependency issues.
 *
 *
 * @note All validators return ChainableResult<ConfigValue<T>> to support composable validation chains
 * @see config_value.hpp for the ConfigValue type
 * @see chainable_result.hpp for the ChainableResult monadic error handling type
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/config/config_value.hpp"

namespace porytiles {

/**
 * @brief Validates that a size_t config value is greater than zero.
 *
 * @details
 * This validator checks that the provided size_t config value is non-zero. If the value is zero, it returns a detailed
 * error message showing the field name and the invalid value along with its configuration source information.
 *
 * @param val The config value to validate
 * @return ChainableResult containing either the original value or a FormattableError if validation fails
 * @post If successful, the returned value is guaranteed to be greater than zero
 */
[[nodiscard]] inline ChainableResult<ConfigValue<std::size_t>>
size_t_val_greater_than_zero(const ConfigValue<std::size_t> &val)
{
    if (val == 0) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' must be greater than '{}'.");
        params.emplace_back(std::vector{FormatParam{val.canonical_name(), Style::bold}, FormatParam{"0", Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        err_text.append_range(format_text);
        params.append_range(format_params);
        return FormattableError{err_text, params};
    }
    return val;
}

/**
 * @brief Validates that a size_t config value is either 8 or 12.
 *
 * @param val The config value to validate
 * @return ChainableResult containing either the original value or a FormattableError if validation fails
 * @post If successful, the returned value is guaranteed to be greater than zero
 */
[[nodiscard]] inline ChainableResult<ConfigValue<std::size_t>>
size_t_val_eight_or_twelve(const ConfigValue<std::size_t> &val)
{
    if (val != 8 && val != 12) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' must be either '{}' or '{}'.");
        params.emplace_back(
            std::vector{
                FormatParam{val.canonical_name(), Style::bold},
                FormatParam{"8", Style::bold},
                FormatParam{"12", Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        err_text.append_range(format_text);
        params.append_range(format_params);
        return FormattableError{err_text, params};
    }
    return val;
}

namespace details {

/**
 * @brief Generic comparison validator that compares the current value against another config value.
 *
 * @details
 * This is the generic implementation for all cross-field comparison validators. It fetches another config value using
 * the provided lambda and performs a comparison using the provided comparator. If the comparison fails, returns a
 * detailed error message showing both values and their sources. This function is used internally by the public
 * comparison validator functions.
 *
 * @tparam T The type of the config values being compared (must support the comparator's operation)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @tparam Comparator Callable type that performs the comparison (e.g., std::greater<>, std::less<>)
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param type The config scope type (tileset or layout)
 * @param scope The scope name (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @param comp Comparator that performs the comparison operation
 * @param error_message The error message to display if validation fails (e.g., "must be greater than")
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc, typename Comparator>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_values(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other,
    Comparator comp,
    std::string_view error_message)
{
    auto other_result = fetch_other(config, type, scope);

    // If fetching the other value failed, propagate that error
    if (!other_result.has_value()) {
        return other_result.error();
    }

    const auto &other_val = other_result.value();

    // Perform the comparison
    if (!comp(val.value(), other_val.value())) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' {} '{}'.");
        params.emplace_back(
            std::vector{
                FormatParam{val.canonical_name(), Style::bold},
                FormatParam{std::string{error_message}, Style::none},
                FormatParam{other_field_name, Style::bold}});
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

} // namespace details

/**
 * @brief Validates that the current value is greater than another config value.
 *
 * @details
 * This cross-field validator compares the current config value against another config value. It fetches the other
 * value using the provided lambda and performs a greater-than comparison. If the comparison fails, returns a detailed
 * error message showing both values and their sources.
 *
 * @tparam T The type of the config values being compared (must support operator>)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param type The config scope type (tileset or layout)
 * @param scope The scope name (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_greater_than(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, type, scope, other_field_name, fetch_other, std::greater<>{}, "must be greater than");
}

/**
 * @brief Validates that the current value is less than another config value.
 *
 * @tparam T The type of the config values being compared (must support operator<)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param type The config scope type (tileset or layout)
 * @param scope The scope name (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_less_than(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, type, scope, other_field_name, fetch_other, std::less<>{}, "must be less than");
}

/**
 * @brief Validates that the current value is greater than or equal to another config value.
 *
 * @tparam T The type of the config values being compared (must support operator>=)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param type The config scope type (tileset or layout)
 * @param scope The scope name (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_greater_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val,
        config,
        type,
        scope,
        other_field_name,
        fetch_other,
        std::greater_equal<>{},
        "must be greater than or equal to");
}

/**
 * @brief Validates that the current value is less than or equal to another config value.
 *
 * @tparam T The type of the config values being compared (must support operator<=)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param type The config scope type (tileset or layout)
 * @param scope The scope name (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_less_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    ConfigScopeType type,
    const std::string &scope,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, type, scope, other_field_name, fetch_other, std::less_equal<>{}, "must be less than or equal to");
}

/**
 * @brief Validates that the current value is equal to another config value.
 *
 * @tparam T The type of the config values being compared (must support operator==)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param scope_param The scope parameter (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, scope_param, other_field_name, fetch_other, std::equal_to<>{}, "must be equal to");
}

/**
 * @brief Validates that the current value is not equal to another config value.
 *
 * @tparam T The type of the config values being compared (must support operator!=)
 * @tparam ConfigInterface The config interface type (DomainConfig, AppConfig, or InfraConfig)
 * @tparam FetchFunc Callable type that fetches the other config value
 * @param val The config value being validated
 * @param config The config interface to fetch other values from
 * @param scope_param The scope parameter (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_not_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, scope_param, other_field_name, fetch_other, std::not_equal_to<>{}, "must not be equal to");
}

} // namespace porytiles
