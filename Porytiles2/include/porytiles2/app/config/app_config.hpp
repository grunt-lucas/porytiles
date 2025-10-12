#pragma once

#include <string>

#include "porytiles2/app/config/incremental_build_mode.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete app layer configuration.
 *
 * @details
 * The app layer operates with this interface - it doesn't need to worry about implementation. Every config value is
 * either virtual (i.e., comes from the user) or defined in terms of other virtual values.
 */
class AppConfig {
  public:
    virtual ~AppConfig() = default;

    [[nodiscard]] virtual IncrementalBuildMode incremental_build_mode(const std::string &tileset) const = 0;
};

} // namespace porytiles2
