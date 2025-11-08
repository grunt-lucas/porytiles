#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"

#include <array>
#include <string>

namespace {

using namespace porytiles2;

// ANSI reset code
const std::string ansi_reset = "\033[0m";

// Ordered list of style flags to check (bold first, then colors)
const std::array<std::pair<Style, std::string>, 8> style_mappings = {
    {{Style::bold, "\033[1m"},
     {Style::italic, "\033[3m"},
     {Style::red, "\033[31m"},
     {Style::green, "\033[32m"},
     {Style::blue, "\033[34m"},
     {Style::yellow, "\033[33m"},
     {Style::cyan, "\033[36m"},
     {Style::magenta, "\033[35m"}}};

} // namespace

namespace porytiles2 {

std::string AnsiStyledTextFormatter::style(const std::string &text, Style styles) const
{
    // If no styles are set, return text unchanged
    if (styles == Style::none) {
        return text;
    }

    // Build the ANSI code prefix by checking each style flag
    std::string prefix;
    for (const auto &[flag, ansi_code] : style_mappings) {
        if (has_style(styles, flag)) {
            prefix += ansi_code;
        }
    }

    return prefix + text + ansi_reset;
}

} // namespace porytiles2