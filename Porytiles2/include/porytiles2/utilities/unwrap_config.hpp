#pragma once

namespace porytiles2 {

/**
 * @brief Unwraps a scoped config value from a DomainConfig, AppConfig, or InfraConfig object, returning early if the
 * value is not available.
 *
 * @details
 * This macro retrieves a config value for a given tileset or layout and automatically handles the error case. If the
 * config value is not available, it returns a ChainableResult error with a formatted message. Otherwise, it unwraps the
 * value and creates a local variable with the name specified by the config parameter.
 *
 * @param ptr A pointer to a config object
 * @param config The name of the config method to call and the resulting variable name
 * @param tileset The tileset parameter to pass to the config method
 * @param return_type The template parameter for ChainableResult in case of error
 */
#define PT_UNWRAP_SCOPED_CONFIG(ptr, config, tileset, return_type)                                                     \
    auto config##_result = ptr->config(tileset);                                                                       \
    if (!config##_result.has_value()) {                                                                                \
        return ChainableResult<return_type>{                                                                           \
            FormattableError{                                                                                          \
                "failed to get config value '{}:{}'",                                                                  \
                FormatParam{tileset, Style::bold},                                                                     \
                FormatParam{#config, Style::bold}},                                                                    \
            config##_result};                                                                                          \
    }                                                                                                                  \
    auto config = std::move(config##_result).value();

} // namespace porytiles2
