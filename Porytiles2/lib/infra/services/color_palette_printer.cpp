#include "porytiles2/infra/services/color_palette_printer.hpp"

#include <algorithm>
#include <ranges>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

template <std::size_t N>
std::vector<std::string> print_palette_with_highlights_impl(
    const Palette<Rgba32, N> &pal, const std::vector<std::size_t> &highlight_slots, const TextFormatter *format)
{
    std::vector<std::string> lines{};

    // JASC pal front matter
    lines.emplace_back("  JASC-PAL");
    lines.emplace_back("  0100");
    lines.push_back(format->format("  {}", FormatParam{pal.size()}));

    for (std::size_t i = 0; i < pal.size(); ++i) {
        const bool is_highlighted = std::ranges::find(highlight_slots, i) != highlight_slots.end();

        std::string prefix;
        if (is_highlighted) {
            prefix = format->format("{}", FormatParam{"➞ ", Style::bold | Style::yellow});
        }
        else {
            prefix = "  ";
        }

        std::string slot_str;
        if (pal.is_wildcard(i)) {
            slot_str = format->format("{}{}", FormatParam{prefix}, FormatParam{"*"});
        }
        else {
            const Rgba32 color = pal.at(i);
            const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());

            if (is_highlighted) {
                const std::string color_text =
                    format->style(color.to_jasc_str(), Style::bold | Style::italic | Style::yellow | color_style);
                slot_str = format->format("{}{}", FormatParam{prefix}, FormatParam{color_text});
            }
            else {
                const std::string color_text = format->style(color.to_jasc_str(), color_style);
                slot_str = format->format("{}{}", FormatParam{prefix}, FormatParam{color_text});
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
    return print_palette_with_highlights_impl(pal, {}, format_);
}

std::vector<std::string> ColorPalettePrinter::print_rgba_pal_with_highlights(
    const Palette<Rgba32> &pal, const std::vector<std::size_t> &slots) const
{
    return print_palette_with_highlights_impl(pal, slots, format_);
}

std::vector<std::string> ColorPalettePrinter::print_rgba_pal_with_highlights(
    const Palette<Rgba32, pal::max_size> &pal, const std::vector<std::size_t> &slots) const
{
    return print_palette_with_highlights_impl(pal, slots, format_);
}

std::vector<std::string> ColorPalettePrinter::print_pal_hint_with_highlights(
    const PaletteHint &hint, const std::vector<std::size_t> &slots) const
{
    std::vector<std::string> lines{};

    // JASC pal front matter
    lines.push_back(format_->format("name: \"{}\"", FormatParam{hint.name(), Style::bold}));
    lines.emplace_back("colors:");

    for (std::size_t i = 0; i < hint.pal().size(); ++i) {
        const bool is_highlighted = std::ranges::find(slots, i) != slots.end();

        std::string prefix;
        if (is_highlighted) {
            prefix = format_->format("{}", FormatParam{"➞ ", Style::bold | Style::yellow});
        }
        else {
            prefix = "  ";
        }

        std::string slot_str;
        if (hint.pal().is_wildcard(i)) {
            panic("illegal wildcard in pal hint '" + hint.name() + "' at index '" + std::to_string(i) + "'");
        }
        const Rgba32 color = hint.pal().at(i);
        const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());

        if (is_highlighted) {
            const std::string color_text =
                format_->style(color.to_csv_str(), Style::bold | Style::italic | color_style);
            slot_str = format_->format("{} - [ {} ]", FormatParam{prefix}, FormatParam{color_text});
        }
        else {
            const std::string color_text = format_->style(color.to_csv_str(), color_style);
            slot_str = format_->format("{} - [ {} ]", FormatParam{prefix}, FormatParam{color_text});
        }

        lines.push_back(slot_str);
    }

    return lines;
}

[[nodiscard]] std::vector<std::string> ColorPalettePrinter::print_rgba_palette_covered_missing(
    const Palette<Rgba32, pal::max_size> &pal, std::set<Rgba32> covered_colors, std::set<Rgba32> missing_colors) const
{
    // Build highlight_slots from covered_colors
    std::vector<std::size_t> highlight_slots;
    const auto index_to_color = pal.index_to_color_map();
    for (const auto &[index, color] : index_to_color) {
        if (covered_colors.contains(color)) {
            highlight_slots.push_back(index);
        }
    }

    // Use the common implementation for palette printing with highlights
    std::vector<std::string> lines = print_palette_with_highlights_impl(pal, highlight_slots, format_);

    // Append missing colors section if needed
    if (!missing_colors.empty()) {
        lines.push_back(format_->format("{}", FormatParam{"------ Palette Still Missing ------", Style::bold}));
        for (const auto &color : missing_colors) {
            const Style color_style = rgb_fg_style(color.red(), color.green(), color.blue());
            const std::string color_text = format_->style(color.to_jasc_str(), color_style);
            lines.push_back(format_->format("{}", FormatParam{color_text}));
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
