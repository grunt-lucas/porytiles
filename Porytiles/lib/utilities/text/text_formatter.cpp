#include "porytiles/utilities/text/text_formatter.hpp"

#include <string>
#include <vector>

#include "fmt/args.h"
#include "fmt/format.h"

namespace porytiles {

// Style static constants - Formatting
const Style Style::none{};
const Style Style::bold{Style::FormatFlagTag::bold};
const Style Style::faint{Style::FormatFlagTag::faint};
const Style Style::italic{Style::FormatFlagTag::italic};
const Style Style::underline{Style::FormatFlagTag::underline};
const Style Style::blink{Style::FormatFlagTag::blink};

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

// NOTE: This function uses fmt::dynamic_format_arg_store and fmt::vformat because
// C++23 std::format does not support runtime-variable argument counts.
// std::make_format_args requires compile-time known argument counts.
//
// Future alternatives:
// 1. C++26 may add better runtime format argument support
// 2. Custom placeholder replacement (limited to simple {} placeholders only)
// 3. Refactor callers to use compile-time known argument counts
//
// For now, fmt remains the only dependency that cannot be fully migrated to std::format.
std::string TextFormatter::format(const std::string &format_str, const std::vector<FormatParam> &params) const
{
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &param : params) {
        store.push_back(style(param.text(), param.styles()));
    }
    return fmt::vformat(format_str, store);
}

} // namespace porytiles
