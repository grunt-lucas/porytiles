#pragma once

#include <string>

#include "porytiles2/app/config/incremental_build_mode.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

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
};

} // namespace porytiles2
