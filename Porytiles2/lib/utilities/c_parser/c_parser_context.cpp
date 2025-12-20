#include "porytiles2/utilities/c_parser/c_parser_context.hpp"

#include <string>
#include <vector>

#include "fmt/format.h"

namespace porytiles2 {

namespace {

std::string format_header(const std::string &file_path, SourcePosition pos, const std::string &message)
{
    if (!file_path.empty()) {
        return fmt::format("{}:{}:{}: {}", file_path, pos.line, pos.column, message);
    }
    return fmt::format("{}:{}: {}", pos.line, pos.column, message);
}

} // namespace

CParserContext::CParserContext(
    gsl::not_null<const std::vector<std::string> *> file_lines,
    gsl::not_null<const TextFormatter *> format,
    std::string file_path)
    : file_lines_{file_lines.get()}, format_{format.get()}, file_path_{std::move(file_path)}
{
}

FormattableError CParserContext::make_error(SourcePosition pos, const std::string &message) const
{
    std::vector<std::string> lines;

    // Always add the header line with position
    lines.push_back(format_header(file_path_, pos, message));

    // Add source context if position is valid
    if (!file_lines_->empty() && pos.line > 0 && pos.line <= file_lines_->size()) {
        // Convert 1-based SourcePosition to 0-based indices for FileHighlightPrinter
        const std::size_t line_idx = pos.line - 1;
        const std::string &target_line = (*file_lines_)[line_idx];

        const FileHighlightPrinter printer{format_};

        // Determine column index for highlighting
        if (pos.column > 0 && pos.column <= target_line.size()) {
            // Valid column position - show column highlight with caret
            const std::size_t col_idx = pos.column - 1;
            auto highlight_lines = printer.print(*file_lines_, line_idx, col_idx);
            for (auto &hl : highlight_lines) {
                lines.push_back(std::move(hl));
            }
        }
        else if (!target_line.empty()) {
            // Column out of bounds but line is valid - highlight the line without column caret
            auto highlight_lines = printer.print(*file_lines_, std::vector{line_idx});
            for (auto &hl : highlight_lines) {
                lines.push_back(std::move(hl));
            }
        }
        else {
            // Empty line - just highlight the line
            auto highlight_lines = printer.print(*file_lines_, std::vector{line_idx});
            for (auto &hl : highlight_lines) {
                lines.push_back(std::move(hl));
            }
        }
    }

    return FormattableError{std::move(lines)};
}

const std::vector<std::string> *CParserContext::file_lines() const
{
    return file_lines_;
}

const TextFormatter *CParserContext::formatter() const
{
    return format_;
}

const std::string &CParserContext::file_path() const
{
    return file_path_;
}

} // namespace porytiles2
