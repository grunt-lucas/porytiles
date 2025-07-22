#pragma once

#include "porytiles2/domain/config/config.hpp"

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
};

} // namespace porytiles2
