#pragma once

#include "porytiles2/domain/config/valueobj/incremental_build_mode.hpp"
#include "porytiles2/domain/config/valueobj/tileset_settings.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete application configuration.
 */
class Config {
  public:
    virtual ~Config() = default;

    [[nodiscard]] virtual TilesetSettings tileset_settings(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual IncrementalBuildMode incremental_build_mode() const = 0;
};

} // namespace porytiles2
