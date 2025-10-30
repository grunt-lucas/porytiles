#pragma once

#include "porytiles2/infra/config/config_provider.hpp"

namespace porytiles2 {

/**
 * @brief A default implementation of ConfigProvider that provides sensible default values.
 *
 * @details
 * This provider returns default values for all configuration parameters. It's useful as a base layer in a configuration
 * system where other providers can override specific values.
 */
class DefaultProvider final : public ConfigProvider {
  public:
    /**
     * @brief Gets the name of this config layer.
     *
     * @return The name "DefaultProvider"
     */
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] LayerValue<std::size_t> num_tiles_primary(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_tiles_total(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_primary(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_metatiles_total(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_pals_primary(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_pals_total(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> max_map_data_size(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<std::size_t> num_tiles_per_metatile(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<Rgba32> extrinsic_transparency(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<bool> patch_build_enabled(const std::string &tileset) const override;

    [[nodiscard]] LayerValue<TilesPalMode> tiles_pal_mode(const std::string &tileset) const override;
};

} // namespace porytiles2