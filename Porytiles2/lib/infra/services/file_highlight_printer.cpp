#include "porytiles2/infra/services/file_highlight_printer.hpp"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

FileHighlightPrinter::FileHighlightPrinter(gsl::not_null<const TextFormatter *> format) : format_{format} {}

std::vector<std::string> FileHighlightPrinter::print(
    const std::vector<std::string> &lines,
    std::size_t window_size,
    const std::vector<std::size_t> &line_indices_to_highlight) const
{
    std::vector<std::string> result;

    if (lines.empty()) {
        return result;
    }

    // Build a set for O(1) lookup (input is already 0-indexed)
    std::set<std::size_t> highlight_set;
    std::size_t min_line = std::numeric_limits<std::size_t>::max();
    std::size_t max_line = 0;

    for (const auto line_index : line_indices_to_highlight) {
        if (line_index >= lines.size()) {
            // Panic on invalid line indices
            panic("invalid line index " + std::to_string(line_index) + ": index out of bounds");
        }
        highlight_set.insert(line_index);
        min_line = std::min(min_line, line_index);
        max_line = std::max(max_line, line_index);
    }

    if (highlight_set.empty()) {
        return result;
    }

    // Calculate window boundaries
    // The window should show context around all highlighted lines
    const std::size_t half_window = (window_size - 1) / 2;
    const std::size_t start = (min_line >= half_window) ? min_line - half_window : 0;
    const std::size_t end = std::min(max_line + half_window + 1, lines.size());

    // Calculate prefix width dynamically based on the maximum line number in the window
    // This ensures right-alignment of line numbers for any file size
    const std::size_t max_line_display = end; // end is 1-indexed (0-indexed + 1 lines shown)
    const std::size_t max_digits = std::to_string(max_line_display).length();

    // Build contextual view
    for (std::size_t i = start; i < end; ++i) {
        constexpr std::size_t base_prefix_width = 3; // minimum spacing before line number

        const bool is_highlighted = highlight_set.contains(i);

        // Calculate prefix width to right-align line numbers
        const std::size_t line_num_display = i + 1;
        const std::size_t current_digits = std::to_string(line_num_display).length();
        const std::size_t prefix_width = base_prefix_width + (max_digits - current_digits);

        std::string prefix;
        if (is_highlighted) {
            // Arrow (1 visual column) + remaining spaces
            const std::string arrow_with_spaces = "➞" + std::string(prefix_width - 1, ' ');
            prefix = format_->format("{}", FormatParam{arrow_with_spaces, Style::bold | Style::italic | Style::yellow});
        }
        else {
            prefix = std::string(prefix_width, ' ');
        }

        // Format the line with line number (1-indexed for display)
        if (is_highlighted) {
            const auto highlight_line =
                format_->format("{}", FormatParam{lines[i], Style::bold | Style::italic | Style::yellow});
            result.push_back(prefix + std::to_string(line_num_display) + ":   " + highlight_line);
        }
        else {
            result.push_back(prefix + std::to_string(line_num_display) + ":   " + lines[i]);
        }
    }

    return result;
}

} // namespace porytiles2
