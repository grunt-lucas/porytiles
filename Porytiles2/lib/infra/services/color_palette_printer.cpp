#include "porytiles2/infra/services/color_palette_printer.hpp"

#include <ranges>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

std::vector<std::string> ColorPalettePrinter::print_rgba_palette(const Palette<Rgba32> &pal) const
{
    std::vector<std::string> lines{};

    if (pal.size() >= 1) {
        // First, print slot 0 color
        const Rgba32 &slot_zero = pal.slot_zero_color();
        const Style slot_zero_style = rgb_fg_style(slot_zero.red(), slot_zero.green(), slot_zero.blue());
        const std::string slot_zero_text = format_->style(slot_zero.to_jasc_str(), slot_zero_style);
        lines.push_back(format_->format("{}", FormatParam{slot_zero_text}));

        // Then iterate over remaining colors (indices 1 through size-1) in order
        const auto index_to_color = pal.index_to_color_map();
        for (const auto &color : index_to_color | std::views::values) {
            const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
            const std::string color_text = format_->style(color.to_jasc_str(), color_style);
            lines.push_back(format_->format("{}", FormatParam{color_text}));
        }
    }

    return lines;
}

[[nodiscard]] std::vector<std::string> ColorPalettePrinter::print_rgba_palette_covered_missing(
    const Palette<Rgba32> &pal, std::set<Rgba32> covered_colors, std::set<Rgba32> missing_colors) const
{
    std::vector<std::string> lines{};

    if (pal.size() >= 1) {
        // First, print slot 0 color
        const Rgba32 &slot_zero = pal.slot_zero_color();
        const Style slot_zero_style = rgb_fg_style(slot_zero.red(), slot_zero.green(), slot_zero.blue());
        const std::string slot_zero_text = format_->style(slot_zero.to_jasc_str(), slot_zero_style);
        lines.push_back(format_->format("{}", FormatParam{slot_zero_text}));

        // Then iterate over remaining colors (indices 1 through size-1) in order
        const auto index_to_color = pal.index_to_color_map();
        for (const auto &color : index_to_color | std::views::values) {
            if (covered_colors.contains(color)) {
                const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
                const std::string color_text =
                    format_->style(color.to_jasc_str(), Style::bold | Style::italic | color_style);
                lines.push_back(format_->format(
                    "{} {}", FormatParam{color_text}, FormatParam{"← covered", Style::bold | Style::yellow}));
            }
            else {
                const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
                const std::string color_text = format_->style(color.to_jasc_str(), color_style);
                lines.push_back(format_->format("{}", FormatParam{color_text}));
            }
        }
        if (!missing_colors.empty()) {
            lines.push_back(format_->format("{}", FormatParam{"------ Palette Would Need ------", Style::bold}));
            for (const auto &color : missing_colors) {
                const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
                const std::string color_text = format_->style(color.to_jasc_str(), color_style);
                lines.push_back(format_->format("{}", FormatParam{color_text}));
            }
        }
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
