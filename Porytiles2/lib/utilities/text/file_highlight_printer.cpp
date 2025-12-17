#include "porytiles2/utilities/text/file_highlight_printer.hpp"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

namespace {

constexpr std::size_t base_prefix_width = 3;
const Style highlight_style = Style::bold | Style::italic | Style::yellow;

struct WindowBounds {
    std::size_t start;
    std::size_t end;
    std::size_t max_digits;
};

WindowBounds
compute_window_bounds(std::size_t min_line, std::size_t max_line, std::size_t window_size, std::size_t total_lines)
{
    const std::size_t half_window = (window_size - 1) / 2;
    const std::size_t start = (min_line >= half_window) ? min_line - half_window : 0;
    const std::size_t end = std::min(max_line + half_window + 1, total_lines);
    const std::size_t max_digits = std::to_string(end).length();
    return {start, end, max_digits};
}

std::size_t compute_prefix_width(std::size_t line_num_display, std::size_t max_digits)
{
    const std::size_t current_digits = std::to_string(line_num_display).length();
    return base_prefix_width + (max_digits - current_digits);
}

std::string format_prefix(bool is_highlighted, std::size_t prefix_width, const TextFormatter *format)
{
    if (is_highlighted) {
        const std::string arrow_with_spaces = "➞" + std::string(prefix_width - 1, ' ');
        return format->format("{}", FormatParam{arrow_with_spaces, highlight_style});
    }
    return std::string(prefix_width, ' ');
}

std::string format_plain_line(const std::string &prefix, std::size_t line_num, const std::string &content)
{
    return prefix + std::to_string(line_num) + ":   " + content;
}

std::string format_highlighted_line(
    const std::string &prefix, std::size_t line_num, const std::string &content, const TextFormatter *format)
{
    const auto styled_content = format->format("{}", FormatParam{content, highlight_style});
    return prefix + std::to_string(line_num) + ":   " + styled_content;
}

} // namespace

FileHighlightPrinter::FileHighlightPrinter(gsl::not_null<const TextFormatter *> format) : format_{format} {}

std::vector<std::string> FileHighlightPrinter::print(
    const std::vector<std::string> &lines,
    const std::vector<std::size_t> &line_indices_to_highlight,
    std::size_t window_size) const
{
    std::vector<std::string> result;

    if (lines.empty()) {
        return result;
    }

    // Build a set for O(1) lookup and find min/max
    std::set<std::size_t> highlight_set;
    std::size_t min_line = std::numeric_limits<std::size_t>::max();
    std::size_t max_line = 0;

    for (const auto line_index : line_indices_to_highlight) {
        if (line_index >= lines.size()) {
            panic("invalid line index " + std::to_string(line_index) + ": index out of bounds");
        }
        highlight_set.insert(line_index);
        min_line = std::min(min_line, line_index);
        max_line = std::max(max_line, line_index);
    }

    if (highlight_set.empty()) {
        return result;
    }

    const auto bounds = compute_window_bounds(min_line, max_line, window_size, lines.size());

    for (std::size_t i = bounds.start; i < bounds.end; ++i) {
        const bool is_highlighted = highlight_set.contains(i);
        const std::size_t line_num = i + 1;
        const std::size_t prefix_width = compute_prefix_width(line_num, bounds.max_digits);
        const std::string prefix = format_prefix(is_highlighted, prefix_width, format_);

        if (is_highlighted) {
            result.push_back(format_highlighted_line(prefix, line_num, lines[i], format_));
        }
        else {
            result.push_back(format_plain_line(prefix, line_num, lines[i]));
        }
    }

    return result;
}

std::vector<std::string> FileHighlightPrinter::print(
    const std::vector<std::string> &lines,
    std::size_t line_index_to_highlight,
    std::size_t col_to_highlight,
    std::size_t window_size) const
{
    std::vector<std::string> result;

    if (lines.empty()) {
        return result;
    }

    if (line_index_to_highlight >= lines.size()) {
        panic("invalid line index " + std::to_string(line_index_to_highlight) + ": index out of bounds");
    }

    const std::string &target_line = lines[line_index_to_highlight];
    if (col_to_highlight >= target_line.size()) {
        panic(
            "invalid column index " + std::to_string(col_to_highlight) + ": index out of bounds for line " +
            std::to_string(line_index_to_highlight));
    }

    const auto bounds =
        compute_window_bounds(line_index_to_highlight, line_index_to_highlight, window_size, lines.size());

    for (std::size_t i = bounds.start; i < bounds.end; ++i) {
        const bool is_highlighted = (i == line_index_to_highlight);
        const std::size_t line_num = i + 1;
        const std::size_t prefix_width = compute_prefix_width(line_num, bounds.max_digits);
        const std::string prefix = format_prefix(is_highlighted, prefix_width, format_);

        if (is_highlighted) {
            // Split line into three parts for column-specific styling
            const std::string before = target_line.substr(0, col_to_highlight);
            const std::string at_col = target_line.substr(col_to_highlight, 1);
            const std::string after =
                (col_to_highlight + 1 < target_line.size()) ? target_line.substr(col_to_highlight + 1) : "";

            const Style col_style = highlight_style | Style::underline;

            const auto formatted_before = format_->format("{}", FormatParam{before, highlight_style});
            const auto formatted_at = format_->format("{}", FormatParam{at_col, col_style});
            const auto formatted_after = format_->format("{}", FormatParam{after, highlight_style});

            result.push_back(
                prefix + std::to_string(line_num) + ":   " + formatted_before + formatted_at + formatted_after);

            // Add caret indicator line
            const std::size_t indent = prefix_width + std::to_string(line_num).length() + 4 + col_to_highlight;
            const std::string caret =
                std::string(indent, ' ') + format_->format("{}", FormatParam{"^", Style::bold | Style::green});
            result.push_back(caret);
        }
        else {
            result.push_back(format_plain_line(prefix, line_num, lines[i]));
        }
    }

    return result;
}

} // namespace porytiles2
