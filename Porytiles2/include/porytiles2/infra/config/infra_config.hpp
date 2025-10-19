#pragma once

#include <string>

#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete infra layer configuration.
 *
 * @details
 * The infra layer operates with this interface - it doesn't need to worry about implementation. Every config value is
 * either virtual (i.e., comes from the user) or defined in terms of other virtual values.
 */
class InfraConfig {
  public:
    virtual ~InfraConfig() = default;

    [[nodiscard]] virtual ChainableResult<ConfigValue<TilesPalMode>>
    tiles_pal_mode(const std::string &tileset) const = 0;
};

} // namespace porytiles2
