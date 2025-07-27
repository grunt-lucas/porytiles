#pragma once

#include "porytiles2/infra/config/config_layer_provider.hpp"

namespace porytiles2 {

/**
 * @brief A default implementation of ConfigProvider that provides sensible default values.
 *
 * @details
 * This provider returns default values for all configuration parameters. It's useful as a base layer in a configuration
 * system where other providers can override specific values.
 */
class DefaultProvider : public ConfigProvider {
  public:
    /**
     * @brief Gets the name of this config layer.
     *
     * @return The name "DefaultProvider"
     */
    [[nodiscard]] std::string name() const override;

    /*
     * Fieldmap Settings
     */

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary() const override;

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total() const override;

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_primary() const override;

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_total() const override;

    [[nodiscard]] LayerValue<std::size_t> num_pals_primary() const override;

    [[nodiscard]] LayerValue<std::size_t> num_pals_total() const override;

    [[nodiscard]] LayerValue<std::size_t> max_map_data_size() const override;

    [[nodiscard]] LayerValue<std::size_t> num_tiles_per_metatile() const override;

    /*
     * Build Settings
     */

    [[nodiscard]] LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const override;
};

} // namespace porytiles2