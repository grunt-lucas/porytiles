#pragma once

#include <string>

#include "porytiles2/infra/config/layered_config.hpp"

namespace porytiles2 {

class ConfigSource {
  public:
    virtual ~ConfigSource() = default;

    [[nodiscard]] virtual LayeredConfig read_config() const = 0;

    /**
     * @brief Gets the name of this config source; useful for debugging.
     *
     * @return The name of this ConfigSource
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

} // namespace porytiles2
