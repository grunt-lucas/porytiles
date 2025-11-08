#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/*
 * Regular (single-value) validators
 */

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

        err_text.emplace_back("'{}' must be greater than '{}'");
        params.emplace_back(std::vector{FormatParam{val.name(), Style::bold}, FormatParam{"0", Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        std::ranges::copy(format_text, std::back_inserter(err_text));
        std::ranges::copy(format_params, std::back_inserter(params));
        return FormattableError{err_text, params};
    }
    return val;
}

/*
 * Cross-field validators
 * These validators can access other config values for validation
 */

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
 * @param scope_param The scope parameter (tileset or layout name)
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
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other,
    Comparator comp,
    std::string_view error_message)
{
    auto other_result = fetch_other(config, scope_param);

    // If fetching the other value failed, propagate that error
    if (!other_result.has_value()) {
        return other_result.error();
    }

    const auto &other_val = other_result.value();

    // Perform the comparison
    if (!comp(val.value(), other_val.value())) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("'{}' {} '{}'");
        params.emplace_back(
            std::vector{
                FormatParam{val.name(), Style::bold},
                FormatParam{std::string{error_message}, Style::none},
                FormatParam{other_field_name, Style::bold}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        std::ranges::copy(format_text, std::back_inserter(err_text));
        std::ranges::copy(format_params, std::back_inserter(params));

        err_text.emplace_back("");
        params.emplace_back();
        err_text.emplace_back("{}");
        params.emplace_back(std::vector{FormatParam{"Comparison value:", Style::italic}});
        err_text.emplace_back("");
        params.emplace_back();

        auto [other_format_text, other_format_params] = other_val.format_data();
        std::ranges::copy(other_format_text, std::back_inserter(err_text));
        std::ranges::copy(other_format_params, std::back_inserter(params));

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
 * @param scope_param The scope parameter (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_greater_than(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, scope_param, other_field_name, fetch_other, std::greater<>{}, "must be greater than");
}

/**
 * @brief Validates that the current value is less than another config value.
 *
 * @tparam T The type of the config values being compared (must support operator<)
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
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_less_than(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, scope_param, other_field_name, fetch_other, std::less<>{}, "must be less than");
}

/**
 * @brief Validates that the current value is greater than or equal to another config value.
 *
 * @tparam T The type of the config values being compared (must support operator>=)
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
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_greater_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val,
        config,
        scope_param,
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
 * @param scope_param The scope parameter (tileset or layout name)
 * @param other_field_name The name of the other field to compare against
 * @param fetch_other Callable that fetches the other config value
 * @return ChainableResult containing either the original value or an error
 */
template <typename T, typename ConfigInterface, typename FetchFunc>
[[nodiscard]] ChainableResult<ConfigValue<T>> compare_less_equal(
    const ConfigValue<T> &val,
    const ConfigInterface &config,
    const std::string &scope_param,
    const std::string &other_field_name,
    FetchFunc fetch_other)
{
    return details::compare_values(
        val, config, scope_param, other_field_name, fetch_other, std::less_equal<>{}, "must be less than or equal to");
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

} // namespace porytiles2
