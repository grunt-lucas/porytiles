#include "porytiles2/infra/services/color_palette_printer.hpp"

#include <algorithm>
#include <ranges>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

template <std::size_t N>
std::vector<std::string> print_palette_with_highlights_impl(
    const Palette<Rgba32, N> &pal, const std::vector<std::size_t> &slots, const TextFormatter *format)
{
    std::vector<std::string> lines{};

    for (std::size_t i = 0; i < pal.size(); ++i) {
        const bool is_highlighted = std::ranges::find(slots, i) != slots.end();

        std::string prefix;
        if (is_highlighted) {
            prefix = format->format("{}", FormatParam{"➞ ", Style::bold | Style::italic | Style::yellow});
        }
        else {
            prefix = "  ";
        }

        std::string slot_str;
        if (pal.is_wildcard(i)) {
            slot_str = format->format("{}[{}]: {}", FormatParam{prefix}, FormatParam{i}, FormatParam{"<wildcard>"});
        }
        else {
            const Rgba32 color = pal.at(i);
            const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());

            if (is_highlighted) {
                const std::string color_text =
                    format->style(color.to_jasc_str(), Style::bold | Style::italic | Style::yellow | color_style);
                slot_str = format->format("{}[{}]: {}", FormatParam{prefix}, FormatParam{i}, FormatParam{color_text});
            }
            else {
                const std::string color_text = format->style(color.to_jasc_str(), color_style);
                slot_str = format->format("{}[{}]: {}", FormatParam{prefix}, FormatParam{i}, FormatParam{color_text});
            }
        }

        lines.push_back(slot_str);
    }

    return lines;
}

} // namespace

namespace porytiles2 {

std::vector<std::string> ColorPalettePrinter::print_rgba_palette(const Palette<Rgba32, pal::max_size> &pal) const
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
    const Palette<Rgba32, pal::max_size> &pal, std::set<Rgba32> covered_colors, std::set<Rgba32> missing_colors) const
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

std::vector<std::string> ColorPalettePrinter::print_rgba_palette_with_highlights(
    const Palette<Rgba32> &pal, const std::vector<std::size_t> &slots) const
{
    return print_palette_with_highlights_impl(pal, slots, format_);
}

std::vector<std::string> ColorPalettePrinter::print_rgba_palette_with_highlights(
    const Palette<Rgba32, pal::max_size> &pal, const std::vector<std::size_t> &slots) const
{
    return print_palette_with_highlights_impl(pal, slots, format_);
}

} // namespace porytiles2
