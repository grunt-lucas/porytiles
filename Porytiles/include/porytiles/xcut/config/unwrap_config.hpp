#pragma once

#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

/**
 * @brief Unwraps a tileset-scoped config value via pointer access, returning early if the value is not available.
 *
 * @details
 * This macro retrieves a config value for a given tileset using pointer syntax (ptr->config) and automatically handles
 * the error case. If the config value is not available, it returns a ChainableResult error with a formatted message.
 * Otherwise, it unwraps the value and creates a local variable with the name specified by the config parameter.
 *
 * @param ptr A pointer to a DomainConfig, AppConfig, or InfraConfig object
 * @param config The name of the config method to call and the resulting variable name
 * @param tileset_name The tileset name to pass to the config method
 * @param return_type The template parameter for ChainableResult in case of error
 *
 * @see PT_UNWRAP_TILESET_CONFIG_REF for reference-based access
 */
#define PT_UNWRAP_TILESET_CONFIG_PTR(ptr, config, tileset_name, return_type)                                           \
    auto config##_result = ptr->config(ConfigScopeType::tileset, tileset_name);                                        \
    if (!config##_result.has_value()) {                                                                                \
        return ChainableResult<return_type>{                                                                           \
            FormattableError{                                                                                          \
                "Failed to get config value '{}:{}:{}'.",                                                              \
                FormatParam{"tileset", Style::bold},                                                                   \
                FormatParam{tileset_name, Style::bold},                                                                \
                FormatParam{#config, Style::bold}},                                                                    \
            config##_result};                                                                                          \
    }                                                                                                                  \
    auto config = std::move(config##_result).value();

/**
 * @brief Unwraps a tileset-scoped config value via reference access, returning early if the value is not available.
 *
 * @details
 * This macro retrieves a config value for a given tileset using reference syntax (ref.config) and automatically handles
 * the error case. If the config value is not available, it returns a ChainableResult error with a formatted message.
 * Otherwise, it unwraps the value and creates a local variable with the name specified by the config parameter.
 *
 * @param ref A reference to a DomainConfig, AppConfig, or InfraConfig object
 * @param config The name of the config method to call and the resulting variable name
 * @param tileset_name The tileset name to pass to the config method
 * @param return_type The template parameter for ChainableResult in case of error
 *
 * @see PT_UNWRAP_TILESET_CONFIG_PTR for pointer-based access
 */
#define PT_UNWRAP_TILESET_CONFIG_REF(ref, config, tileset_name, return_type)                                           \
    auto config##_result = ref.config(ConfigScopeType::tileset, tileset_name);                                         \
    if (!config##_result.has_value()) {                                                                                \
        return ChainableResult<return_type>{                                                                           \
            FormattableError{                                                                                          \
                "Failed to get config value '{}:{}:{}'.",                                                              \
                FormatParam{"tileset", Style::bold},                                                                   \
                FormatParam{tileset_name, Style::bold},                                                                \
                FormatParam{#config, Style::bold}},                                                                    \
            config##_result};                                                                                          \
    }                                                                                                                  \
    auto config = std::move(config##_result).value();

/**
 * @brief Unwraps a layout-scoped config value from a DomainConfig, AppConfig, or InfraConfig object, returning early if
 * the value is not available.
 *
 * @details
 * This macro retrieves a config value for a given layout and automatically handles the error case. If the config value
 * is not available, it returns a ChainableResult error with a formatted message. Otherwise, it unwraps the value and
 * creates a local variable with the name specified by the config parameter.
 *
 * @param ptr A pointer to a config object
 * @param config The name of the config method to call and the resulting variable name
 * @param layout_name The layout name to pass to the config method
 * @param return_type The template parameter for ChainableResult in case of error
 */
#define PT_UNWRAP_LAYOUT_CONFIG(ptr, config, layout_name, return_type)                                                 \
    auto config##_result = ptr->config(ConfigScopeType::layout, layout_name);                                          \
    if (!config##_result.has_value()) {                                                                                \
        return ChainableResult<return_type>{                                                                           \
            FormattableError{                                                                                          \
                "Failed to get config value '{}:{}:{}'.",                                                              \
                FormatParam{"layout", Style::bold},                                                                    \
                FormatParam{layout_name, Style::bold},                                                                 \
                FormatParam{#config, Style::bold}},                                                                    \
            config##_result};                                                                                          \
    }                                                                                                                  \
    auto config = std::move(config##_result).value();

} // namespace porytiles
