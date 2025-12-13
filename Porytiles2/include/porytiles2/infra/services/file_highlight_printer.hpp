#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A service for printing file lines with highlighted lines and line numbers.
 *
 * @details
 * FileHighlightPrinter formats file content for display with specific lines highlighted. It's useful for showing
 * contextual views of source files around error or warning locations. The output includes line numbers and visual
 * highlighting (arrow prefix and styling) for the specified lines.
 *
 * Example output:
 * ```
 *       8:   some_config: value
 *       9:   another_config: value
 * >    10:   highlighted_line: value
 *      11:   next_config: value
 *      12:   last_config: value
 * ```
 */
class FileHighlightPrinter {
  public:
    /**
     * @brief Constructs a FileHighlightPrinter with a text formatter for styling.
     *
     * @param format The text formatter to use for styled output
     */
    explicit FileHighlightPrinter(gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Prints lines with specified lines highlighted and line numbers shown.
     *
     * @details
     * Creates a formatted view of the given lines, showing a window of context around the highlighted lines. Each line
     * in the output includes a prefix (arrow for highlighted lines), line number, and the line content. Highlighted
     * lines are styled with bold/italic/yellow formatting.
     *
     * When multiple lines are highlighted, the window expands to show all highlighted lines plus context around them.
     *
     * @param lines The file contents as a vector of strings (one per line)
     * @param window_size Total number of context lines to show around highlighted lines
     * @param line_nums_to_highlight 1-indexed line numbers to highlight (line 1 is the first line)
     * @pre All line numbers in line_nums_to_highlight must be >= 1 and <= lines.size()
     * @return Formatted output lines ready for display
     */
    [[nodiscard]] std::vector<std::string> print(
        const std::vector<std::string> &lines,
        std::size_t window_size,
        const std::vector<std::size_t> &line_nums_to_highlight) const;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2
