#pragma once

#include <optional>
#include <string>

#include "yaml-cpp/yaml.h"

#include "porytiles2/infra/config/config_provider.hpp"

namespace porytiles2 {

/**
 * @brief A ConfigProvider implementation that reads configuration values from a YAML file.
 *
 * @details
 * YamlFileProvider loads a YAML configuration file and provides access to configuration values defined within it. If
 * the YAML file does not exist or cannot be loaded, all configuration methods return LayerValue::not_provided(). This
 * allows the provider to gracefully fall back to other providers in a layered configuration system.
 */
class YamlFileProvider final : public ConfigProvider {
  public:
    /**
     * @brief Constructs a YamlFileProvider that loads configuration from the specified YAML file.
     *
     * @details
     * If the file does not exist or cannot be parsed, the provider will still be constructed successfully, but all
     * configuration methods will return LayerValue::not_provided().
     *
     * @param yaml_file_path Path to the YAML configuration file
     */
    explicit YamlFileProvider(const std::string &yaml_file_path);

    /**
     * @brief Gets the name of this config layer.
     *
     * @return The name "YamlFileProvider"
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

  private:
    std::optional<YAML::Node> yaml_doc_;
    std::string file_path_;
};

} // namespace porytiles2
