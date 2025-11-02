#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/config/config_provider.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A ConfigProvider implementation that reads configuration values from multiple YAML files with priority.
 *
 * @details
 * YamlFileProvider loads YAML configuration files from multiple locations and provides access to configuration values
 * defined within them. Config files are searched in priority order:
 * 1. tileset_folder/config.local.yaml (highest priority)
 * 2. tileset_folder/config.yaml
 * 3. project_root/config.local.yaml
 * 4. project_root/config.yaml (lowest priority)
 *
 * Files are loaded lazily and cached for performance. If no config files exist or a key is not found, methods return
 * LayerValue::not_provided(), allowing graceful fallback to other providers in a layered configuration system.
 */
class YamlFileProvider final : public ConfigProvider {
  public:
    /**
     * @brief Constructs a YamlFileProvider that searches for configuration across multiple YAML files.
     *
     * @details
     * This constructor sets up the provider to search for configuration values across multiple config files in priority
     * order. Config files are loaded lazily when first accessed and cached for subsequent lookups.
     *
     * @param format A pointer to the TextFormatter to use
     * @param project_root The root directory of the project
     * @param tileset_key_provider Provider for generating tileset artifact keys and paths
     */
    explicit YamlFileProvider(
        gsl::not_null<TextFormatter *> format,
        const std::filesystem::path &project_root,
        const TilesetArtifactKeyProvider &tileset_key_provider);

    /**
     * @brief Constructs a YamlFileProvider with a default PlainTextFormatter.
     *
     * @details
     * This constructor creates an internally owned PlainTextFormatter instance and uses it for formatting. The
     * YamlFileProvider will search for configuration values across multiple config files in priority order. Config
     * files are loaded lazily when first accessed and cached for subsequent lookups.
     *
     * @param project_root The root directory of the project
     * @param tileset_key_provider Provider for generating tileset artifact keys and paths
     */
    explicit YamlFileProvider(
        const std::filesystem::path &project_root, const TilesetArtifactKeyProvider &tileset_key_provider);

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
    std::unique_ptr<TextFormatter> owned_format_; // Optional owned formatter (when using default ctor)
    TextFormatter *format_;                       // Non-owning pointer to formatter
    std::filesystem::path project_root_;
    const TilesetArtifactKeyProvider *tileset_key_provider_;
};

} // namespace porytiles2
