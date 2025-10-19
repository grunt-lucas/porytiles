#pragma once

#include <optional>
#include <string>

#include "porytiles2/app/config/incremental_build_mode.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"

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
    std::optional<T> value;
    std::string source_info;
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
        return LayerValue{std::move(val), std::move(source_info), ValidationState::valid, ""};
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
        return LayerValue{std::nullopt, std::move(source_info), ValidationState::invalid, std::move(error)};
    }

    /**
     * @brief Creates a LayerValue representing that the provider does not supply this configuration.
     *
     * @return A LayerValue in the not_provided state
     */
    static LayerValue not_provided()
    {
        return LayerValue{std::nullopt, "", ValidationState::not_provided, ""};
    }
};

/**
 * @brief An interface which config implementations can use to load config values.
 *
 * @details
 * ConfigProvider is basically just a copy of all three layer configs (domain, app, infra) but with LayerValue return
 * types. It's technically a DRY violation; a better solution would be to use some kind of code-gen, wherein the config
 * params are defined in a common spec and both the layer configs and ConfigProvider are generated from the spec.
 *
 * ConfigProvider provides a default implementation for each method which returns an empty LayerValue. This is helpful
 * for ConfigProvider implementations, since often the implementations may not want to provide a value for every config
 * param.
 */
class ConfigProvider {
  public:
    virtual ~ConfigProvider() = default;

    /**
     * @brief Gets the name of this ConfigProvider, useful for diagnostic purposes.
     *
     * @return The name of this ConfigProvider
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*
     * Domain Config
     */

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_primary(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_total(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_primary(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_total(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_primary(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_total(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> max_map_data_size(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_per_metatile(const std::string &tileset) const;

    [[nodiscard]] virtual LayerValue<Rgba32> extrinsic_transparency(const std::string &tileset) const;

    /*
     * App Config
     */

    [[nodiscard]] virtual LayerValue<IncrementalBuildMode> incremental_build_mode(const std::string &tileset) const;

    /*
     * Infra Config
     */
    [[nodiscard]] virtual LayerValue<TilesPalMode> tiles_pal_mode(const std::string &tileset) const;
};

} // namespace porytiles2
