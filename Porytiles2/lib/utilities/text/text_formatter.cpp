#include "porytiles2/utilities/text/text_formatter.hpp"

#include <string>
#include <vector>

#include "fmt/args.h"
#include "fmt/format.h"

namespace porytiles2 {

std::string TextFormatter::format(const std::string &format_str, const std::vector<FormatParam> &params) const
{
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &[text, styles] : params) {
        store.push_back(style(text, styles));
    }
    return fmt::vformat(format_str, store);
}

} // namespace porytiles2
