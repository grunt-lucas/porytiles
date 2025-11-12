#include "porytiles2/utilities/text/text_formatter.hpp"

#include <string>
#include <vector>

#include "fmt/args.h"
#include "fmt/format.h"

namespace porytiles2 {

// Style static constants - Formatting
const Style Style::none{};
const Style Style::bold{Style::FormatFlagTag::bold};
const Style Style::italic{Style::FormatFlagTag::italic};

// Style static constants - Foreground colors
const Style Style::black{Style::FgColorTag::predefined, PredefinedColor::black};
const Style Style::red{Style::FgColorTag::predefined, PredefinedColor::red};
const Style Style::green{Style::FgColorTag::predefined, PredefinedColor::green};
const Style Style::yellow{Style::FgColorTag::predefined, PredefinedColor::yellow};
const Style Style::blue{Style::FgColorTag::predefined, PredefinedColor::blue};
const Style Style::magenta{Style::FgColorTag::predefined, PredefinedColor::magenta};
const Style Style::cyan{Style::FgColorTag::predefined, PredefinedColor::cyan};
const Style Style::white{Style::FgColorTag::predefined, PredefinedColor::white};

// Style static constants - Background colors
const Style Style::bg_black{Style::BgColorTag::predefined, PredefinedColor::black};
const Style Style::bg_red{Style::BgColorTag::predefined, PredefinedColor::red};
const Style Style::bg_green{Style::BgColorTag::predefined, PredefinedColor::green};
const Style Style::bg_yellow{Style::BgColorTag::predefined, PredefinedColor::yellow};
const Style Style::bg_blue{Style::BgColorTag::predefined, PredefinedColor::blue};
const Style Style::bg_magenta{Style::BgColorTag::predefined, PredefinedColor::magenta};
const Style Style::bg_cyan{Style::BgColorTag::predefined, PredefinedColor::cyan};
const Style Style::bg_white{Style::BgColorTag::predefined, PredefinedColor::white};

std::string TextFormatter::format(const std::string &format_str, const std::vector<FormatParam> &params) const
{
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &param : params) {
        store.push_back(style(param.text(), param.styles()));
    }
    return fmt::vformat(format_str, store);
}

} // namespace porytiles2
