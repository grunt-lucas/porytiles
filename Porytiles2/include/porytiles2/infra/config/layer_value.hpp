#pragma once

#include <optional>
#include <string>
#include <vector>

namespace porytiles2 {

/**
 * @brief Represents the validation state of a configuration value from a ConfigProvider.
 *
 * @details
 * This enum distinguishes between three different states when a ConfigProvider attempts to supply a configuration
 * value:
 * - not_provided: The provider does not supply this configuration value (try next provider)
 * - valid: The provider supplies a valid configuration value (use this value)
 * - invalid: The provider attempted to supply a value, but it failed validation (stop and report error)
 */
enum class ValidationState {
    not_provided, // Provider doesn't supply this config
    valid,        // Provider supplies valid config
    invalid       // Provider found invalid config
};

/**
 * @brief A small container that holds an optional-wrapped value, validation state, and metadata about the value source.
 *
 * @details
 * LayerValue supports three states:
 * - not_provided: Provider doesn't handle this config (empty optional, no error) - continue to next provider
 * - valid: Provider supplies valid config (has value, no error) - use this value
 * - invalid: Provider attempted to supply config but it's invalid (no value, has error message) - fail immediately
 *
 * @tparam T The type of the underlying value
 */
template <typename T>
struct LayerValue {
    // TODO: add _ to end of these field names
    std::optional<T> value;
    std::string source_info;
    std::vector<std::string> source_details;
    ValidationState state = ValidationState::not_provided;
    std::string error_message;

    /**
     * @brief Creates a LayerValue representing a valid configuration value.
     *
     * @param val The valid configuration value
     * @param source_info String describing the source of this value
     * @return A LayerValue in the valid state
     */
    static LayerValue valid(T val, std::string source_info)
    {
        return LayerValue{std::move(val), std::move(source_info), {}, ValidationState::valid, ""};
    }

    /**
     * @brief Creates a LayerValue representing a valid configuration value with detailed source context.
     *
     * @param val The valid configuration value
     * @param source_info String describing the source of this value
     * @param source_details Vector of strings showing contextual lines around the source
     * @return A LayerValue in the valid state
     */
    static LayerValue valid(T val, std::string source_info, std::vector<std::string> source_details)
    {
        return LayerValue{
            std::move(val), std::move(source_info), std::move(source_details), ValidationState::valid, ""};
    }

    /**
     * @brief Creates a LayerValue representing an invalid configuration value.
     *
     * @param error Error message describing why the value is invalid
     * @param source_info String describing the source that attempted to provide this value
     * @return A LayerValue in the invalid state
     */
    static LayerValue invalid(std::string error, std::string source_info)
    {
        return LayerValue{std::nullopt, std::move(source_info), {}, ValidationState::invalid, std::move(error)};
    }

    /**
     * @brief Creates a LayerValue representing an invalid configuration value with detailed source context.
     *
     * @param error Error message describing why the value is invalid
     * @param source_info String describing the source that attempted to provide this value
     * @param source_details Vector of strings showing contextual lines around the source
     * @return A LayerValue in the invalid state
     */
    static LayerValue invalid(std::string error, std::string source_info, std::vector<std::string> source_details)
    {
        return LayerValue{
            std::nullopt,
            std::move(source_info),
            std::move(source_details),
            ValidationState::invalid,
            std::move(error)};
    }

    /**
     * @brief Creates a LayerValue representing that the provider does not supply this configuration.
     *
     * @return A LayerValue in the not_provided state
     */
    static LayerValue not_provided()
    {
        return LayerValue{std::nullopt, "", {}, ValidationState::not_provided, ""};
    }
};

} // namespace porytiles2
