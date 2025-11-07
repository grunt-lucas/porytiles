#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

[[nodiscard]] inline ChainableResult<ConfigValue<std::size_t>> size_t_val_greater_than_zero(const ConfigValue<std::size_t> &val)
{
    if (val == 0) {
        std::vector err_text = {
            std::string{"{} {}"},
            std::string{"{} {}"}
        };

        std::vector<std::vector<FormatParam>> params = {
            std::vector{
                FormatParam{"1", Style::bold | Style::red},
                FormatParam{"2", Style::italic | Style::blue}
            },
            std::vector{
                FormatParam{"a", Style::bold | Style::red},
                FormatParam{"b", Style::italic | Style::blue}
            }
        };

        return FormattableError{err_text, params};
    }
    return val;
}

} // namespace porytiles2
