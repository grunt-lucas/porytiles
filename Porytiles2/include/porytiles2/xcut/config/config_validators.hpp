#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

[[nodiscard]] inline ChainableResult<ConfigValue<std::size_t>>
size_t_val_greater_than_zero(const ConfigValue<std::size_t> &val)
{
    if (val == 0) {
        auto [err_text, params] = val.format_data();
        return FormattableError{err_text, params};
    }
    return val;
}

} // namespace porytiles2
