#pragma once

#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/config/incremental_build_mode.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete domain layer configuration.
 *
 * @details
 * Domain and app layer operate with this interface - they don't need to worry about implementation. Every config value
 * is either virtual (i.e., comes from the user) or defined in terms of other virtual values.
 *
 * TODO: how to handle infrastructure specific configuration? We don't want it polluting the domain interface
 */
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

    /*
     * Fieldmap Settings
     */

    [[nodiscard]] virtual std::size_t num_tiles_primary() const = 0;

    [[nodiscard]] virtual std::size_t num_tiles_total() const = 0;

    [[nodiscard]] std::size_t num_tiles_secondary() const {
        if (num_tiles_total() < num_tiles_primary()) {
            panic(fmt::format("num_tiles_total({}) < num_tiles_primary({})", num_tiles_total(), num_tiles_primary()));
        }
        return num_tiles_total() - num_tiles_primary();
    }

    [[nodiscard]] virtual std::size_t num_metatiles_primary() const = 0;

    [[nodiscard]] virtual std::size_t num_metatiles_total() const = 0;

    [[nodiscard]] std::size_t num_metatiles_secondary() const {
        if (num_tiles_total() < num_tiles_primary()) {
            panic(
                fmt::format(
                    "num_metatiles_total({}) < num_metatiles_primary({})",
                    num_metatiles_total(),
                    num_metatiles_primary()));
        }
        return num_metatiles_total() - num_metatiles_primary();
    }

    [[nodiscard]] virtual std::size_t num_pals_primary() const = 0;

    [[nodiscard]] virtual std::size_t num_pals_total() const = 0;

    [[nodiscard]] std::size_t num_pals_secondary() const {
        if (num_tiles_total() < num_tiles_primary()) {
            panic(fmt::format("num_pals_total({}) < num_pals_primary({})", num_pals_total(), num_pals_primary()));
        }
        return num_pals_total() - num_pals_primary();
    }

    [[nodiscard]] virtual std::size_t max_map_data_size() const = 0;

    [[nodiscard]] virtual std::size_t num_tiles_per_metatile() const = 0;

    /*
     * Build Settings
     */

    [[nodiscard]] virtual IncrementalBuildMode incremental_build_mode(const std::string &tileset_name) const = 0;
};

} // namespace porytiles2
