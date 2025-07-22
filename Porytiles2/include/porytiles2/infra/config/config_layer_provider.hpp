#pragma once

#include <optional>
#include <string>

#include "porytiles2/domain/config/incremental_build_mode.hpp"

namespace porytiles2 {

/**
 * @brief A small container that holds an optional-wrapped value and some metadata about the value source.
 *
 * @tparam T The type of the underlying value
 */
template <typename T>
struct LayerValue {
    std::optional<T> value;
    std::string metadata;
};

/**
 * @brief Defines an interface which Config implementations can use to load config values.
 *
 * @details
 * ConfigLayerProvider is basically just a copy of Config but with std::optional return types. It's technically a DRY
 * violation; a better solution would be to use some kind of code-gen, wherein we define the config params and both
 * Config and ConfigLayerProvider are generated from the spec.
 */
class ConfigLayerProvider {
  public:
    virtual ~ConfigLayerProvider() = default;

    /**
     * @brief Gets the name of this config layer, useful for debugging/diagnostic purposes.
     *
     * @return The name of this config layer
     */
    virtual std::string name() const = 0;

    /*
     * Fieldmap Settings
     */

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_primary(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_total(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_primary(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_total(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_primary(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_total(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> max_map_data_size() const = 0;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_per_metatile() const = 0;

    /*
     * Build Settings
     */

    [[nodiscard]] virtual LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const = 0;
};

} // namespace porytiles2
