#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "porytiles2/app/config/app_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/infra/config/config_provider.hpp"
#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"

namespace porytiles2 {

/**
 * @brief A Config implementation that lazily pulls a config value by consulting multiple priority-ordered backing
 * \link ConfigProvider ConfigProviders \endlink.
 *
 * @details
 * LazyLayeredConfig provides the following functionality:
 * - fetches the value from the highest priority layer, lazily (i.e., only loads upon first request, then caches)
 * - tracks the provenance of the value (e.g., did it come from tileset TOML? environment? default value?)
 * - hard panics if no value exists, this is a programmer error (programmer should at least provide a default layer)
 * - provides a way to dump itself for debugging purposes
 */
class LazyLayeredConfig final : public DomainConfig, public AppConfig, public InfraConfig {
  public:
    /**
     * @brief Constructs a LazyLayeredConfig with a list of \link ConfigProvider ConfigProviders \endlink in priority
     * order, highest to lowest.
     *
     * @details
     * The LazyLayeredConfig will attempt to resolve configuration values by traversing the provided list of \link
     * ConfigProvider ConfigProviders \endlink in order. That is, it will consult the first provider in the list first,
     * and the next provider only if the first does not supply the config value. And so on. It is the programmer's
     * responsibility to provide a default layer as the final provider in the list. If any config value resolution call
     * chain reaches the end of the provider list without finding a value, the LazyLayeredConfig will terminate with a
     * panic.
     *
     * @param providers The list of providers in priority order
     */
    explicit LazyLayeredConfig(std::vector<std::unique_ptr<ConfigProvider>> &&providers)
        : providers_{std::move(providers)} {}

    /*
     * Domain Config
     */

    [[nodiscard]] std::size_t num_tiles_primary() const override;

    [[nodiscard]] std::size_t num_tiles_total() const override;

    [[nodiscard]] std::size_t num_metatiles_primary() const override;

    [[nodiscard]] std::size_t num_metatiles_total() const override;

    [[nodiscard]] std::size_t num_pals_primary() const override;

    [[nodiscard]] std::size_t num_pals_total() const override;

    [[nodiscard]] std::size_t max_map_data_size() const override;

    [[nodiscard]] std::size_t num_tiles_per_metatile() const override;

    /*
     * App Config
     */

    [[nodiscard]] IncrementalBuildMode incremental_build_mode(const std::string &tileset_name) const override;

    /*
     * Infra Config
     */
    [[nodiscard]] TilesPalMode tiles_pal_mode(const std::string &tileset_name) const override;

    /*
     * LazyLayeredConfig Specific Functionality
     */

    /**
     * @brief Dumps the current state of the config for debugging purposes.
     *
     * @details
     * Returns a string showing each cached config key, actual value, and source layer name with metadata. Only cached
     * values are shown (values that have been requested at least once).
     *
     * @return A formatted string representation of the config state
     */
    [[nodiscard]] std::string dump() const;

    /**
     * @brief Forces all configuration values to be cached immediately for all known tilesets.
     *
     * @details
     * This function eagerly evaluates and caches all configuration values by calling each config method. This is useful
     * for warming up the cache before performance-critical operations or for validating all config values at startup.
     * The function requires a list of tileset names to evaluate tileset-specific configuration values.
     *
     * @param tileset_names List of tileset names for which to evaluate the configuration
     */
    void warmup_cache(const std::vector<std::string> &tileset_names) const;

  private:
    // Providers in priority order (highest first)
    std::vector<std::unique_ptr<ConfigProvider>> providers_;

    mutable std::unordered_map<std::string, std::string> provenance_;
    mutable std::map<std::string, std::any> cache_;
    mutable std::unordered_map<std::string, std::string> cache_value_strings_;

    /**
     * @brief Resolves config values using the common caching and provider iteration pattern.
     *
     * @tparam T The type of the config value
     * @param cache_key The key to use for caching this value
     * @param provider_call Function that calls the appropriate method on a ConfigProvider
     * @return The resolved config value
     */
    template <typename T>
    T resolve_config_value(
        const std::string &cache_key, std::function<LayerValue<T>(const ConfigProvider &)> provider_call) const;
};

} // namespace porytiles2
