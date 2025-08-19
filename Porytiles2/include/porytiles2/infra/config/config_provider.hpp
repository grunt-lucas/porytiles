#pragma once

#include <optional>
#include <string>

#include "../../app/config/incremental_build_mode.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"

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

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_primary() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_total() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_primary() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_metatiles_total() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_primary() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_pals_total() const;

    [[nodiscard]] virtual LayerValue<std::size_t> max_map_data_size() const;

    [[nodiscard]] virtual LayerValue<std::size_t> num_tiles_per_metatile() const;

    /*
     * App Config
     */

    [[nodiscard]] virtual LayerValue<IncrementalBuildMode>
    incremental_build_mode(const std::string &tileset_name) const;

    /*
     * Infra Config
     */
    [[nodiscard]] virtual LayerValue<TilesPalMode> tiles_pal_mode(const std::string &tileset_name) const;
};

} // namespace porytiles2
