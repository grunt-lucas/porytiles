#pragma once

#include <vector>

#include "porytiles2/domain/config/config.hpp"
#include "porytiles2/infra/config/config_layer_provider.hpp"

namespace porytiles2 {

/**
 * @brief A Config implementation that lazily pulls a config value by consulting multiple priority-ordered backing
 * sources.
 *
 * @details
 * LazyLayeredConfig provides the following functionality:
 * - fetches the value from the highest priority layer, lazily (i.e., only loads upon first request, then caches)
 * - tracks the provenance of the value (e.g., did it come from tileset TOML? environment? default value?)
 * - hard panics if no value is found, this is a programmer error (programmer should at least provide a default layer)
 * - provides a way to dump itself for debugging purposes
 */
class LazyLayeredConfig final : public Config {
  public:
    /**
     * @brief Constructs a LazyLayeredConfig with a list of ConfigLayerProviders in priority order, highest to lowest.
     *
     * @details
     * The LazyLayeredConfig will attempt to resolve configuration values by traversing the provided list of
     * ConfigLayerProviders in order. That is, the first provider in the list will be consulted first, and the next
     * provider will only be consulted if the first does not supply the config value. And so on. It is the programmer's
     * responsibility to provide a default layer as the final provider in the list. If any config value resolution call
     * chain reaches the end of the provider list without finding a value, the LazyLayeredConfig will terminate with a
     * panic.
     *
     * @param providers The list of providers in priority order
     */
    explicit LazyLayeredConfig(std::vector<std::unique_ptr<ConfigLayerProvider>> &&providers)
        : providers_{std::move(providers)} {}

    /**
     * @brief foo
     *
     * @tparam Args bar
     * @param args baz
     * @return bat
     */
    template <typename... Args>
    static LazyLayeredConfig create(Args &&...args) {
        static_assert((std::is_base_of_v<ConfigLayerProvider, Args> && ...));
        std::vector<std::unique_ptr<ConfigLayerProvider>> providers;
        providers.reserve(sizeof...(args));
        (providers.push_back(std::forward<Args>(args)), ...);
        return LazyLayeredConfig{std::move(providers)};
    }

    /*
     * Fieldmap Settings
     */

    [[nodiscard]] std::size_t num_tiles_primary(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t num_tiles_total(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t num_metatiles_primary(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t num_metatiles_total(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t num_pals_primary(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t num_pals_total(const std::string &tileset_name) const override;

    [[nodiscard]] std::size_t max_map_data_size() const override;

    [[nodiscard]] std::size_t num_tiles_per_metatile() const override;

    /*
     * Build Settings
     */

    [[nodiscard]] IncrementalBuildMode incremental_build_mode(const std::string &tileset_name) const override;

  private:
    // Providers in priority order (highest first)
    std::vector<std::unique_ptr<ConfigLayerProvider>> providers_;
};

} // namespace porytiles2
