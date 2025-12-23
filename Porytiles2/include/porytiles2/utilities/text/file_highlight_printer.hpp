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
     * Line numbers are displayed as 1-indexed in the output (matching user expectations), but the input indices are
     * 0-indexed (matching vector indices).
     *
     * @param lines The file contents as a vector of strings (one per line)
     * @param line_indices_to_highlight 0-indexed line indices to highlight (index 0 is the first line)
     * @param window_size Total number of context lines to show around highlighted line, defaults to 9
     * @pre All indices in line_indices_to_highlight must be < lines.size()
     * @return Formatted output lines ready for display
     */
    [[nodiscard]] std::vector<std::string> print(
        const std::vector<std::string> &lines,
        const std::vector<std::size_t> &line_indices_to_highlight,
        std::size_t window_size = 9) const;

    /**
     * @brief Prints lines with a specific line and column highlighted.
     *
     * @details
     * Creates a formatted view showing a window of context around the highlighted line. The specified column is
     * highlighted with underline styling, and a caret indicator line is added below the highlighted line pointing to
     * the column position.
     *
     * @param lines The file contents as a vector of strings (one per line)
     * @param line_index_to_highlight 0-indexed line index to highlight
     * @param col_to_highlight 0-indexed column position to highlight within the line
     * @param window_size Total number of context lines to show around highlighted line, defaults to 9
     * @pre line_index_to_highlight must be < lines.size()
     * @pre col_to_highlight must be < lines[line_index_to_highlight].size()
     * @return Formatted output lines ready for display
     */
    [[nodiscard]] std::vector<std::string> print(
        const std::vector<std::string> &lines,
        std::size_t line_index_to_highlight,
        std::size_t col_to_highlight,
        std::size_t window_size = 9) const;

    /**
     * @brief Filesystem path-based overload.
     *
     * @param file The path to the file for printing
     * @param line_indices_to_highlight 0-indexed line indices to highlight (index 0 is the first line)
     * @param window_size Total number of context lines to show around highlighted line, defaults to 9
     * @pre All indices in line_indices_to_highlight must be < lines.size()
     * @return Formatted output lines ready for display
     */
    [[nodiscard]] std::vector<std::string> print(
        const std::filesystem::path &file,
        const std::vector<std::size_t> &line_indices_to_highlight,
        std::size_t window_size = 9) const;

    /**
     * @brief Filesystem path-based overload.
     *
     * @param file The path to the file for printing
     * @param line_index_to_highlight 0-indexed line index to highlight
     * @param col_to_highlight 0-indexed column position to highlight within the line
     * @param window_size Total number of context lines to show around highlighted line, defaults to 9
     * @pre line_index_to_highlight must be < lines.size()
     * @pre col_to_highlight must be < lines[line_index_to_highlight].size()
     * @return Formatted output lines ready for display
     */
    [[nodiscard]] std::vector<std::string> print(
        const std::filesystem::path &file,
        std::size_t line_index_to_highlight,
        std::size_t col_to_highlight,
        std::size_t window_size = 9) const;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2
