#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"

#include <map>
#include <string>

#include "porytiles2/xcut/panic/panic.hpp"

namespace {

// ANSI reset code
const std::string ansi_reset = "\033[0m";

// Map of Style enum values to their corresponding ANSI escape codes
const std::map<porytiles2::Style, std::string> style_to_ansi = {
    {porytiles2::Style::bold, "\033[1m"},
    {porytiles2::Style::red, "\033[31m"},
    {porytiles2::Style::green, "\033[32m"},
    {porytiles2::Style::blue, "\033[34m"},
    {porytiles2::Style::yellow, "\033[33m"},
    {porytiles2::Style::cyan, "\033[36m"},
    {porytiles2::Style::magenta, "\033[35m"}};

} // namespace

namespace porytiles2 {

std::string AnsiStyledTextFormatter::style(const std::string &text, Style style) const
{
    if (!style_to_ansi.contains(style)) {
        panic("style_to_ansi map did not contain requested style");
    }
    const auto &style_code = style_to_ansi.at(style);
    return style_code + text + ansi_reset;
}

} // namespace porytiles2