#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

[[nodiscard]] inline ChainableResult<ConfigValue<std::size_t>>
size_t_val_greater_than_zero(const ConfigValue<std::size_t> &val)
{
    if (val == 0) {
        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("value must be greater than zero");
        params.emplace_back();
        err_text.emplace_back("");
        params.emplace_back();

        auto [format_text, format_params] = val.format_data();
        std::ranges::copy(format_text, std::back_inserter(err_text));
        std::ranges::copy(format_params, std::back_inserter(params));
        return FormattableError{err_text, params};
    }
    return val;
}

} // namespace porytiles2
