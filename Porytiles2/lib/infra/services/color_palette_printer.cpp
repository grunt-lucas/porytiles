#include "porytiles2/infra/services/color_palette_printer.hpp"

#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

std::vector<std::string> ColorPalettePrinter::print_rgba_palette(const Palette<Rgba32> &pal) const
{
    std::vector<std::string> lines{};
    for (const auto &color : pal.colors()) {
        const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
        const std::string color_text = format_->style(color.to_jasc_str(), color_style);
        lines.push_back(format_->format("{}", FormatParam{color_text}));
    }
    return lines;
}

std::vector<std::string>
ColorPalettePrinter::print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const
{
    std::vector<std::string> lines{};
    for (const auto &[color, count] : colors_counts) {
        const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
        const std::string color_text = format_->style(color.to_jasc_str(), color_style);
        lines.push_back(format_->format("{} ➞ {} pixel(s)", FormatParam{color_text}, FormatParam{count}));
    }
    return lines;
}

} // namespace porytiles2
